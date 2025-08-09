#include "Canvas/CanvasSewing.h"
#include "Canvas/CanvasMesh.h"
#include "Misc/MessageDialog.h"
#include "Canvas/CanvasPatternMerge.h"


void FCanvasSewing::ClearAllSeams()
{
	// SewingConstraints.Empty();
	SeamDefinitions.Empty();
	AllDefinedSeams.Empty();
	SeamClickState = ESeamClickState::None;
	AStartTarget = FClickTarget();
	AEndTarget = FClickTarget();
	BStartTarget = FClickTarget();
	BEndTarget = FClickTarget();
}

void FCanvasSewing::FinaliseSeamDefinitionByTargets(
	const FClickTarget& AStart,
	const FClickTarget& AEnd,
	const FClickTarget& BStart,
	const FClickTarget& BEnd,
	const FInterpCurve<FVector2D>& CurvePoints,
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	const TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors)
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

		
	// 1) Add logical seam for editor bookkeeping
	// This stores which shapes and edges you’re sewing in terms of 
	// 2D pattern data, like shape indices and point indices
	FSeamDefinition NewSeamDef;
	NewSeamDef.ShapeA = AStart.ShapeIndex;
	NewSeamDef.EdgeA.Start = AStart.PointIndex;
	NewSeamDef.EdgeA.End = AEnd.PointIndex;

	NewSeamDef.ShapeB = BStart.ShapeIndex;
	NewSeamDef.EdgeB.Start = BStart.PointIndex;
	NewSeamDef.EdgeB.End = BEnd.PointIndex;

	SeamDefinitions.Add(NewSeamDef);



	
	UE_LOG(LogTemp, Warning, TEXT("SpawnedPatternActors count: %d"), SpawnedPatternActors.Num());
	for (int i = 0; i < SpawnedPatternActors.Num(); i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%d] IsValid: %d"), i, SpawnedPatternActors[i].IsValid());
	}
	
	// 2) Build your FPatternSewingConstraint (runtime struct)
	// This stores how to sew those shapes in the actual 3D mesh
	FPatternSewingConstraint NewSeam;

	// Mesh pointers
	APatternMesh* MeshA = SpawnedPatternActors.IsValidIndex(AStart.ShapeIndex) ? SpawnedPatternActors[AStart.ShapeIndex].Get() : nullptr;
	APatternMesh* MeshB = SpawnedPatternActors.IsValidIndex(BStart.ShapeIndex) ? SpawnedPatternActors[BStart.ShapeIndex].Get() : nullptr;

	UE_LOG(LogTemp, Warning, TEXT("MeshA ptr: %s"), MeshA ? *MeshA->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("MeshA->MeshComponent ptr: %s"), MeshA && MeshA->MeshComponent ? *MeshA->MeshComponent->GetName() : TEXT("NULL"));

	
	NewSeam.MeshA = MeshA ? MeshA->MeshComponent : nullptr;
	NewSeam.MeshB = MeshB ? MeshB->MeshComponent : nullptr;

	// Look up vertex IDs from the mesh's stored mapping
	NewSeam.VertexIndexA = -1;
	NewSeam.VertexIndexB = -1;

	if (MeshA)
	{
		const TArray<int32>& Mapping = MeshA->GetPolyIndexToVID();
		if (Mapping.IsValidIndex(AStart.PointIndex))
			NewSeam.VertexIndexA = Mapping[AStart.PointIndex];
	}

	if (MeshB)
	{
		const TArray<int32>& Mapping = MeshB->GetPolyIndexToVID();
		if (Mapping.IsValidIndex(BStart.PointIndex))
			NewSeam.VertexIndexB = Mapping[BStart.PointIndex];
	}

	// Screen points for UI (2D points along seam)
	NewSeam.ScreenPointsA = PointsA;
	NewSeam.ScreenPointsB = PointsB;

	AllDefinedSeams.Add(NewSeam);
	UE_LOG(LogTemp, Warning, TEXT("AStart.ShapeIndex=%d, BStart.ShapeIndex=%d"), AStart.ShapeIndex, BStart.ShapeIndex);



	// print all stored seams
	UE_LOG(LogTemp, Log, TEXT("---- Stored Seams ----"));
	for (int32 i = 0; i < AllDefinedSeams.Num(); i++)
	{
		const FPatternSewingConstraint& Seam = AllDefinedSeams[i];
		UE_LOG(LogTemp, Log, TEXT("[%d] MeshA=%s, VertexA=%d | MeshB=%s, VertexB=%d"),
			i,
			Seam.MeshA ? *Seam.MeshA->GetName() : TEXT("NULL"),
			Seam.VertexIndexA,
			Seam.MeshB ? *Seam.MeshB->GetName() : TEXT("NULL"),
			Seam.VertexIndexB
		);
	}


	
	UE_LOG(LogTemp, Log, TEXT("Seam finalized between shapes %d and %d (points [%d,%d] and [%d,%d])"),
		NewSeamDef.ShapeA, NewSeamDef.ShapeB,
		NewSeamDef.EdgeA.Start, NewSeamDef.EdgeA.End,
		NewSeamDef.EdgeB.Start, NewSeamDef.EdgeB.End);
}

//  APatternMesh* ActorA = SpawnedPatternActors[AShapeIdx].Get();

