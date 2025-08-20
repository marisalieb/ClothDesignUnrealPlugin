#include "Canvas/CanvasMesh.h"
#include "Canvas/CanvasUtils.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "CoreMinimal.h"
#include "ClothDesignCanvas.h"



// 1) Helper: even–odd rule point-in-polygon test
bool FCanvasMesh::IsPointInPolygon(
	const FVector2f& Test, 
	const TArray<FVector2f>& Poly
) {
	bool bInside = false;
	int N = Poly.Num();
	for (int i = 0, j = N - 1; i < N; j = i++) {
		const FVector2f& A = Poly[i];
		const FVector2f& B = Poly[j];
		bool bYCross = ((A.Y > Test.Y) != (B.Y > Test.Y));
		if (bYCross) {
			float XatY = (B.X - A.X) * (Test.Y - A.Y) / (B.Y - A.Y) + A.X;
			if (Test.X < XatY) {
				bInside = !bInside;
			}
		}
	}
	return bInside;
}


void FCanvasMesh::SampleShapeCurve(
	const FInterpCurve<FVector2D>& Shape,
	bool bRecordSeam,
	int32 StartPointIdx2D,
	int32 EndPointIdx2D,
	int SamplesPerSegment,
	TArray<FVector2f>& OutPolyVerts,
	TArray<int32>& OutSeamVertexIDs,
	TArray<int32>& OutVertexIDs,
	FDynamicMesh3& Mesh)
{
	// Before your loop, compute the integer sample‐range once:
	int TotalSamples = (Shape.Points.Num() - 1) * SamplesPerSegment;
	int SampleCounter = 0;

	// Default to an empty range
	int MinSample = TotalSamples + 1;
	int MaxSample = -1;

	// Only compute if we really want to record a seam
	if (bRecordSeam && StartPointIdx2D >= 0 && EndPointIdx2D >= 0)
	{
		int S0 = StartPointIdx2D * SamplesPerSegment;
		int S1 = EndPointIdx2D   * SamplesPerSegment;
		MinSample = FMath::Min(S0, S1);
		MaxSample = FMath::Max(S0, S1);
	}

	OutSeamVertexIDs.Empty();

	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
	{
		float In0 = Shape.Points[Seg].InVal;
		float In1 = Shape.Points[Seg + 1].InVal;

		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
		{
			float Alpha = static_cast<float>(i) / SamplesPerSegment;
			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
			OutPolyVerts.Add(FVector2f(P2.X, P2.Y));

			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
			OutVertexIDs.Add(VID);

			// record seam if this sample falls in the integer [MinSample,MaxSample] range
			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
			{
				OutSeamVertexIDs.Add(VID);
			}
		}
	}
}

void FCanvasMesh::AddGridInteriorPoints(
	TArray<FVector2f>& PolyVerts,
	int32 OriginalBoundaryCount,
	TArray<int32>& OutVertexIDs,
	FDynamicMesh3& Mesh)
{
	// Your code for bounding box, grid sampling, and adding interior points

	
	// Copy out just the boundary verts for your in‐polygon test:
	TArray<FVector2f> BoundaryOnly;
	BoundaryOnly.Append( PolyVerts.GetData(), OriginalBoundaryCount );

	// --- compute 2D bounding‐box of your sampled polyline
	float MinX = FLT_MAX, MinY = FLT_MAX, MaxX = -FLT_MAX, MaxY = -FLT_MAX;
	for (int32 i = 0; i < OriginalBoundaryCount; ++i)
	{
		const FVector2f& V = PolyVerts[i];
		MinX = FMath::Min(MinX, V.X);
		MinY = FMath::Min(MinY, V.Y);
		MaxX = FMath::Max(MaxX, V.X);
		MaxY = FMath::Max(MaxY, V.Y);
	}

	// --- grid parameters
	constexpr int32 GridRes = 20;    // 10×10 grid → up to 100 interior seeds
	int32 Added = 0;

	// --- sample on a regular grid, keep only centers inside the original polygon
	for (int32 iy = 0; iy < GridRes; ++iy)
	{
		float fy = (iy + 0.5f) / static_cast<float>(GridRes);
		float Y  = FMath::Lerp(MinY, MaxY, fy);

		for (int32 ix = 0; ix < GridRes; ++ix)
		{
			float fx = (ix + 0.5f) / static_cast<float>(GridRes);
			float X  = FMath::Lerp(MinX, MaxX, fx);

			FVector2f Cand(X, Y);
			if ( IsPointInPolygon(Cand, BoundaryOnly) )
			{
				// add to the full list
				PolyVerts.Add(Cand);

				// let your Delaunay/CDT see it:
				int32 VID = Mesh.AppendVertex(FVector3d(Cand.X, Cand.Y, 0));
				OutVertexIDs.Add(VID);

				++Added;
			}
		}
	}
}

