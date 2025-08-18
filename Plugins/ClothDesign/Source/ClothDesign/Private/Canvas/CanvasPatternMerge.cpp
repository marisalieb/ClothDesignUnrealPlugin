#include "Canvas/CanvasPatternMerge.h"

#include "PatternMesh.h" 
#include "PatternSewingConstraint.h" 
#include "Algo/Reverse.h"
#include "Misc/MessageDialog.h"
#include "Engine/World.h"
#include "Editor.h" 
#include "Containers/Set.h"

#include "Canvas/CanvasUtils.h"
#include "ClothSimSettings.h"

#include "DynamicMesh/Operations/MergeCoincidentMeshEdges.h"
#include "DynamicMesh/MeshNormals.h"

#if WITH_EDITOR
#include "CoreMinimal.h"
#include "UDynamicMesh.h" 
#include "GeometryScript/CreateNewAssetUtilityFunctions.h"
#include "GeometryScript/GeometryScriptTypes.h"
#include "Editor.h"
#include "Animation/SkeletalMeshActor.h"
#include "GeometryScript/MeshBoneWeightFunctions.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#endif


FCanvasPatternMerge::FCanvasPatternMerge(
    TArray<TWeakObjectPtr<APatternMesh>>& InSpawnedActors,
    TArray<FPatternSewingConstraint>& InAllSeams)
    : SpawnedActorsRef(InSpawnedActors)
    , AllSeamsRef(InAllSeams)
{}

void FCanvasPatternMerge::BuildActorListAndIndexMap(
    TArray<APatternMesh*>& OutActors,
    TMap<APatternMesh*, int32>& OutMap) const
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

void FCanvasPatternMerge::BuildAdjacencyFromSeams(
    const TArray<APatternMesh*>& Actors,
    const TMap<APatternMesh*,int32>& ActorToIndex,
    TArray<TArray<int32>>& OutAdj) const
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

void FCanvasPatternMerge::FindConnectedComponents(
    const TArray<TArray<int32>>& Adj,
    TArray<TArray<int32>>& OutComponents)
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

bool FCanvasPatternMerge::ComponentHasExternalEdges(
    const TArray<int32>& Component,
    const TMap<APatternMesh*,int32>& ActorToIndex) const
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

bool FCanvasPatternMerge::MergeComponentToDynamicMesh(
    const TArray<int32>& Component,
    const TArray<APatternMesh*>& Actors,
    UE::Geometry::FDynamicMesh3& OutMerged)
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
            if (!Remap.Contains(T.C) || !Remap.Contains(T.B) || !Remap.Contains(T.A)) continue;
            OutMerged.AppendTriangle(Remap[T.C], Remap[T.B], Remap[T.A]);
        }
        
    }
    // Step 2: Merge vertices along seams
    double MergeSearchTolerance = 5.5;
    double MergeVertexTolerance = 1.05;

    UE::Geometry::FMergeCoincidentMeshEdges Merger(&OutMerged);
    Merger.MergeSearchTolerance = MergeSearchTolerance;
    Merger.MergeVertexTolerance = MergeVertexTolerance;
    Merger.bWeldAttrsOnMergedEdges = true;

    bool bMerged = Merger.Apply();
    if (bMerged)
    {
        UE_LOG(LogTemp, Log, TEXT("[Merge] MergeCoincidentMeshEdges succeeded. initial boundary edges: %d final: %d"),
            Merger.InitialNumBoundaryEdges, Merger.FinalNumBoundaryEdges);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Merge] MergeCoincidentMeshEdges did not merge anything."));
    }

    // Step 3: Recompute normals (optional)
    UE::Geometry::FMeshNormals Normals(&OutMerged);
    Normals.ComputeVertexNormals();

    return OutMerged.TriangleCount() > 0;
}