// void FCanvasSewing::FinaliseSeamDefinitionByTargets(
// 	const FClickTarget& AStart,
// 	const FClickTarget& AEnd,
// 	const FClickTarget& BStart,
// 	const FClickTarget& BEnd,
// 	const FInterpCurve<FVector2D>& CurvePoints,
// 	const TArray<FInterpCurve<FVector2D>>& CompletedShapes)
// {
// 	const int32 NumSeamPoints = 10;
// 	TArray<FVector2D> PointsA, PointsB;
// 	
//
// 	auto GetPt = [&](const FClickTarget& T) {
// 		if (T.ShapeIndex == INDEX_NONE)
// 			return CurvePoints.Points[T.PointIndex].OutVal;
// 		else
// 			return CompletedShapes[T.ShapeIndex].Points[T.PointIndex].OutVal;
// 	};
//
// 	FVector2D A1 = GetPt(AStart), A2 = GetPt(AEnd);
// 	FVector2D B1 = GetPt(BStart), B2 = GetPt(BEnd);
//
// 	for (int32 i = 0; i < NumSeamPoints; ++i)
// 	{
// 		float Alpha = float(i) / (NumSeamPoints - 1);
// 		PointsA.Add(FMath::Lerp(A1, A2, Alpha));
// 		PointsB.Add(FMath::Lerp(B1, B2, Alpha));
// 	}
//
// 	FPatternSewingConstraint NewSeam;
// 	NewSeam.ScreenPointsA = PointsA;
// 	NewSeam.ScreenPointsB = PointsB;
// 	AllDefinedSeams.Add(NewSeam);
//
// 	UE_LOG(LogTemp, Log, TEXT("Seam finalized between [%d,%d] and [%d,%d]"),
// 		AStart.ShapeIndex, AStart.PointIndex,
// 		BStart.ShapeIndex, BStart.PointIndex);
// }



//
// void FCanvasSewing::AlignSeamMeshes(
// 	APatternMesh* MeshActorA,
// 	APatternMesh* MeshActorB)
// {
// 	if (!MeshActorA || !MeshActorB) return;
//
// 	// Access the stored vertex IDs and dynamic meshes:
// 	const TArray<int32>& IDsA = MeshActorA->LastSeamVertexIDs;
// 	const TArray<int32>& IDsB = MeshActorB->LastSeamVertexIDs;
// 	UE_LOG(LogTemp, Warning, TEXT("MeshActorA = %s, MeshActorB = %s"), *MeshActorA->GetName(), *MeshActorB->GetName());
//
// 	// 1) Guard against empty or mismatched arrays
// 	int32 NumA = IDsA.Num();
// 	int32 NumB = IDsB.Num();
// 	if (NumA == 0 || NumB == 0)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Cannot align: one of the seam lists is empty (A=%d, B=%d)"), NumA, NumB);
// 		return;
// 	}
// 	if (NumA != NumB)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Cannot align: seam lists length mismatch (A=%d, B=%d)"), NumA, NumB);
// 		return;
// 	}
//
// 	// 2) Fetch world positions
// 	TArray<FVector> WorldA, WorldB;
// 	WorldA.Reserve(NumA);
// 	WorldB.Reserve(NumA);
//
// 	// for (int i = 0; i < NumA; ++i)
// 	// {
// 	// 	FVector LocalA = FVector(MeshActorA->DynamicMesh.GetVertex(IDsA[i]));
// 	// 	FVector LocalB = FVector(MeshActorB->DynamicMesh.GetVertex(IDsB[i]));
// 	// 	WorldA.Add( MeshActorA->GetActorTransform().TransformPosition(LocalA) );
// 	// 	WorldB.Add( MeshActorB->GetActorTransform().TransformPosition(LocalB) );
// 	// }
// 	for (int i = 0; i < IDsA.Num(); ++i)
// 	{
// 		int vidA = IDsA[i];
// 		int vidB = IDsB[i];
// 		// validate in-range
// 		if (vidA < 0 || vidA >= MeshActorA->DynamicMesh.VertexCount()) { /* bail/log */ }
// 		if (vidB < 0 || vidB >= MeshActorB->DynamicMesh.VertexCount()) { /* bail/log */ }
//
// 		FVector3d pA3 = MeshActorA->DynamicMesh.GetVertex(vidA);
// 		FVector3d pB3 = MeshActorB->DynamicMesh.GetVertex(vidB);
//
// 		FVector worldA = MeshActorA->GetActorTransform().TransformPosition(FVector(pA3.X,pA3.Y,pA3.Z));
// 		FVector worldB = MeshActorB->GetActorTransform().TransformPosition(FVector(pB3.X,pB3.Y,pB3.Z));
//
// 		WorldA.Add(worldA);
// 		WorldB.Add(worldB);
// 	}
//
// 	// 3) Compute average offset safely
// 	FVector TotalOffset = FVector::ZeroVector;
// 	for (int i = 0; i < NumA; ++i)
// 	{
// 		TotalOffset += (WorldA[i] - WorldB[i]);
// 	}
//
// 	// Now we know NumA > 0, so this is safe:
// 	FVector AverageOffset = TotalOffset / float(NumA);
//
// 	// 4) Apply the translation to MeshActorB
// 	FTransform NewTransform = MeshActorB->GetActorTransform();
// 	NewTransform.AddToTranslation(AverageOffset);
// 	MeshActorB->SetActorTransform(NewTransform);
//
// 	UE_LOG(LogTemp, Log, TEXT("Aligned MeshB by %s"), *AverageOffset.ToString());
// }

void FCanvasSewing::AlignSeamMeshes(APatternMesh* MeshActorA, APatternMesh* MeshActorB)
{
    if (!MeshActorA || !MeshActorB) return;

    const TArray<int32>& IDsA = MeshActorA->LastSeamVertexIDs;
    const TArray<int32>& IDsB = MeshActorB->LastSeamVertexIDs;
    int32 NumA = IDsA.Num(), NumB = IDsB.Num();
    if (NumA == 0 || NumB == 0) {
        UE_LOG(LogTemp, Warning, TEXT("Cannot align: one of the seam lists is empty (A=%d, B=%d)"), NumA, NumB);
        return;
    }
    if (NumA != NumB) {
        UE_LOG(LogTemp, Warning, TEXT("Warning: mismatched seam lengths (A=%d, B=%d) — proceeding with min length."), NumA, NumB);
    }
    int32 N = FMath::Min(NumA, NumB);

    // Build world-space point lists
    FVector TotalOffset = FVector::ZeroVector;
    TArray<FVector> WorldA; WorldA.Reserve(N);
    TArray<FVector> WorldB; WorldB.Reserve(N);
    for (int i = 0; i < N; ++i)
    {
        int vidA = IDsA[i];
        int vidB = IDsB[i];
        if (vidA < 0 || vidA >= MeshActorA->DynamicMesh.VertexCount()) continue;
        if (vidB < 0 || vidB >= MeshActorB->DynamicMesh.VertexCount()) continue;

        FVector3d pA3 = MeshActorA->DynamicMesh.GetVertex(vidA);
        FVector3d pB3 = MeshActorB->DynamicMesh.GetVertex(vidB);

        FVector worldA = MeshActorA->GetActorTransform().TransformPosition(FVector(pA3.X,pA3.Y,pA3.Z));
        FVector worldB = MeshActorB->GetActorTransform().TransformPosition(FVector(pB3.X,pB3.Y,pB3.Z));

        WorldA.Add(worldA);
        WorldB.Add(worldB);
        TotalOffset += (worldA - worldB);
    }

    if (WorldA.Num() == 0) {
        UE_LOG(LogTemp, Warning, TEXT("No valid seam points to align."));
        return;
    }

    FVector AverageOffset = TotalOffset / float(WorldA.Num());

    // Move both actors half the offset (symmetric)
    FVector Half = AverageOffset * 0.5f;

    FTransform TA = MeshActorA->GetActorTransform();
    TA.AddToTranslation(-Half);   // move A half toward B (so they meet halfway)
    MeshActorA->SetActorTransform(TA);

    FTransform TB = MeshActorB->GetActorTransform();
    TB.AddToTranslation(Half);    // move B half toward A
    MeshActorB->SetActorTransform(TB);

    UE_LOG(LogTemp, Log, TEXT("Aligned meshes by splitting offset: %s (half=%s)"),
           *AverageOffset.ToString(), *Half.ToString());
}