void FCanvasMesh::BuildBoundaryEdges(
	int32 OriginalBoundaryCount,
	TArray<UE::Geometry::FIndex2i>& OutBoundaryEdges)
{
	// Your code building boundary edges array
	// Build the list of constrained edges on the original boundary:
	// TArray<UE::Geometry::FIndex2i> BoundaryEdges;
	OutBoundaryEdges.Reserve(OriginalBoundaryCount);
	for (int32 i = 0; i < OriginalBoundaryCount; ++i)
	{
		OutBoundaryEdges.Add(
			UE::Geometry::FIndex2i(i, (i + 1) % OriginalBoundaryCount)
		);
	}
}

void FCanvasMesh::RunConstrainedDelaunay(
	const TArray<FVector2f>& PolyVerts,
	const TArray<UE::Geometry::FIndex2i>& BoundaryEdges,
	UE::Geometry::TConstrainedDelaunay2<float>& OutCDT)
{
	// Your code setting up and running CDT triangulation

	OutCDT.Vertices      = PolyVerts;          // TArray<TVector2<float>>
	OutCDT.Edges         = BoundaryEdges;      // TArray<FIndex2i>
	OutCDT.bOrientedEdges = true;              // enforce input edge orientation
	OutCDT.FillRule = UE::Geometry::TConstrainedDelaunay2<float>::EFillRule::Odd;  

	// CDT.FillRule      = EFillRule::EvenOdd;  
	OutCDT.bOutputCCW    = true;               // get CCW‐wound triangles

	// If you want to cut out hole‐loops, you can fill CDT.HoleEdges similarly.

	// Run the triangulation:
	bool bOK = OutCDT.Triangulate();

	
	if (!bOK || OutCDT.Triangles.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("CDT failed to triangulate shape"));
		return;
	}
	
}

void FCanvasMesh::ConvertCDTToDynamicMesh(
	const UE::Geometry::TConstrainedDelaunay2<float>& CDT,
	FDynamicMesh3& OutMesh,
	TArray<int32>& OutPolyIndexToVID)
{
	// Your code converting CDT vertices and triangles to FDynamicMesh3
	OutMesh.EnableTriangleGroups();
	// MeshOut.SetAllowBowties(true);  // if you expect split‐bowties

	OutPolyIndexToVID.Reset();
	OutPolyIndexToVID.Reserve(CDT.Vertices.Num());

	
	// Append all vertices:
	for (const UE::Geometry::TVector2<float>& V2 : CDT.Vertices)
	{
		int NewVID = OutMesh.AppendVertex(FVector3d(V2.X, V2.Y, 0));
		OutPolyIndexToVID.Add(NewVID);
	}

	// Append all triangles:
	for (const UE::Geometry::FIndex3i& Tri : CDT.Triangles)
	{
		// Tri is CCW if bOutputCCW==true
		//OutMesh.AppendTriangle(Tri.A, Tri.B, Tri.C);
		int VA = OutPolyIndexToVID[Tri.A];
		int VB = OutPolyIndexToVID[Tri.B];
		int VC = OutPolyIndexToVID[Tri.C];
		OutMesh.AppendTriangle(VA, VB, VC);
	}
}

