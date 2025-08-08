#include "Canvas/CanvasSewing.h"
#include "Canvas/CanvasMesh.h"
#include "ClothDesignCanvas.h"

void FCanvasSewing::ClearAllSeams()
{
	SewingConstraints.Empty();
	AllDefinedSeams.Empty();
	SeamClickState = ESeamClickState::None;
	AStartTarget = FClickTarget();
	AEndTarget = FClickTarget();
	BStartTarget = FClickTarget();
	BEndTarget = FClickTarget();
}

void FCanvasSewing::FinalizeSeamDefinitionByTargets(
	const FClickTarget& AStart,
	const FClickTarget& AEnd,
	const FClickTarget& BStart,
	const FClickTarget& BEnd,
	const FInterpCurve<FVector2D>& CurvePoints,
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes)
{
	const int32 NumSeamPoints = 10;
	TArray<FVector2D> PointsA, PointsB;

	auto GetPt = [&](const FClickTarget& T) {
		if (T.ShapeIndex == INDEX_NONE)
			return CurvePoints.Points[T.PointIndex].OutVal;
		else
			return CompletedShapes[T.ShapeIndex].Points[T.PointIndex].OutVal;
	};

	FVector2D A1 = GetPt(AStart), A2 = GetPt(AEnd);
	FVector2D B1 = GetPt(BStart), B2 = GetPt(BEnd);

	for (int32 i = 0; i < NumSeamPoints; ++i)
	{
		float Alpha = float(i) / (NumSeamPoints - 1);
		PointsA.Add(FMath::Lerp(A1, A2, Alpha));
		PointsB.Add(FMath::Lerp(B1, B2, Alpha));
	}

	FPatternSewingConstraint NewSeam;
	NewSeam.ScreenPointsA = PointsA;
	NewSeam.ScreenPointsB = PointsB;
	AllDefinedSeams.Add(NewSeam);

	UE_LOG(LogTemp, Log, TEXT("Seam finalized between [%d,%d] and [%d,%d]"),
		AStart.ShapeIndex, AStart.PointIndex,
		BStart.ShapeIndex, BStart.PointIndex);
}


void FCanvasSewing::AlignSeamMeshes(
	APatternMesh* MeshActorA,
	APatternMesh* MeshActorB)
{
	// Access the stored vertex IDs and dynamic meshes:
	const TArray<int32>& IDsA = MeshActorA->LastSeamVertexIDs;
	const TArray<int32>& IDsB = MeshActorB->LastSeamVertexIDs;
	UE_LOG(LogTemp, Warning, TEXT("MeshActorA = %s, MeshActorB = %s"), *MeshActorA->GetName(), *MeshActorB->GetName());

	// 1) Guard against empty or mismatched arrays
	int32 NumA = IDsA.Num();
	int32 NumB = IDsB.Num();
	if (NumA == 0 || NumB == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot align: one of the seam lists is empty (A=%d, B=%d)"), NumA, NumB);
		return;
	}
	if (NumA != NumB)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot align: seam lists length mismatch (A=%d, B=%d)"), NumA, NumB);
		return;
	}

	// 2) Fetch world positions
	TArray<FVector> WorldA, WorldB;
	WorldA.Reserve(NumA);
	WorldB.Reserve(NumA);

	for (int i = 0; i < NumA; ++i)
	{
		FVector LocalA = FVector(MeshActorA->DynamicMesh.GetVertex(IDsA[i]));
		FVector LocalB = FVector(MeshActorB->DynamicMesh.GetVertex(IDsB[i]));
		WorldA.Add( MeshActorA->GetActorTransform().TransformPosition(LocalA) );
		WorldB.Add( MeshActorB->GetActorTransform().TransformPosition(LocalB) );
	}

	// 3) Compute average offset safely
	FVector TotalOffset = FVector::ZeroVector;
	for (int i = 0; i < NumA; ++i)
	{
		TotalOffset += (WorldA[i] - WorldB[i]);
	}

	// Now we know NumA > 0, so this is safe:
	FVector AverageOffset = TotalOffset / float(NumA);

	// 4) Apply the translation to MeshActorB
	FTransform NewTransform = MeshActorB->GetActorTransform();
	NewTransform.AddToTranslation(AverageOffset);
	MeshActorB->SetActorTransform(NewTransform);

	UE_LOG(LogTemp, Log, TEXT("Aligned MeshB by %s"), *AverageOffset.ToString());
}