//
// void FCanvasSewing::BuildAndAlignClickedSeam(
//     const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
//     const FInterpCurve<FVector2D>& CurvePoints)
// {
//     // Don't destroy existing actors (we want to use the already-spawned ones)
//     // helper: fetch 2D point from ClickTarget
//     auto Get2DPoint = [&](const FClickTarget& T) -> FVector2D {
//         if (T.ShapeIndex == INDEX_NONE) return CurvePoints.Points[T.PointIndex].OutVal;
//         return CompletedShapes[T.ShapeIndex].Points[T.PointIndex].OutVal;
//     };
//
//     // Sample seam 2D points (same as FinalizeSeamDefinitionByTargets)
//     const int32 NumSeamSamples = 10;
//     auto SampleSegment2D = [&](const FVector2D& A, const FVector2D& B, TArray<FVector2D>& Out) {
//         Out.Reset(); Out.Reserve(NumSeamSamples);
//         for (int i = 0; i < NumSeamSamples; ++i) {
//             float Alpha = float(i) / float(NumSeamSamples - 1);
//             Out.Add(FMath::Lerp(A, B, Alpha));
//         }
//     };
// 	
// 	// Make sure the targets are set and valid
// 	if (AStartTarget.PointIndex == INDEX_NONE || AEndTarget.PointIndex == INDEX_NONE ||
// 		BStartTarget.PointIndex == INDEX_NONE || BEndTarget.PointIndex == INDEX_NONE)
// 	{
// 		FText Msg = FText::FromString(TEXT("One or more seam targets are undefined. Please finish creating seams first."));
// 		FMessageDialog::Open(EAppMsgType::Ok, Msg);
// 		UE_LOG(LogTemp, Warning, TEXT("Cannot build seam: One or more seam targets are undefined."));
// 		return;
// 	}
// 	
//     FVector2D A1 = Get2DPoint(AStartTarget);
//     FVector2D A2 = Get2DPoint(AEndTarget);
//     FVector2D B1 = Get2DPoint(BStartTarget);
//     FVector2D B2 = Get2DPoint(BEndTarget);
//
//     TArray<FVector2D> SeamA2D, SeamB2D;
//     SampleSegment2D(A1, A2, SeamA2D);
//     SampleSegment2D(B1, B2, SeamB2D);
//
//     // Find the actors (we assume SpawnedPatternActors indexes correspond to CompletedShapes)
//     int32 AShapeIdx = AStartTarget.ShapeIndex;
//     int32 BShapeIdx = BStartTarget.ShapeIndex;
// 	
//     if (AShapeIdx == INDEX_NONE || BShapeIdx == INDEX_NONE) {
//         // UE_LOG(LogTemp, Warning, TEXT("Seam targets reference the curvePoints or invalid shape index; handle separately if needed"));
//     	FText Msg = FText::FromString(TEXT("Cannot sew an in-progress shape. Please finish the shape first."));
//     	FMessageDialog::Open(EAppMsgType::Ok, Msg);
//         return;
//     }
// 	
//     if (!SpawnedPatternActors.IsValidIndex(AShapeIdx) || !SpawnedPatternActors.IsValidIndex(BShapeIdx)) {
//     	FText Msg = FText::FromString(TEXT("No valid actors. Please generate meshes first."));
//     	FMessageDialog::Open(EAppMsgType::Ok, Msg);
//     	UE_LOG(LogTemp, Warning, TEXT("No spawned actor found for given shape index."));
//         return;
//     }
// 	
//     APatternMesh* ActorA = SpawnedPatternActors[AShapeIdx].Get();
//     APatternMesh* ActorB = SpawnedPatternActors[BShapeIdx].Get();
//     if (!ActorA || !ActorB) {
//     	FText Msg = FText::FromString(TEXT("Missing meshes. Please generate meshes first."));
//     	FMessageDialog::Open(EAppMsgType::Ok, Msg);
//         UE_LOG(LogTemp, Warning, TEXT("Spawned actor invalid."));
//         return;
//     }
//
//     // Helper: map a set of seam 2D points -> an array of VIDs by nearest boundary sample on the actor
//     auto MapSeam2DToVIDs = [&](APatternMesh* Actor, const TArray<FVector2D>& Seam2D, TArray<int32>& OutVIDs) {
//         OutVIDs.Reset();
//         int32 NumBoundary = Actor->BoundarySamplePoints2D.Num();
//         if (NumBoundary == 0) {
//             UE_LOG(LogTemp, Warning, TEXT("Actor %s has no boundary samples."), *Actor->GetName());
//             return;
//         }
//         for (const FVector2D& Q : Seam2D) {
//             int BestIdx = INDEX_NONE;
//             float BestDist2 = FLT_MAX;
//             // BoundarySamplePoints2D are stored as FVector2f (x,y)
//             for (int i = 0; i < NumBoundary; ++i) {
//                 const FVector2f& S = Actor->BoundarySamplePoints2D[i];
//                 float dx = S.X - (float)Q.X;
//                 float dy = S.Y - (float)Q.Y;
//                 float d2 = dx*dx + dy*dy;
//                 if (d2 < BestDist2) { BestDist2 = d2; BestIdx = i; }
//             }
//             int32 VID = INDEX_NONE;
//             if (BestIdx != INDEX_NONE && Actor->BoundarySampleVertexIDs.IsValidIndex(BestIdx))
//                 VID = Actor->BoundarySampleVertexIDs[BestIdx];
//             OutVIDs.Add(VID);
//         }
//     };
//
//     // Map seam 2D samples to VIDs
//     TArray<int32> VIDsA, VIDsB;
//     MapSeam2DToVIDs(ActorA, SeamA2D, VIDsA);
//     MapSeam2DToVIDs(ActorB, SeamB2D, VIDsB);
//
//     // Filter out invalid VIDs and keep pairs (we only keep positions where both are valid)
//     TArray<int32> PairedA, PairedB;
//     int32 PairCount = FMath::Min(VIDsA.Num(), VIDsB.Num());
//     for (int i = 0; i < PairCount; ++i) {
//         int32 a = VIDsA[i];
//         int32 b = VIDsB[i];
//         bool aValid = (a != INDEX_NONE) && (a >= 0 && a < ActorA->DynamicMesh.VertexCount());
//         bool bValid = (b != INDEX_NONE) && (b >= 0 && b < ActorB->DynamicMesh.VertexCount());
//         if (aValid && bValid) {
//             PairedA.Add(a);
//             PairedB.Add(b);
//         }
//     }
//
//     if (PairedA.Num() == 0 || PairedB.Num() == 0) {
//         UE_LOG(LogTemp, Warning, TEXT("No valid paired seam vertices found (A=%d, B=%d)"), PairedA.Num(), PairedB.Num());
//         return;
//     }
//
//     // Orientation check: which ordering produces smaller average distance? If reversed B is better, reverse it.
//     auto BuildWorldPositions = [&](APatternMesh* Actor, const TArray<int32>& VIDs, TArray<FVector>& OutPos) {
//         OutPos.Reset(); OutPos.Reserve(VIDs.Num());
//         for (int id : VIDs) {
//             FVector3d p3d = Actor->DynamicMesh.GetVertex(id);
//             OutPos.Add(Actor->GetActorTransform().TransformPosition(FVector(p3d.X, p3d.Y, p3d.Z)));
//         }
//     };
//
//     TArray<FVector> WorldA, WorldB;
//     BuildWorldPositions(ActorA, PairedA, WorldA);
//     BuildWorldPositions(ActorB, PairedB, WorldB);
//
//     // compute avg distance for normal and reversed B
//     auto AverageDistance = [&](const TArray<FVector>& X, const TArray<FVector>& Y) -> double {
//         int n = FMath::Min(X.Num(), Y.Num());
//         if (n == 0) return DBL_MAX;
//         double sum = 0.0;
//         for (int i = 0; i < n; ++i) sum += (X[i] - Y[i]).Size();
//         return sum / double(n);
//     };
//
//     double avgNormal = AverageDistance(WorldA, WorldB);
//
//     // build reversed WorldB and compute distance
//     TArray<FVector> WorldBRev = WorldB;
//     Algo::Reverse(WorldBRev); // requires #include "Algo/Reverse.h"
//     double avgReversed = AverageDistance(WorldA, WorldBRev);
//
//     if (avgReversed + KINDA_SMALL_NUMBER < avgNormal) { // reversed matches better; reverse PairedB
//         Algo::Reverse(PairedB);
//         UE_LOG(LogTemp, Log, TEXT("Reversed B seam ordering to match A (avgNormal=%f avgRev=%f)"), avgNormal, avgReversed);
//     }
//
//     // Final lengths should now be equal (or close). Store into actors and align.
//     ActorA->LastSeamVertexIDs = PairedA;
//     ActorB->LastSeamVertexIDs = PairedB;
//
//     UE_LOG(LogTemp, Log, TEXT("Seam prepared: A=%d verts, B=%d verts"), ActorA->LastSeamVertexIDs.Num(), ActorB->LastSeamVertexIDs.Num());
//
// 	
//     AlignSeamMeshes(ActorA, ActorB);
// }