void FCanvasMesh::ExtractVerticesAndIndices(
	const FDynamicMesh3& OutMesh,
	TArray<FVector>& OutVertices,
	TArray<int32>& OutIndices)
{
	// Your code extracting vertex and index arrays for procedural mesh
	OutVertices.Reserve(OutMesh.VertexCount());
	for (int vid : OutMesh.VertexIndicesItr())
	{
		FVector3d P = OutMesh.GetVertex(vid);
		OutVertices.Add(FVector(P.X, P.Y, P.Z));
	}
	for (int tid : OutMesh.TriangleIndicesItr())
	{
		auto T = OutMesh.GetTriangle(tid);
		// already CCW, so push A→B→C
		OutIndices.Add(T.C);
		OutIndices.Add(T.B);
		OutIndices.Add(T.A);
	}
}


void FCanvasMesh::CreateProceduralMesh(
	const TArray<FVector>& Vertices,
	const TArray<int32>& Indices,
	FDynamicMesh3&& DynamicMesh,
	TArray<int32>&& SeamVertexIDs,
	const TArray<FVector2f>& BoundarySamples2D,
	const TArray<int32>& BoundarySampleVIDs,
	TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors,
	FVector MeshCentroid)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return;

    static int32 MeshCounter = 0;
    FString UniqueLabel = FString::Printf(TEXT("ClothMeshActor_%d"), MeshCounter++);

    FActorSpawnParameters SpawnParams;
    APatternMesh* MeshActor = World->SpawnActor<APatternMesh>(SpawnParams);
    if (!MeshActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn APatternMesh!"));
        return;
    }
    OutSpawnedActors.Add(TWeakObjectPtr<APatternMesh>(MeshActor));
	
    {
        // If you want to spawn at a specific location, compute that first and use it here.
        const FVector CurrentActorLoc = MeshActor->GetActorLocation();
        const FVector CentroidWorldPos = MeshActor->GetActorTransform().TransformPosition(MeshCentroid);
        const FVector WorldOffset = CentroidWorldPos - CurrentActorLoc;

        // Move the actor by the computed world offset
        MeshActor->SetActorLocation(CurrentActorLoc + WorldOffset);
    }

    // store transform-dependent data AFTER repositioning so world samples are correct
    MeshActor->DynamicMesh        = MoveTemp(DynamicMesh);
    MeshActor->LastSeamVertexIDs  = MoveTemp(SeamVertexIDs);

    MeshActor->BoundarySamplePoints2D = BoundarySamples2D;
    MeshActor->BoundarySampleVertexIDs = BoundarySampleVIDs;

    // compute and store world positions for convenience (move this AFTER reposition)
    MeshActor->BoundarySampleWorldPositions.Reset();
    MeshActor->BoundarySampleWorldPositions.SetNum(BoundarySampleVIDs.Num());
    for (int i = 0; i < BoundarySampleVIDs.Num(); ++i)
    {
        int VID = BoundarySampleVIDs[i];
        if (VID >= 0 && VID < MeshActor->DynamicMesh.VertexCount())
        {
            FVector3d P = MeshActor->DynamicMesh.GetVertex(VID);
            // DynamicMesh vertex is local to actor; transform to actor's new world transform:
            MeshActor->BoundarySampleWorldPositions[i] = MeshActor->GetActorTransform().TransformPosition(FVector(P.X, P.Y, P.Z));
        }
        else
        {
            MeshActor->BoundarySampleWorldPositions[i] = FVector::ZeroVector;
        }
    }

    MeshActor->SetFolderPath(FName(TEXT("ClothDesignActors")));
#if WITH_EDITOR
    MeshActor->SetActorLabel(UniqueLabel);
