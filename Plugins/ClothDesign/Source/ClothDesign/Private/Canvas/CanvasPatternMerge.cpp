#include "Canvas/CanvasPatternMerge.h"

#include "PatternMesh.h"                 // APatternMesh
#include "PatternSewingConstraint.h"     // FPatternSewingConstraint
#include "Algo/Reverse.h"
#include "Misc/MessageDialog.h"
#include "Engine/World.h"
#include "Editor.h"                      // GEditor
#include "Containers/Set.h"

FCanvasPatternMerge::FCanvasPatternMerge(
    TArray<TWeakObjectPtr<APatternMesh>>& InSpawnedActors,
    TArray<FPatternSewingConstraint>& InAllSeams)
    : SpawnedActorsRef(InSpawnedActors)
    , AllSeamsRef(InAllSeams)
{}

void FCanvasPatternMerge::BuildActorListAndIndexMap(TArray<APatternMesh*>& OutActors, TMap<APatternMesh*, int32>& OutMap) const
{
    OutActors.Reset();
    OutMap.Empty();
    OutActors.Reserve(SpawnedActorsRef.Num());
    for (const TWeakObjectPtr<APatternMesh>& W : SpawnedActorsRef)
    {
        OutActors.Add(W.Get()); // may add nullptrs; callers handle them
    }
    for (int i = 0; i < OutActors.Num(); ++i)
    {
        if (OutActors[i]) OutMap.Add(OutActors[i], i);
    }
}

void FCanvasPatternMerge::BuildAdjacencyFromSeams(const TArray<APatternMesh*>& Actors, const TMap<APatternMesh*,int32>& ActorToIndex, TArray<TArray<int32>>& OutAdj) const
{
    int32 N = Actors.Num();
    OutAdj.SetNumZeroed(N);

    for (const FPatternSewingConstraint& Seam : AllSeamsRef)
    {
        if (!Seam.MeshA || !Seam.MeshB) continue;

        APatternMesh* A = nullptr;
        APatternMesh* B = nullptr;
        for (const TPair<APatternMesh*, int32>& Pair : ActorToIndex)
        {
            if (Pair.Key && Pair.Key->MeshComponent == Seam.MeshA) A = Pair.Key;
            if (Pair.Key && Pair.Key->MeshComponent == Seam.MeshB) B = Pair.Key;
        }
        if (!A || !B) continue;
        int ai = ActorToIndex[A];
        int bi = ActorToIndex[B];
        if (ai == bi) continue;
        OutAdj[ai].AddUnique(bi);
        OutAdj[bi].AddUnique(ai);
    }
}

void FCanvasPatternMerge::FindConnectedComponents(const TArray<TArray<int32>>& Adj, TArray<TArray<int32>>& OutComponents) const
{
    OutComponents.Reset();
    int32 N = Adj.Num();
    TArray<char> Visited; Visited.Init(0, N);

    for (int i = 0; i < N; ++i)
    {
        if (Visited[i] || Adj[i].Num() == 0) continue;
        TArray<int32> Stack; Stack.Add(i);
        TArray<int32> Comp;
        while (Stack.Num())
        {
            int cur = Stack.Pop();
            if (Visited[cur]) continue;
            Visited[cur] = 1;
            Comp.Add(cur);
            for (int nb : Adj[cur]) if (!Visited[nb]) Stack.Add(nb);
        }
        if (Comp.Num()) OutComponents.Add(MoveTemp(Comp));
    }
}

bool FCanvasPatternMerge::ComponentHasExternalEdges(const TArray<int32>& Component, const TArray<APatternMesh*>& Actors, const TMap<APatternMesh*,int32>& ActorToIndex) const
{
    TSet<int32> Set; for (int idx : Component) Set.Add(idx);

    for (const FPatternSewingConstraint& Seam : AllSeamsRef)
    {
        if (!Seam.MeshA || !Seam.MeshB) continue;
        int idxA = INDEX_NONE, idxB = INDEX_NONE;
        for (const TPair<APatternMesh*, int32>& Pair : ActorToIndex)
        {
            if (Pair.Key && Pair.Key->MeshComponent == Seam.MeshA) idxA = Pair.Value;
            if (Pair.Key && Pair.Key->MeshComponent == Seam.MeshB) idxB = Pair.Value;
        }
        if (idxA == INDEX_NONE || idxB == INDEX_NONE) continue;
        bool aIn = Set.Contains(idxA), bIn = Set.Contains(idxB);
        if (aIn != bIn) return true;
    }
    return false;
}