APatternMesh* FCanvasPatternMerge::SpawnMergedActorFromDynamicMesh(
    UE::Geometry::FDynamicMesh3&& MergedMesh)
{
    // compute centroid in world space (MergedMesh currently stores world positions)
    FVector3d Centroid3d = FCanvasUtils::ComputeAreaWeightedCentroid(MergedMesh);
    FVector CentroidF(Centroid3d.X, Centroid3d.Y, Centroid3d.Z);

    // Translate the dynamic mesh so centroid moves to origin (local coords)
    FCanvasUtils::TranslateDynamicMeshBy(MergedMesh, Centroid3d);

    
    static int32 MeshCounter = 0;
    FString UniqueLabel = FString::Printf(TEXT("MergedPatternMesh_%d"), MeshCounter++);
    
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return nullptr;

    FActorSpawnParameters Params;
    FTransform SpawnTransform;
    SpawnTransform.SetLocation(CentroidF);

    APatternMesh* MergedActor = World->SpawnActor<APatternMesh>(APatternMesh::StaticClass(), SpawnTransform, Params);
    // APatternMesh* MergedActor = World->SpawnActor<APatternMesh>(Params);
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

void FCanvasPatternMerge::ReplaceActorsWithMerged(
    const TArray<int32>& Component,
    const TArray<APatternMesh*>& Actors,
    APatternMesh* MergedActor) const
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

void FCanvasPatternMerge::RemoveInternalSeams(
    const TArray<int32>& Component,
    const TMap<APatternMesh*,int32>& ActorToIndex) const
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

void FCanvasPatternMerge::MergeSewnGroups() const
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
        if (ComponentHasExternalEdges(Comp, ActorToIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("[Merge] Skipping component size %d: has external seams."), Comp.Num());
            continue;
        }

        UE::Geometry::FDynamicMesh3 Merged;
        if (!MergeComponentToDynamicMesh(Comp, Actors, Merged)) { UE_LOG(LogTemp, Warning, TEXT("[Merge] merged had no triangles")); continue; }
        
        APatternMesh* MergedActor = SpawnMergedActorFromDynamicMesh(MoveTemp(Merged));
        if (!MergedActor) { UE_LOG(LogTemp, Warning, TEXT("[Merge] spawn failed")); continue; }

        // Now update your lists (or you may have destroyed the merged actor already)
        ReplaceActorsWithMerged(Comp, Actors, MergedActor);
        RemoveInternalSeams(Comp, ActorToIndex);
        
        // after MergedActor is created and has .DynamicMesh populated
#if WITH_EDITOR
        UDynamicMesh* TempDyn = NewObject<UDynamicMesh>(GetTransientPackage(), NAME_None);
        if (TempDyn)
        {
            TempDyn->SetMesh(MergedActor->DynamicMesh); // copy FDynamicMesh3 into UDynamicMesh

            FString SafeLabel = MergedActor->GetActorLabel();
            SafeLabel.ReplaceInline(TEXT(" "), TEXT("_"));
            FString Guid = FGuid::NewGuid().ToString(EGuidFormats::Digits);
            FString AssetPathAndName = FString::Printf(TEXT("/Game/ClothDesign/MergedClothPattern/%s"), *SafeLabel);

            // Use the static helper that creates bone weights then the skeletal asset
            USkeletalMesh* NewSkel = CreateSkeletalFromFDynamicMesh(TempDyn, AssetPathAndName);
            if (NewSkel)
            {
                UWorld* World = GEditor->GetEditorWorldContext().World();
                if (World)
                {
                    FActorSpawnParameters SpawnParams;
                    FTransform SpawnTransform = MergedActor->GetActorTransform();
                    ASkeletalMeshActor* SkelActor = World->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), SpawnTransform, SpawnParams);
                    if (SkelActor && SkelActor->GetSkeletalMeshComponent())
                    {
                        SkelActor->GetSkeletalMeshComponent()->SetSkeletalMesh(NewSkel);
#if WITH_EDITOR
                        SkelActor->SetFolderPath(FName(TEXT("ClothDesignActors")));
                        SkelActor->SetActorLabel(FString::Printf(TEXT("%s"), *SafeLabel));
#endif
                        // remove the merged APatternMesh if you want:
                        MergedActor->Destroy();
                        MergedActor = nullptr;
                    }
                }
                
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Merge] CreateSkeletalFromFDynamicMesh failed for %s"), *AssetPathAndName);
            }
        }