void FCanvasSewing::BuildAndAlignSeam(
	const FPatternSewingConstraint& Seam,
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	const FInterpCurve<FVector2D>& CurvePoints)
{
	// --- helper: find the APatternMesh that owns this procedural mesh component ---
	auto FindActorForMesh = [&](UProceduralMeshComponent* MeshComp) -> APatternMesh*
	{
		if (!MeshComp) return nullptr;
		for (const TWeakObjectPtr<APatternMesh>& Weak : SpawnedPatternActors)
		{
			if (APatternMesh* Actor = Weak.Get())
			{
				if (Actor->MeshComponent == MeshComp) // adapt if your member has a different name
				{
					return Actor;
				}
			}
		}
		return nullptr;
	};
	
	// --- validate seam screen points ---
	if (Seam.ScreenPointsA.Num() < 2 || Seam.ScreenPointsB.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot build seam: stored seam does not have enough screen points (A=%d, B=%d)"),
			Seam.ScreenPointsA.Num(), Seam.ScreenPointsB.Num());
		return;
	}

	// Use first and last screen points as endpoints (A1,A2,B1,B2)
	FVector2D A1 = Seam.ScreenPointsA[0];
	FVector2D A2 = Seam.ScreenPointsA.Last();
	FVector2D B1 = Seam.ScreenPointsB[0];
	FVector2D B2 = Seam.ScreenPointsB.Last();


    // Sample seam 2D points (same as FinalizeSeamDefinitionByTargets)
    const int32 NumSeamSamples = 10;
    auto SampleSegment2D = [&](const FVector2D& A, const FVector2D& B, TArray<FVector2D>& Out)
	{
        Out.Reset(); Out.Reserve(NumSeamSamples);
        for (int i = 0; i < NumSeamSamples; ++i)
        {
            float Alpha = float(i) / float(NumSeamSamples - 1);
            Out.Add(FMath::Lerp(A, B, Alpha));
        }
    };
	
	// // Make sure the targets are set and valid
	// if (Seam.VertexIndexA == INDEX_NONE || AEndTarget.PointIndex == INDEX_NONE ||
	// 	BStartTarget.PointIndex == INDEX_NONE || BEndTarget.PointIndex == INDEX_NONE)
	// {
	// 	FText Msg = FText::FromString(TEXT("One or more seam targets are undefined. Please finish creating seams first."));
	// 	FMessageDialog::Open(EAppMsgType::Ok, Msg);
	// 	UE_LOG(LogTemp, Warning, TEXT("Cannot build seam: One or more seam targets are undefined."));
	// 	return;
	// }

	// --- VALIDATION: make sure the seam has the data it needs ---
	// 1) Screen points already checked above (we require at least 2 each).
	// 2) Mesh pointers must be present
	if (!Seam.MeshA || !Seam.MeshB)
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildAndAlignSeam: seam has null mesh pointers (MeshA=%s MeshB=%s)"),
			Seam.MeshA ? *Seam.MeshA->GetName() : TEXT("NULL"),
			Seam.MeshB ? *Seam.MeshB->GetName() : TEXT("NULL"));
		// Don't show a modal dialog here — just skip this seam
		return;
	}
	

	
    TArray<FVector2D> SeamA2D, SeamB2D;
    SampleSegment2D(A1, A2, SeamA2D);
    SampleSegment2D(B1, B2, SeamB2D);

	// 3) Find owning actors for the seam meshes
	APatternMesh* ActorA = FindActorForMesh(Seam.MeshA);
	APatternMesh* ActorB = FindActorForMesh(Seam.MeshB);
	
	if (!ActorA || !ActorB)
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildAndAlignSeam: could not find spawned actors for seam meshes (A=%s, B=%s)"),
			Seam.MeshA ? *Seam.MeshA->GetName() : TEXT("NULL"),
			Seam.MeshB ? *Seam.MeshB->GetName() : TEXT("NULL"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Cannot sew: one or more associated mesh actors are missing. Generate meshes first.")));
		// Skip this seam (actor missing or not spawned)
		return;
	}
	
	// 4) Make sure the actors have boundary samples ready (MapSeam2DToVIDs relies on them)
	if (ActorA->BoundarySamplePoints2D.Num() == 0 || ActorA->BoundarySampleVertexIDs.Num() == 0 ||
		ActorB->BoundarySamplePoints2D.Num() == 0 || ActorB->BoundarySampleVertexIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildAndAlignSeam: actor(s) missing boundary samples. ActorA=%s (b=%d,ids=%d) ActorB=%s (b=%d,ids=%d)"),
			*ActorA->GetName(), ActorA->BoundarySamplePoints2D.Num(), ActorA->BoundarySampleVertexIDs.Num(),
			*ActorB->GetName(), ActorB->BoundarySamplePoints2D.Num(), ActorB->BoundarySampleVertexIDs.Num());
		return;
	}
	
    // Helper: map a set of seam 2D points -> an array of VIDs by nearest boundary sample on the actor
    auto MapSeam2DToVIDs = [&](APatternMesh* Actor, const TArray<FVector2D>& Seam2D, TArray<int32>& OutVIDs) {
        OutVIDs.Reset();
        int32 NumBoundary = Actor->BoundarySamplePoints2D.Num();
        if (NumBoundary == 0) {
            UE_LOG(LogTemp, Warning, TEXT("Actor %s has no boundary samples."), *Actor->GetName());
            return;
        }
        for (const FVector2D& Q : Seam2D) {
            int BestIdx = INDEX_NONE;
            float BestDist2 = FLT_MAX;
            // BoundarySamplePoints2D are stored as FVector2f (x,y)
            for (int i = 0; i < NumBoundary; ++i) {
                const FVector2f& S = Actor->BoundarySamplePoints2D[i];
                float dx = S.X - (float)Q.X;
                float dy = S.Y - (float)Q.Y;
                float d2 = dx*dx + dy*dy;
                if (d2 < BestDist2) { BestDist2 = d2; BestIdx = i; }
            }
            int32 VID = INDEX_NONE;
            if (BestIdx != INDEX_NONE && Actor->BoundarySampleVertexIDs.IsValidIndex(BestIdx))
                VID = Actor->BoundarySampleVertexIDs[BestIdx];
            OutVIDs.Add(VID);
        }
    };

    // Map seam 2D samples to VIDs
    TArray<int32> VIDsA, VIDsB;
    MapSeam2DToVIDs(ActorA, SeamA2D, VIDsA);
    MapSeam2DToVIDs(ActorB, SeamB2D, VIDsB);

    // Filter out invalid VIDs and keep pairs (we only keep positions where both are valid)
    TArray<int32> PairedA, PairedB;
    int32 PairCount = FMath::Min(VIDsA.Num(), VIDsB.Num());
    for (int i = 0; i < PairCount; ++i) {
        int32 a = VIDsA[i];
        int32 b = VIDsB[i];
        bool aValid = (a != INDEX_NONE) && (a >= 0 && a < ActorA->DynamicMesh.VertexCount());
        bool bValid = (b != INDEX_NONE) && (b >= 0 && b < ActorB->DynamicMesh.VertexCount());
        if (aValid && bValid) {
            PairedA.Add(a);
            PairedB.Add(b);
        }
    }

    if (PairedA.Num() == 0 || PairedB.Num() == 0) {
        UE_LOG(LogTemp, Warning, TEXT("No valid paired seam vertices found (A=%d, B=%d)"), PairedA.Num(), PairedB.Num());
        return;
    }

	// Build world positions from vertex ids
	auto BuildWorldPositions = [&](APatternMesh* Actor, const TArray<int32>& VIDs, TArray<FVector>& OutPos) {
		OutPos.Reset(); OutPos.Reserve(VIDs.Num());
		for (int id : VIDs)
		{
			// Defensive check (shouldn't be necessary if we validated above)
			if (id < 0 || id >= Actor->DynamicMesh.VertexCount()) continue;
			FVector3d p3d = Actor->DynamicMesh.GetVertex(id);
			OutPos.Add(Actor->GetActorTransform().TransformPosition(FVector(p3d.X, p3d.Y, p3d.Z)));
		}
    };

    TArray<FVector> WorldA, WorldB;
    BuildWorldPositions(ActorA, PairedA, WorldA);
    BuildWorldPositions(ActorB, PairedB, WorldB);

    // compute avg distance for normal and reversed B
    auto AverageDistance = [&](const TArray<FVector>& X, const TArray<FVector>& Y) -> double {
        int n = FMath::Min(X.Num(), Y.Num());
        if (n == 0) return DBL_MAX;
        double sum = 0.0;
        for (int i = 0; i < n; ++i) sum += (X[i] - Y[i]).Size();
        return sum / double(n);
    };

    double avgNormal = AverageDistance(WorldA, WorldB);

    // build reversed WorldB and compute distance
    TArray<FVector> WorldBRev = WorldB;
    Algo::Reverse(WorldBRev); // requires #include "Algo/Reverse.h"
    double avgReversed = AverageDistance(WorldA, WorldBRev);

    if (avgReversed + KINDA_SMALL_NUMBER < avgNormal) { // reversed matches better; reverse PairedB
        Algo::Reverse(PairedB);
        UE_LOG(LogTemp, Log, TEXT("Reversed B seam ordering to match A (avgNormal=%f avgRev=%f)"), avgNormal, avgReversed);
    }

    // Final lengths should now be equal (or close). Store into actors and align.
    ActorA->LastSeamVertexIDs = PairedA;
    ActorB->LastSeamVertexIDs = PairedB;

    UE_LOG(LogTemp, Log, TEXT("Seam prepared: A=%d verts, B=%d verts"), ActorA->LastSeamVertexIDs.Num(), ActorB->LastSeamVertexIDs.Num());

	
    AlignSeamMeshes(ActorA, ActorB);
}