bool FCanvasPatternMerge::MergeComponentToDynamicMesh(const TArray<int32>& Component, const TArray<APatternMesh*>& Actors, UE::Geometry::FDynamicMesh3& OutMerged) const
{
    OutMerged = UE::Geometry::FDynamicMesh3();
    for (int idx : Component)
    {
        APatternMesh* Src = Actors.IsValidIndex(idx) ? Actors[idx] : nullptr;
        if (!Src) continue;

        TMap<int32,int32> Remap;
        for (int vid : Src->DynamicMesh.VertexIndicesItr())
        {
            FVector3d p = Src->DynamicMesh.GetVertex(vid);
            FVector world = Src->GetActorTransform().TransformPosition(FVector(p.X,p.Y,p.Z));
            int newVid = OutMerged.AppendVertex(FVector3d(world));
            Remap.Add(vid, newVid);
        }

        for (int tid : Src->DynamicMesh.TriangleIndicesItr())
        {
            UE::Geometry::FIndex3i T = Src->DynamicMesh.GetTriangle(tid);
            if (!Remap.Contains(T.A) || !Remap.Contains(T.B) || !Remap.Contains(T.C)) continue;
            OutMerged.AppendTriangle(Remap[T.A], Remap[T.B], Remap[T.C]);
        }
    }
    return OutMerged.TriangleCount() > 0;
}

APatternMesh* FCanvasPatternMerge::SpawnMergedActorFromDynamicMesh(UE::Geometry::FDynamicMesh3&& MergedMesh) const
{
    static int32 MeshCounter = 0;
    FString UniqueLabel = FString::Printf(TEXT("MergedPatternMesh_%d"), MeshCounter++);
    
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return nullptr;

    FActorSpawnParameters Params;
    APatternMesh* MergedActor = World->SpawnActor<APatternMesh>(Params);
    if (!MergedActor) return nullptr;

    MergedActor->SetFolderPath(FName(TEXT("ClothDesignActors")));
#if WITH_EDITOR
    //MergedActor->SetActorLabel(TEXT("MergedPatternMesh"));
    MergedActor->SetActorLabel(UniqueLabel);
#endif

    MergedActor->DynamicMesh = MoveTemp(MergedMesh);

    // Extract arrays
    TArray<FVector> Verts; Verts.Reserve(MergedActor->DynamicMesh.VertexCount());
    TArray<int32> Inds; Inds.Reserve(MergedActor->DynamicMesh.TriangleCount()*3);
    for (int vid : MergedActor->DynamicMesh.VertexIndicesItr())
    {
        FVector3d p = MergedActor->DynamicMesh.GetVertex(vid);
        Verts.Add(FVector(p.X,p.Y,p.Z));
    }
    for (int tid : MergedActor->DynamicMesh.TriangleIndicesItr())
    {
        UE::Geometry::FIndex3i Tri = MergedActor->DynamicMesh.GetTriangle(tid);
        Inds.Add(Tri.C); Inds.Add(Tri.B); Inds.Add(Tri.A);
    }

    TArray<FVector> Normals; Normals.Init(FVector::UpVector, Verts.Num());
    TArray<FVector2D> UV0; UV0.Init(FVector2D::ZeroVector, Verts.Num());
    TArray<FLinearColor> VertexColors; VertexColors.Init(FLinearColor::White, Verts.Num());
    TArray<FProcMeshTangent> Tangents; Tangents.Init(FProcMeshTangent(1,0,0), Verts.Num());

    MergedActor->MeshComponent->CreateMeshSection_LinearColor(0, Verts, Inds, Normals, UV0, VertexColors, Tangents, true);

    return MergedActor;
}