void FCanvasSewing::BuildAndAlignClickedSeam(TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors,
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
		const FInterpCurve<FVector2D>& CurvePoints)
{
	// 1) Clear old actors
	for (auto& Weak : SpawnedPatternActors)
		if (auto* A = Weak.Get()) A->Destroy();
	SpawnedPatternActors.Empty();


	// Arrays to hold the seam IDs for A and B
	TArray<int32> SeamVertsA;
	TArray<int32> SeamVertsB;
	
	// 2) Build mesh and spawn actor for first shape
	{
		int32 ShapeIdx = AStartTarget.ShapeIndex;
		const FInterpCurve<FVector2D>& Shape = (ShapeIdx == INDEX_NONE) ? CurvePoints : CompletedShapes[ShapeIdx];
		// CanvasMesh::TriangulateAndBuildMesh(Shape, /*bRecordSeam=*/true, AStartTarget.PointIndex, AEndTarget.PointIndex);
		FDynamicMesh3 Mesh;
		// TArray<int32> SeamVerts;
		TArray<int32> DummySeamVerts;

		CanvasMesh::TriangulateAndBuildMesh(
			Shape,
			true,
			AStartTarget.PointIndex, AEndTarget.PointIndex,
			SeamVertsA, //SeamVerts,
			Mesh,
			DummySeamVerts,
			SpawnedPatternActors
			);
	}
	

	// 3) Build mesh and spawn actor for second shape
	{
		int32 ShapeIdx = BStartTarget.ShapeIndex;
		const FInterpCurve<FVector2D>& Shape = (ShapeIdx == INDEX_NONE) ? CurvePoints : CompletedShapes[ShapeIdx];
		// CanvasMesh::TriangulateAndBuildMesh(Shape, /*bRecordSeam=*/true, BStartTarget.PointIndex, BEndTarget.PointIndex);
		FDynamicMesh3 Mesh;
		// TArray<int32> SeamVerts;
		TArray<int32> DummySeamVerts;

		CanvasMesh::TriangulateAndBuildMesh(
			Shape,
			true,
			BStartTarget.PointIndex, BEndTarget.PointIndex,
			SeamVertsB, //SeamVerts,
			Mesh,
			DummySeamVerts,
			SpawnedPatternActors
			);
	}
	

	// Now check the weak array
	UE_LOG(LogTemp, Log, TEXT("SpawnedPatternActors.Num() = %d"), SpawnedPatternActors.Num());

	if (SpawnedPatternActors.Num() >= 2)
	{
		APatternMesh* A = SpawnedPatternActors[0].Get();
		APatternMesh* B = SpawnedPatternActors[1].Get();
		if (A && B)
		{
			AlignSeamMeshes(A, B);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Spawned actors exist but were garbage-collected or invalid"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Expected 2 SpawnedPatternActors, got %d"), SpawnedPatternActors.Num());
	}
	
}



