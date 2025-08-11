#include "Canvas/CanvasSewing.h"
#include "Canvas/CanvasMesh.h"
#include "Canvas/CanvasPatternMerge.h"
#include "Misc/MessageDialog.h"



void FCanvasSewing::FinaliseSeamDefinitionByTargets(
	const FClickTarget& AStart,
	const FClickTarget& AEnd,
	const FClickTarget& BStart,
	const FClickTarget& BEnd,
	const FInterpCurve<FVector2D>& CurvePoints,
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	const TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors)
{
	constexpr int32 NumSeamPoints = 10;
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
		float Alpha = static_cast<float>(i) / (NumSeamPoints - 1);
		PointsA.Add(FMath::Lerp(A1, A2, Alpha));
		PointsB.Add(FMath::Lerp(B1, B2, Alpha));
	}
	
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


void FCanvasSewing::AlignSeamMeshes(APatternMesh* MeshActorA, APatternMesh* MeshActorB)
{
    if (!MeshActorA || !MeshActorB) return;

    const TArray<int32>& IDsA = MeshActorA->LastSeamVertexIDs;
    const TArray<int32>& IDsB = MeshActorB->LastSeamVertexIDs;
    int32 NumA = IDsA.Num(), NumB = IDsB.Num();
    if (NumA == 0 || NumB == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot align: one of the seam lists is empty (A=%d, B=%d)"), NumA, NumB);
        return;
    }

    // Use min count (defensive)
    int32 N = FMath::Min(NumA, NumB);

    // pick first and last valid indices to represent seam endpoints (robust for straight seams)
    int firstIdx = 0;
    while (firstIdx < N &&
           (IDsA[firstIdx] < 0 || IDsA[firstIdx] >= MeshActorA->DynamicMesh.VertexCount() ||
            IDsB[firstIdx] < 0 || IDsB[firstIdx] >= MeshActorB->DynamicMesh.VertexCount()))
    {
        ++firstIdx;
    }
    int lastIdx = N - 1;
    while (lastIdx >= 0 &&
           (IDsA[lastIdx] < 0 || IDsA[lastIdx] >= MeshActorA->DynamicMesh.VertexCount() ||
            IDsB[lastIdx] < 0 || IDsB[lastIdx] >= MeshActorB->DynamicMesh.VertexCount()))
    {
        --lastIdx;
    }

    if (firstIdx >= lastIdx)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough valid seam points to compute direction (first=%d last=%d)"), firstIdx, lastIdx);
        return;
    }

    // Fetch world-space endpoints
    auto GetWorldVertex = [](const APatternMesh* Actor, int vid) -> FVector {
        FVector3d p = Actor->DynamicMesh.GetVertex(vid);
        return Actor->GetActorTransform().TransformPosition(FVector(p.X, p.Y, p.Z));
    };

    FVector A0 = GetWorldVertex(MeshActorA, IDsA[firstIdx]);
    FVector A1 = GetWorldVertex(MeshActorA, IDsA[lastIdx]);
    FVector B0 = GetWorldVertex(MeshActorB, IDsB[firstIdx]);
    FVector B1 = GetWorldVertex(MeshActorB, IDsB[lastIdx]);

    // Directions (3D)
    FVector DirA = (A1 - A0).GetSafeNormal();
    FVector DirB = (B1 - B0).GetSafeNormal();

    if (DirA.IsNearlyZero() || DirB.IsNearlyZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("Degenerate seam direction (zero-length), skipping rotation."));
    }
    else
    {
        // Compute quaternion that rotates DirB -> DirA
        float cosTheta = FVector::DotProduct(DirB, DirA);
        constexpr float EPS = 1e-6f;

        FQuat RotQuat = FQuat::Identity;

        if (cosTheta > 1.0f - EPS)
        {
            // nearly identical, do nothing
            RotQuat = FQuat::Identity;
        }
        else if (cosTheta < -1.0f + EPS)
        {
            // opposite vectors: find an orthogonal axis to rotate 180deg about
            FVector Ortho = FVector::CrossProduct(FVector::UpVector, DirB);
            if (Ortho.IsNearlyZero())
            {
                Ortho = FVector::CrossProduct(FVector::RightVector, DirB);
            }
            Ortho.Normalize();
            RotQuat = FQuat(Ortho, PI); // 180 degrees
        }
        else
        {
            FVector Axis = FVector::CrossProduct(DirB, DirA);
            Axis.Normalize();
            float Angle = FMath::Acos(FMath::Clamp(cosTheta, -1.0f, 1.0f));
            RotQuat = FQuat(Axis, Angle);
        }

        // Rotate actor B about its seam midpoint (B midpoint)
        FVector MidB = (B0 + B1) * 0.5f;
        FTransform TB = MeshActorB->GetActorTransform();
        FVector OldLoc = TB.GetLocation();
        FQuat OldRot = TB.GetRotation();

        // Rotate position around MidB
        FVector NewLoc = RotQuat.RotateVector(OldLoc - MidB) + MidB;
        FQuat NewRot = RotQuat * OldRot;

        TB.SetRotation(NewRot);
        TB.SetLocation(NewLoc);

        // Use TeleportPhysics if actors might have physics in play:
        MeshActorB->SetActorTransform(TB, false, nullptr, ETeleportType::TeleportPhysics);

        UE_LOG(LogTemp, Log, TEXT("Rotated MeshB by quat (axis=%s angle=%f)"), *RotQuat.GetRotationAxis().ToString(), RotQuat.GetAngle());
    }

    // Recompute world positions after rotation and compute translation offset (average)
    TArray<FVector> WorldA; WorldA.Reserve(N);
    TArray<FVector> WorldB; WorldB.Reserve(N);
    FVector TotalOffset = FVector::ZeroVector;

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

    if (WorldA.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid seam points to translate after rotation."));
        return;
    }

	FVector AverageOffset = TotalOffset / static_cast<float>(WorldA.Num());
    FVector HalfTrans = AverageOffset * 0.5f;

    // Symmetric translation: move A back half, B forward half
    {
        FTransform TA = MeshActorA->GetActorTransform();
        TA.AddToTranslation(-HalfTrans);
        MeshActorA->SetActorTransform(TA, false, nullptr, ETeleportType::TeleportPhysics);
    }
    {
        FTransform TB2 = MeshActorB->GetActorTransform();
        TB2.AddToTranslation(HalfTrans);
        MeshActorB->SetActorTransform(TB2, false, nullptr, ETeleportType::TeleportPhysics);
    }

    UE_LOG(LogTemp, Log, TEXT("AlignSeamMeshes: rotated B into alignment then applied symmetric translation (AverageOffset=%s)"), *AverageOffset.ToString());
}