void FCanvasPatternMerge::ReplaceActorsWithMerged(const TArray<int32>& Component, const TArray<APatternMesh*>& Actors, APatternMesh* MergedActor)
{
    if (!MergedActor) return;
    TSet<APatternMesh*> ToRemove;
    for (int idx : Component) if (Actors.IsValidIndex(idx) && Actors[idx]) ToRemove.Add(Actors[idx]);

    TArray<TWeakObjectPtr<APatternMesh>> NewList;
    NewList.Reserve(SpawnedActorsRef.Num());
    for (const TWeakObjectPtr<APatternMesh>& W : SpawnedActorsRef)
    {
        APatternMesh* A = W.Get();
        if (!A) continue;
        if (ToRemove.Contains(A))
        {
            A->Destroy();
            continue;
        }
        NewList.Add(W);
    }
    NewList.Add(TWeakObjectPtr<APatternMesh>(MergedActor));
    SpawnedActorsRef = MoveTemp(NewList);
}

void FCanvasPatternMerge::RemoveInternalSeams(const TArray<int32>& Component, const TArray<APatternMesh*>& Actors, const TMap<APatternMesh*,int32>& ActorToIndex)
{
    TSet<int32> CompSet; for (int idx : Component) CompSet.Add(idx);

    TArray<FPatternSewingConstraint> Kept;
    Kept.Reserve(AllSeamsRef.Num());
    for (const FPatternSewingConstraint& Seam : AllSeamsRef)
    {
        if (!Seam.MeshA || !Seam.MeshB) { Kept.Add(Seam); continue; }
        int idxA = INDEX_NONE, idxB = INDEX_NONE;
        for (const TPair<APatternMesh*, int32>& Pair : ActorToIndex)
        {
            if (Pair.Key && Pair.Key->MeshComponent == Seam.MeshA) idxA = Pair.Value;
            if (Pair.Key && Pair.Key->MeshComponent == Seam.MeshB) idxB = Pair.Value;
        }
        bool aIn = (idxA != INDEX_NONE) && CompSet.Contains(idxA);
        bool bIn = (idxB != INDEX_NONE) && CompSet.Contains(idxB);
        if (aIn && bIn) continue; // drop
        Kept.Add(Seam);
    }
    AllSeamsRef = MoveTemp(Kept);
}

void FCanvasPatternMerge::MergeSewnGroups()
{
    TArray<APatternMesh*> Actors;
    TMap<APatternMesh*,int32> ActorToIndex;
    BuildActorListAndIndexMap(Actors, ActorToIndex);

    TArray<TArray<int32>> Adj;
    BuildAdjacencyFromSeams(Actors, ActorToIndex, Adj);

    TArray<TArray<int32>> Components;
    FindConnectedComponents(Adj, Components);

    for (const TArray<int32>& Comp : Components)
    {
        if (Comp.Num() < 2) continue;
        if (ComponentHasExternalEdges(Comp, Actors, ActorToIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("[Merge] Skipping component size %d: has external seams."), Comp.Num());
            continue;
        }

        UE::Geometry::FDynamicMesh3 Merged;
        if (!MergeComponentToDynamicMesh(Comp, Actors, Merged)) { UE_LOG(LogTemp, Warning, TEXT("[Merge] merged had no triangles")); continue; }

        APatternMesh* MergedActor = SpawnMergedActorFromDynamicMesh(MoveTemp(Merged));
        if (!MergedActor) { UE_LOG(LogTemp, Warning, TEXT("[Merge] spawn failed")); continue; }

        ReplaceActorsWithMerged(Comp, Actors, MergedActor);
        RemoveInternalSeams(Comp, Actors, ActorToIndex);

        UE_LOG(LogTemp, Log, TEXT("[Merge] merged component of %d actors into %s"), Comp.Num(), *MergedActor->GetName());
    }
}