void FCanvasSewing::BuildAndAlignAllSeams(
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	const FInterpCurve<FVector2D>& CurvePoints)
{
	for (const FPatternSewingConstraint& Seam : AllDefinedSeams)
	{
		BuildAndAlignSeam(Seam, CompletedShapes, CurvePoints);
	}
}

void FCanvasSewing::MergeSewnGroups()
{
	FCanvasPatternMerge Merge(SpawnedPatternActors, AllDefinedSeams);
	Merge.MergeSewnGroups();
}





// void FCanvasSewing::BuildAndAlignClickedSeam(
//         const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
//         const FInterpCurve<FVector2D>& CurvePoints)
// {
//     // don't destroy existing spawned actors here — we want to reuse existing actors.
//     // assume SpawnedPatternActors already contains the actors in the same order as CompletedShapes
//     // (you must ensure that when meshes were generated earlier).
//
//     // helper to get 2D point from a ClickTarget
//     auto Get2DPoint = [&](const FClickTarget& T) -> FVector2D {
//         if (T.ShapeIndex == INDEX_NONE) return CurvePoints.Points[T.PointIndex].OutVal;
//         return CompletedShapes[T.ShapeIndex].Points[T.PointIndex].OutVal;
//     };
//
//     FVector2D A1 = Get2DPoint(AStartTarget);
//     FVector2D A2 = Get2DPoint(AEndTarget);
//     FVector2D B1 = Get2DPoint(BStartTarget);
//     FVector2D B2 = Get2DPoint(BEndTarget);
//
//     const int32 NumSeamSamples = 10;
//     auto SampleSegment = [&](const FVector2D& P0, const FVector2D& P1, TArray<FVector2D>& Out){
//         Out.Reset(); Out.Reserve(NumSeamSamples);
//         for (int i = 0; i < NumSeamSamples; ++i)
//         {
//             float Alpha = float(i) / float(NumSeamSamples - 1);
//             Out.Add( FMath::Lerp(P0, P1, Alpha) );
//         }
//     };
//
//     TArray<FVector2D> SeamA2D, SeamB2D;
//     SampleSegment(A1, A2, SeamA2D);
//     SampleSegment(B1, B2, SeamB2D);
//
//     // find actors: assume shapeIndex maps directly into SpawnedPatternActors
//     int32 AShapeIdx = AStartTarget.ShapeIndex;
//     int32 BShapeIdx = BStartTarget.ShapeIndex;
//
//     if (AShapeIdx == INDEX_NONE || BShapeIdx == INDEX_NONE)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("Canvas-sewing: clicked seam refers to curvePoints or unsupported actor mapping"));
//         // handle curvePoints case if you spawned an actor for curvePoints as well
//         return;
//     }
//
//     if (!SpawnedPatternActors.IsValidIndex(AShapeIdx) || !SpawnedPatternActors.IsValidIndex(BShapeIdx))
//     {
//         UE_LOG(LogTemp, Warning, TEXT("No spawned actor found for given shape index."));
//         return;
//     }
//
//     APatternMesh* ActorA = SpawnedPatternActors[AShapeIdx].Get();
//     APatternMesh* ActorB = SpawnedPatternActors[BShapeIdx].Get();
//     if (!ActorA || !ActorB)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("Spawned actor invalid."));
//         return;
//     }
//
//     // helper: find nearest boundary sample index for a given 2D point
//     auto FindNearestBoundaryIndex = [&](APatternMesh* Actor, const FVector2D& Query)->int32
//     {
//         int32 Best = INDEX_NONE;
//         float BestDistSqr = FLT_MAX;
//         for (int i = 0; i < Actor->BoundarySamplePoints2D.Num(); ++i)
//         {
//             const FVector2f& S = Actor->BoundarySamplePoints2D[i];
//             float dx = S.X - Query.X;
//             float dy = S.Y - Query.Y;
//             float d2 = dx*dx + dy*dy;
//             if (d2 < BestDistSqr) { BestDistSqr = d2; Best = i; }
//         }
//         return Best;
//     };
//
//     // find start & end indices on each boundary
//     int AstartIdx = FindNearestBoundaryIndex(ActorA, FVector2D(SeamA2D[0]));
//     int AendIdx   = FindNearestBoundaryIndex(ActorA, FVector2D(SeamA2D.Last()));
//     int BstartIdx = FindNearestBoundaryIndex(ActorB, FVector2D(SeamB2D[0]));
//     int BendIdx   = FindNearestBoundaryIndex(ActorB, FVector2D(SeamB2D.Last()));
//
//     if (AstartIdx == INDEX_NONE || AendIdx == INDEX_NONE || BstartIdx == INDEX_NONE || BendIdx == INDEX_NONE)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("Failed to find nearest boundary sample for one of the seam endpoints."));
//         return;
//     }
//
//     // helper: make contiguous list between two indices (wrap-around allowed)
//     auto MakeIndexSequence = [&](int32 Start, int32 End, int32 N, TArray<int32>& OutSeq)
//     {
//         OutSeq.Reset();
//         if (Start <= End)
//         {
//             for (int i = Start; i <= End; ++i) OutSeq.Add(i);
//         }
//         else
//         {
//             for (int i = Start; i < N; ++i) OutSeq.Add(i);
//             for (int i = 0; i <= End; ++i) OutSeq.Add(i);
//         }
//     };
//
//     // build vertex-ID sequences using actor->BoundarySampleVertexIDs
//     TArray<int32> SeamVIDsA_idx, SeamVIDsB_idx;
//     MakeIndexSequence(AstartIdx, AendIdx, ActorA->BoundarySampleVertexIDs.Num(), SeamVIDsA_idx);
//     MakeIndexSequence(BstartIdx, BendIdx, ActorB->BoundarySampleVertexIDs.Num(), SeamVIDsB_idx);
//
//     // map index -> actual VID
//     TArray<int32> SeamVIDsA, SeamVIDsB;
//     SeamVIDsA.Reserve(SeamVIDsA_idx.Num());
//     for (int idx : SeamVIDsA_idx)
//     {
//         SeamVIDsA.Add( ActorA->BoundarySampleVertexIDs.IsValidIndex(idx) ? ActorA->BoundarySampleVertexIDs[idx] : INDEX_NONE );
//     }
//     SeamVIDsB.Reserve(SeamVIDsB_idx.Num());
//     for (int idx : SeamVIDsB_idx)
//     {
//         SeamVIDsB.Add( ActorB->BoundarySampleVertexIDs.IsValidIndex(idx) ? ActorB->BoundarySampleVertexIDs[idx] : INDEX_NONE );
//     }
//
//     // Option A (minimal change): store them on the actors and call existing AlignSeamMeshes
//     ActorA->LastSeamVertexIDs = SeamVIDsA;
//     ActorB->LastSeamVertexIDs = SeamVIDsB;
//
//     // Align using your existing method
//     AlignSeamMeshes(ActorA, ActorB);
// }