void FCanvasSewing::MergeLastTwoMeshes()
{
    using namespace CanvasMesh;

    if (SpawnedPatternActors.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("Need at least two meshes to merge"));
        return;
    }

    TWeakObjectPtr<APatternMesh> AWeak = SpawnedPatternActors[SpawnedPatternActors.Num() - 2];
    TWeakObjectPtr<APatternMesh> BWeak = SpawnedPatternActors[SpawnedPatternActors.Num() - 1];

    APatternMesh* A = AWeak.Get();
    APatternMesh* B = BWeak.Get();

    if (!A || !B)
    {
        UE_LOG(LogTemp, Warning, TEXT("One of the two actors is invalid"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Merge] Source A: verts=%d tris=%d"), A->DynamicMesh.VertexCount(), A->DynamicMesh.TriangleCount());
    UE_LOG(LogTemp, Warning, TEXT("[Merge] Source B: verts=%d tris=%d"), B->DynamicMesh.VertexCount(), B->DynamicMesh.TriangleCount());

    // If either source has 0 triangles, bail (triangulation problem earlier)
    if (A->DynamicMesh.TriangleCount() == 0 || B->DynamicMesh.TriangleCount() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Merge] One source has no triangles; aborting merge"));
        return;
    }

    // Start with an empty merged mesh
    UE::Geometry::FDynamicMesh3 MergedMesh;
    // (Optional) Enable attributes if you need them later
    // MergedMesh.EnableAttributes();

    // Helper lambda to append one actor's mesh into MergedMesh
    auto AppendMeshIntoMerged = [&](APatternMesh* Src, const FTransform& SrcTransform)
    {
        TMap<int, int> Remap; // mapping old vertex id -> new vertex id

        // Append vertices
        for (int vid : Src->DynamicMesh.VertexIndicesItr())
        {
            FVector3d LocalP = Src->DynamicMesh.GetVertex(vid);
            FVector WorldP = SrcTransform.TransformPosition(FVector(LocalP.X, LocalP.Y, LocalP.Z));
            int newVid = MergedMesh.AppendVertex(FVector3d(WorldP));
            Remap.Add(vid, newVid);
        }

        // Append triangles using remapped indices
        for (int tid : Src->DynamicMesh.TriangleIndicesItr())
        {
            UE::Geometry::FIndex3i T = Src->DynamicMesh.GetTriangle(tid);
            int Aidx = Remap[T.A];
            int Bidx = Remap[T.B];
            int Cidx = Remap[T.C];
            MergedMesh.AppendTriangle(Aidx, Bidx, Cidx);
        }
    };

    // Append A then B
    AppendMeshIntoMerged(A, A->GetActorTransform());
    UE_LOG(LogTemp, Warning, TEXT("[Merge] After append-A: Verts=%d Tris=%d"), MergedMesh.VertexCount(), MergedMesh.TriangleCount());

    AppendMeshIntoMerged(B, B->GetActorTransform());
    UE_LOG(LogTemp, Warning, TEXT("[Merge] After append-B: Verts=%d Tris=%d"), MergedMesh.VertexCount(), MergedMesh.TriangleCount());

    // If still no triangles, something upstream is wrong
    if (MergedMesh.TriangleCount() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Merge] Merged mesh has 0 triangles, aborting"));
        return;
    }

    // Extract to raw arrays
    TArray<FVector> Vertices; Vertices.Reserve(MergedMesh.VertexCount());
    TArray<int32> Indices; Indices.Reserve(MergedMesh.TriangleCount() * 3);

    for (int vid : MergedMesh.VertexIndicesItr())
    {
        FVector3d P = MergedMesh.GetVertex(vid);
        Vertices.Add(FVector(P.X, P.Y, P.Z));
    }
    for (int tid : MergedMesh.TriangleIndicesItr())
    {
        UE::Geometry::FIndex3i Tri = MergedMesh.GetTriangle(tid);
        Indices.Add(Tri.C);
        Indices.Add(Tri.B);
        Indices.Add(Tri.A);
    }

    UE_LOG(LogTemp, Warning, TEXT("[Merge] Extracted: Verts=%d Tris=%d"), Vertices.Num(), Indices.Num()/3);

    // Spawn merged actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return;

    FActorSpawnParameters Params;
    APatternMesh* MergedActor = World->SpawnActor<APatternMesh>(Params);
    if (!MergedActor) return;

#if WITH_EDITOR
    MergedActor->SetActorLabel(TEXT("MergedPatternMesh"));
#endif

    // Store the merged dynamic mesh and create procedural mesh section
    MergedActor->DynamicMesh = MoveTemp(MergedMesh);

    TArray<FVector> Normals; Normals.Init(FVector::UpVector, Vertices.Num());
    TArray<FVector2D> UV0; UV0.Init(FVector2D::ZeroVector, Vertices.Num());
    TArray<FLinearColor> VertexColors; VertexColors.Init(FLinearColor::White, Vertices.Num());
    TArray<FProcMeshTangent> Tangents; Tangents.Init(FProcMeshTangent(1,0,0), Vertices.Num());

    MergedActor->MeshComponent->CreateMeshSection_LinearColor(
        0, Vertices, Indices, Normals, UV0, VertexColors, Tangents, true
    );

    UE_LOG(LogTemp, Log, TEXT("Merged two meshes into '%s' (%d verts, %d tris)"),
        *MergedActor->GetActorLabel(), Vertices.Num(), Indices.Num() / 3);
}