void FCanvasSewing::BuildAndAlignSeam(
	const FPatternSewingConstraint& Seam,
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	const FInterpCurve<FVector2D>& CurvePoints)
{
	// --- helper: find the APatternMesh that owns this procedural mesh component ---
	auto FindActorForMesh = [&](const UProceduralMeshComponent* MeshComp) -> APatternMesh*
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
    constexpr int32 NumSeamSamples = 10;
    auto SampleSegment2D = [&](const FVector2D& A, const FVector2D& B, TArray<FVector2D>& Out)
	{
        Out.Reset(); Out.Reserve(NumSeamSamples);
        for (int i = 0; i < NumSeamSamples; ++i)
        {
			float Alpha = static_cast<float>(i) / static_cast<float>(NumSeamSamples - 1);
            Out.Add(FMath::Lerp(A, B, Alpha));
        }
    };
	
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
            	float dx = S.X - static_cast<float>(Q.X);
            	float dy = S.Y - static_cast<float>(Q.Y);
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
	auto BuildWorldPositions = [&](const APatternMesh* Actor, const TArray<int32>& VIDs, TArray<FVector>& OutPos) {
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
		return sum / static_cast<double>(n);
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


void FCanvasSewing::MergeSewnPatternPieces()
{
	FCanvasPatternMerge Merge(SpawnedPatternActors, AllDefinedSeams);
	Merge.MergeSewnGroups();
}



void FCanvasSewing::BuildSewnPointSets(TMap<int32, TSet<int32>>& OutSewn) const
{
	OutSewn.Empty();
	for (const FSeamDefinition& S : SeamDefinitions)
	{
		// Shape A
		TSet<int32>& SetA = OutSewn.FindOrAdd(S.ShapeA);
		SetA.Add(S.EdgeA.Start);
		SetA.Add(S.EdgeA.End);

		// Shape B
		TSet<int32>& SetB = OutSewn.FindOrAdd(S.ShapeB);
		SetB.Add(S.EdgeB.Start);
		SetB.Add(S.EdgeB.End);
		
	}
}

void FCanvasSewing::AddPreviewPoint(int32 ShapeIndex, int32 PointIndex)
{
	if (ShapeIndex != INDEX_NONE && PointIndex != INDEX_NONE)
	{
		CurrentSeamPreviewPoints.FindOrAdd(ShapeIndex).Add(PointIndex);
	}
}