//
// void FCanvasSewing::BuildAndAlignClickedSeam(
// 		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
// 		const FInterpCurve<FVector2D>& CurvePoints)
// {
// 	// 1) Clear old actors
// 	for (auto& Weak : SpawnedPatternActors)
// 		if (auto* A = Weak.Get()) A->Destroy();
// 	SpawnedPatternActors.Empty();
//
// 	// Arrays to hold the seam IDs for A and B
// 	TArray<int32> SeamVertsA;
// 	TArray<int32> SeamVertsB;
// 	
// 	// 2) Build mesh and spawn actor for first shape
// 	{
// 		int32 ShapeIdx = AStartTarget.ShapeIndex;
// 		const FInterpCurve<FVector2D>& Shape = (ShapeIdx == INDEX_NONE) ? CurvePoints : CompletedShapes[ShapeIdx];
// 		// CanvasMesh::TriangulateAndBuildMesh(Shape, /*bRecordSeam=*/true, AStartTarget.PointIndex, AEndTarget.PointIndex);
// 		FDynamicMesh3 Mesh;
// 		// TArray<int32> SeamVerts;
// 		TArray<int32> DummySeamVerts;
// 	
// 		CanvasMesh::TriangulateAndBuildMesh(
// 			Shape,
// 			true,
// 			AStartTarget.PointIndex, AEndTarget.PointIndex,
// 			SeamVertsA, //SeamVerts,
// 			Mesh,
// 			DummySeamVerts,
// 			SpawnedPatternActors
// 			);
// 	}
// 	
// 	
// 	// 3) Build mesh and spawn actor for second shape
// 	{
// 		int32 ShapeIdx = BStartTarget.ShapeIndex;
// 		const FInterpCurve<FVector2D>& Shape = (ShapeIdx == INDEX_NONE) ? CurvePoints : CompletedShapes[ShapeIdx];
// 		// CanvasMesh::TriangulateAndBuildMesh(Shape, /*bRecordSeam=*/true, BStartTarget.PointIndex, BEndTarget.PointIndex);
// 		FDynamicMesh3 Mesh;
// 		// TArray<int32> SeamVerts;
// 		TArray<int32> DummySeamVerts;
// 	
// 		CanvasMesh::TriangulateAndBuildMesh(
// 			Shape,
// 			true,
// 			BStartTarget.PointIndex, BEndTarget.PointIndex,
// 			SeamVertsB, //SeamVerts,
// 			Mesh,
// 			DummySeamVerts,
// 			SpawnedPatternActors
// 			);
// 	}
// 	
//
// 	// Now check the weak array
// 	UE_LOG(LogTemp, Log, TEXT("SpawnedPatternActors.Num() = %d"), SpawnedPatternActors.Num());
//
// 	if (SpawnedPatternActors.Num() >= 2)
// 	{
// 		APatternMesh* A = SpawnedPatternActors[0].Get();
// 		APatternMesh* B = SpawnedPatternActors[1].Get();
// 		if (A && B)
// 		{
// 			AlignSeamMeshes(A, B);
// 		}
// 		else
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("Spawned actors exist but were garbage-collected or invalid"));
// 		}
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Expected 2 SpawnedPatternActors, got %d"), SpawnedPatternActors.Num());
// 	}
// 	
// }