#endif

    // Build the visible section
    // NOTE: 'Vertices' must already be centered (centroid subtracted) before you call this fn.
    TArray<FVector>      Normals;      Normals.AddUninitialized(Vertices.Num());
    TArray<FVector2D>    UV0;          UV0.AddUninitialized(Vertices.Num());
    TArray<FLinearColor> VertexColors; VertexColors.AddUninitialized(Vertices.Num());
    TArray<FProcMeshTangent> Tangents; Tangents.AddUninitialized(Vertices.Num());

    for (int i = 0; i < Vertices.Num(); ++i)
    {
        Normals[i]      = FVector::UpVector;
        UV0[i]          = FVector2D(Vertices[i].X * .01f, Vertices[i].Y * .01f);
        VertexColors[i] = FLinearColor::White;
        Tangents[i]     = FProcMeshTangent(1,0,0);
    }

    MeshActor->MeshComponent->CreateMeshSection_LinearColor(
        0, Vertices, Indices,
        Normals, UV0, VertexColors, Tangents,
        /*bCreateCollision=*/true
    );
}




// second version but with steiner points, grid spaced constrained delaunay
void FCanvasMesh::TriangulateAndBuildMesh(
	const FInterpCurve<FVector2D>& Shape,
	bool bRecordSeam ,
	int32 StartPointIdx2D,
	int32 EndPointIdx2D,
	TArray<int32>& LastSeamVertexIDs,
	FDynamicMesh3& LastBuiltMesh,
	TArray<int32>& LastBuiltSeamVertexIDs,
	TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors)
{
	if (Shape.Points.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
		return;
	}

	TArray<FVector2f> PolyVerts;
	constexpr int SamplesPerSegment = 10;
	FDynamicMesh3 Mesh;
	
	// Sample shape curve points and build seam info
	TArray<int32> VertexIDs;
	SampleShapeCurve(Shape, bRecordSeam, StartPointIdx2D, EndPointIdx2D, SamplesPerSegment, PolyVerts, LastSeamVertexIDs, VertexIDs, Mesh);

	// Keep track of boundary vertices for polygon test
	int32 OriginalBoundaryCount = PolyVerts.Num();

	// Add interior points on a grid inside polygon
	AddGridInteriorPoints(PolyVerts, OriginalBoundaryCount, VertexIDs, Mesh);

	// Build constrained edges from boundary vertices
	TArray<UE::Geometry::FIndex2i> BoundaryEdges;
	BuildBoundaryEdges(OriginalBoundaryCount, BoundaryEdges);

	// Run constrained Delaunay triangulation
	UE::Geometry::TConstrainedDelaunay2<float> CDT;
	RunConstrainedDelaunay(PolyVerts, BoundaryEdges, CDT);





	
	// Convert CDT result to dynamic mesh
	UE::Geometry::FDynamicMesh3 OutMesh;
	TArray<int32> PolyIndexToVID;
	ConvertCDTToDynamicMesh(CDT, OutMesh, PolyIndexToVID);

	// Debug: make sure CDT produced triangles
	UE_LOG(LogTemp, Warning, TEXT("[Triangulate] CDT produced: vertices=%d triangles=%d"),
		   CDT.Vertices.Num(), CDT.Triangles.Num());
	
	if (OutSpawnedActors.Num() > 0 && OutSpawnedActors.Last().IsValid())
	{
		OutSpawnedActors.Last()->SetPolyIndexToVID(PolyIndexToVID);
	}

	TArray<FVector2f> BoundarySamples2D;
	TArray<int32> BoundarySampleVIDs;
	TArray<FVector> BoundarySampleWorld; // we'll compute world positions in CreateProceduralMesh

	BoundarySamples2D.Reserve(OriginalBoundaryCount);
	BoundarySampleVIDs.Reserve(OriginalBoundaryCount);

	for (int b = 0; b < OriginalBoundaryCount; ++b)
	{
		BoundarySamples2D.Add(PolyVerts[b]);                 // store 2D sample
		int VID = (b >= 0 && b < PolyIndexToVID.Num()) ? PolyIndexToVID[b] : INDEX_NONE;
		BoundarySampleVIDs.Add(VID);
	}




	
	// Extract vertices and indices for procedural mesh
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	ExtractVerticesAndIndices(OutMesh, Vertices, Indices);

	// Very important: log the triangulated dynamic mesh counts (not the pre-sample Mesh)
	UE_LOG(LogTemp, Warning, TEXT("[Triangulate] OutMesh has %d verts, %d triangles"), OutMesh.VertexCount(), OutMesh.TriangleCount());

	
	LastBuiltMesh = MoveTemp(OutMesh);
	LastBuiltSeamVertexIDs = MoveTemp(LastSeamVertexIDs);


	
	FDynamicMesh3 CentroidTempMesh;
	// Fill TempMesh with Vertices + Indices
	for (const FVector& V : Vertices)
	{
		CentroidTempMesh.AppendVertex(FVector3d(V));
	}
	for (int i = 0; i < Indices.Num(); i += 3)
	{
		CentroidTempMesh.AppendTriangle(Indices[i], Indices[i+1], Indices[i+2]);
	}
	// FVector MeshCentroid = CanvasMesh::ComputeAreaWeightedCentroid(Vertices, Indices);
	FVector3d MeshCentroid = FCanvasUtils::ComputeAreaWeightedCentroid(CentroidTempMesh);
	// // Shift vertices so centroid becomes local origin (pivot at 0,0,0)
	FCanvasUtils::CenterMeshVerticesToOrigin(Vertices, MeshCentroid);
	// ALSO shift the dynamic mesh (double precision)
	FVector3d Centroid3d(MeshCentroid);
	FCanvasUtils::TranslateDynamicMeshBy(LastBuiltMesh, Centroid3d);	// CreateProceduralMesh(Vertices, Indices);


	
	CreateProceduralMesh(
	Vertices,
	Indices,
	MoveTemp(LastBuiltMesh),
	MoveTemp(LastBuiltSeamVertexIDs),
	BoundarySamples2D,
	BoundarySampleVIDs,
	OutSpawnedActors,
	MeshCentroid

		);
}