#endif


     // Now update your lists (or you may have destroyed the merged actor already)
     ReplaceActorsWithMerged(Comp, Actors, MergedActor);
     RemoveInternalSeams(Comp, ActorToIndex);
     }
}



USkeletalMesh* FCanvasPatternMerge::CreateSkeletalFromFDynamicMesh(UDynamicMesh* DynMesh, const FString& AssetPathAndName)
{
#if WITH_EDITOR
    if (!IsValid(DynMesh))
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateSkeletalFromFDynamicMesh_Static: DynMesh null"));
        return nullptr;
    }

    // optional debug object
    UGeometryScriptDebug* DebugObj = NewObject<UGeometryScriptDebug>(GetTransientPackage(), NAME_None);

    // 1) Create/replace a bone-weight profile named "Default"
    bool bReplaceExistingProfile = true;
    FGeometryScriptBoneWeightProfile Profile;
    Profile.ProfileName = FName(TEXT("Default"));

    bool bProfileExisted = UGeometryScriptLibrary_MeshBoneWeightFunctions::MeshCreateBoneWeights(
        DynMesh,
        bReplaceExistingProfile,
        DebugObj,
        Profile
    );

    // 2) Set all vertex weights to bone 0 with weight 1.0 (rigid bind)
    FGeometryScriptBoneWeight SingleBW;
    SingleBW.BoneIndex = 0;
    SingleBW.Weight = 1.0f;
    TArray<FGeometryScriptBoneWeight> AllWeights;
    AllWeights.Add(SingleBW);

    // Some builds accept Debug as optional 4th param; include it for safety
    UGeometryScriptLibrary_MeshBoneWeightFunctions::SetAllVertexBoneWeights(
        DynMesh,
        AllWeights,
        Profile,
        DebugObj
    );

    // 3) Load a small skeleton asset you've included in your plugin content.
    // Put the skeleton uasset under Plugins/YourPlugin/Content/SimpleSkeleton.uasset
    // and use the object path "/Plugin/YourPlugin/SimpleSkeleton.SimpleSkeleton"
    USkeleton* SkeletonAsset = LoadObject<USkeleton>(nullptr, TEXT("/Game/ClothDesign/SkelAsset/SK_ProcMesh.SK_ProcMesh"));
    if (!SkeletonAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateSkeletal: could not load skeleton asset at '/Plugin/...' - please add it to plugin Content"));
        return nullptr;
    }

    // 4) Create the skeletal mesh asset (pass the skeleton so generator has bone definitions)
    FGeometryScriptCreateNewSkeletalMeshAssetOptions Options;
    Options.bEnableRecomputeNormals = false;
    Options.bEnableRecomputeTangents = false;
    EGeometryScriptOutcomePins Outcome = EGeometryScriptOutcomePins::Failure;

    USkeletalMesh* NewSkeletal = UGeometryScriptLibrary_CreateNewAssetFunctions::CreateNewSkeletalMeshAssetFromMesh(
        DynMesh,          // UDynamicMesh*
        SkeletonAsset,    // pass real skeleton (not nullptr)
        AssetPathAndName,
        Options,
        Outcome,
        DebugObj
    );

    if (!IsValid(NewSkeletal) || Outcome != EGeometryScriptOutcomePins::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateNewSkeletalMeshAssetFromMesh failed. Outcome=%d"), (int32)Outcome);
        return nullptr;
    }
    
  


    UE_LOG(LogTemp, Display, TEXT("Created SkeletalMesh asset at %s"), *AssetPathAndName);
    return NewSkeletal;
    
#else
    return nullptr;
#endif
}