// void FCanvasSewing::MergeLastTwoMeshes()
// {
//     using namespace CanvasMesh;
//
//     if (SpawnedPatternActors.Num() < 2)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("Need at least two meshes to merge"));
//         return;
//     }
//
//     TWeakObjectPtr<APatternMesh> AWeak = SpawnedPatternActors[SpawnedPatternActors.Num() - 2];
//     TWeakObjectPtr<APatternMesh> BWeak = SpawnedPatternActors[SpawnedPatternActors.Num() - 1];
//
//     APatternMesh* A = AWeak.Get();
//     APatternMesh* B = BWeak.Get();
//
//     if (!A || !B)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("One of the two actors is invalid"));
//         return;
//     }
//
//     UE_LOG(LogTemp, Warning, TEXT("[Merge] Source A: verts=%d tris=%d"), A->DynamicMesh.VertexCount(), A->DynamicMesh.TriangleCount());
//     UE_LOG(LogTemp, Warning, TEXT("[Merge] Source B: verts=%d tris=%d"), B->DynamicMesh.VertexCount(), B->DynamicMesh.TriangleCount());
//
//     // If either source has 0 triangles, bail (triangulation problem earlier)
//     if (A->DynamicMesh.TriangleCount() == 0 || B->DynamicMesh.TriangleCount() == 0)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("[Merge] One source has no triangles; aborting merge"));
//         return;
//     }
//
//     // Start with an empty merged mesh
//     UE::Geometry::FDynamicMesh3 MergedMesh;
//     // (Optional) Enable attributes if you need them later
//     // MergedMesh.EnableAttributes();
//
//     // Helper lambda to append one actor's mesh into MergedMesh
//     auto AppendMeshIntoMerged = [&](APatternMesh* Src, const FTransform& SrcTransform)
//     {
//         TMap<int, int> Remap; // mapping old vertex id -> new vertex id
//
//         // Append vertices
//         for (int vid : Src->DynamicMesh.VertexIndicesItr())
//         {
//             FVector3d LocalP = Src->DynamicMesh.GetVertex(vid);
//             FVector WorldP = SrcTransform.TransformPosition(FVector(LocalP.X, LocalP.Y, LocalP.Z));
//             int newVid = MergedMesh.AppendVertex(FVector3d(WorldP));
//             Remap.Add(vid, newVid);
//         }
//
//         // Append triangles using remapped indices
//         for (int tid : Src->DynamicMesh.TriangleIndicesItr())
//         {
//             UE::Geometry::FIndex3i T = Src->DynamicMesh.GetTriangle(tid);
//             int Aidx = Remap[T.A];
//             int Bidx = Remap[T.B];
//             int Cidx = Remap[T.C];
//             MergedMesh.AppendTriangle(Aidx, Bidx, Cidx);
//         }
//     };
//
//     // Append A then B
//     AppendMeshIntoMerged(A, A->GetActorTransform());
//     UE_LOG(LogTemp, Warning, TEXT("[Merge] After append-A: Verts=%d Tris=%d"), MergedMesh.VertexCount(), MergedMesh.TriangleCount());
//
//     AppendMeshIntoMerged(B, B->GetActorTransform());
//     UE_LOG(LogTemp, Warning, TEXT("[Merge] After append-B: Verts=%d Tris=%d"), MergedMesh.VertexCount(), MergedMesh.TriangleCount());
//
//     // If still no triangles, something upstream is wrong
//     if (MergedMesh.TriangleCount() == 0)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("[Merge] Merged mesh has 0 triangles, aborting"));
//         return;
//     }
//
//     // Extract to raw arrays
//     TArray<FVector> Vertices; Vertices.Reserve(MergedMesh.VertexCount());
//     TArray<int32> Indices; Indices.Reserve(MergedMesh.TriangleCount() * 3);
//
//     for (int vid : MergedMesh.VertexIndicesItr())
//     {
//         FVector3d P = MergedMesh.GetVertex(vid);
//         Vertices.Add(FVector(P.X, P.Y, P.Z));
//     }
//     for (int tid : MergedMesh.TriangleIndicesItr())
//     {
//         UE::Geometry::FIndex3i Tri = MergedMesh.GetTriangle(tid);
//         Indices.Add(Tri.C);
//         Indices.Add(Tri.B);
//         Indices.Add(Tri.A);
//     }
//
//     UE_LOG(LogTemp, Warning, TEXT("[Merge] Extracted: Verts=%d Tris=%d"), Vertices.Num(), Indices.Num()/3);
//
//     // Spawn merged actor
//     UWorld* World = GEditor->GetEditorWorldContext().World();
//     if (!World) return;
//
//     FActorSpawnParameters Params;
//     APatternMesh* MergedActor = World->SpawnActor<APatternMesh>(Params);
//     if (!MergedActor) return;
//
// #if WITH_EDITOR
//     MergedActor->SetActorLabel(TEXT("MergedPatternMesh"));
// #endif
//
//     // Store the merged dynamic mesh and create procedural mesh section
//     MergedActor->DynamicMesh = MoveTemp(MergedMesh);
//
//     TArray<FVector> Normals; Normals.Init(FVector::UpVector, Vertices.Num());
//     TArray<FVector2D> UV0; UV0.Init(FVector2D::ZeroVector, Vertices.Num());
//     TArray<FLinearColor> VertexColors; VertexColors.Init(FLinearColor::White, Vertices.Num());
//     TArray<FProcMeshTangent> Tangents; Tangents.Init(FProcMeshTangent(1,0,0), Vertices.Num());
//
//     MergedActor->MeshComponent->CreateMeshSection_LinearColor(
//         0, Vertices, Indices, Normals, UV0, VertexColors, Tangents, true
//     );
//
//     UE_LOG(LogTemp, Log, TEXT("Merged two meshes into '%s' (%d verts, %d tris)"),
//         *MergedActor->GetActorLabel(), Vertices.Num(), Indices.Num() / 3);
// }
//


  // i have this function from my old code that would merge the last
  // two meshes (since those were the ones with any sewing since mesh
  // generation and sewing were linked and when sewing were restricted
  // to 2 meshes) so it would create one procedural mesh from those
  // two meshes. but now could i have it so that it creates a procedural
  // mesh from any sewn meshes? so if meshes a and b are sewn but not c it
  // would merget hose first two meshes, etc