void FCanvasMesh::TriangulateAndBuildAllMeshes(
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	TArray<FDynamicMesh3>& OutMeshes,
	TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors)
{
	
	for (const FInterpCurve<FVector2D>& Shape : CompletedShapes)
	{
		FDynamicMesh3 Mesh;
		TArray<int32> SeamVerts;
		TArray<int32> TempSeamVerts;

		TriangulateAndBuildMesh(
			Shape,
			false,
			0, 0,
			SeamVerts,
			Mesh,
			TempSeamVerts,
			OutSpawnedActors);
		
		OutMeshes.Add(Mesh);
	}



	for (int i = 0; i < OutSpawnedActors.Num(); ++i)
	{
		if (APatternMesh* A = OutSpawnedActors[i].Get())
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnedActors[%d] = %s"), i, *A->GetName());
		}
	}
}


// void FCanvasMesh::TriangulateAndBuildAllMeshes(
// 	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
// 	const FInterpCurve<FVector2D>& CurvePoints,
// 	TArray<FDynamicMesh3>& OutMeshes,
// 	TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors)
// {
// 	// add check for in-progress shapes to finalise them
//
// 	for (const FInterpCurve<FVector2D>& Shape : CompletedShapes)
// 	{
// 		FDynamicMesh3 Mesh;
// 		TArray<int32> SeamVerts;
// 		TArray<int32> TempSeamVerts;
//
// 		TriangulateAndBuildMesh(
// 			Shape,
// 			false,
// 			0, 0,
// 			SeamVerts,
// 			Mesh,
// 			TempSeamVerts,
// 			OutSpawnedActors);
// 		
// 		OutMeshes.Add(Mesh);
// 	}
//
// 	if (CurvePoints.Points.Num() >= 3)
// 	{
// 		FDynamicMesh3 Mesh;
// 		TArray<int32> SeamVerts;
// 		TArray<int32> TempSeamVerts;
//
// 		TriangulateAndBuildMesh(
// 			CurvePoints,
// 			false,
// 			0, 0,
// 			SeamVerts,
// 			Mesh,
// 			TempSeamVerts,
// 			OutSpawnedActors);
// 		
// 		OutMeshes.Add(Mesh);
// 	}
//
// 	for (int i = 0; i < OutSpawnedActors.Num(); ++i)
// 	{
// 		if (APatternMesh* A = OutSpawnedActors[i].Get())
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("SpawnedActors[%d] = %s"), i, *A->GetName());
// 		}
// 	}
// }

