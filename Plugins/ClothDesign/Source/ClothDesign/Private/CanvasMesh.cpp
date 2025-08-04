#include "CanvasMesh.h"

// 1) Helper: even–odd rule point-in-polygon test
bool CanvasMesh::IsPointInPolygon(
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


void CanvasMesh::CreateProceduralMesh(
	const TArray<FVector>& Vertices,
	const TArray<int32>& Indices,
	FDynamicMesh3&& DynamicMesh,
	TArray<int32>&& SeamVertexIDs,
	TArray<AClothPatternMeshActor*>& OutSpawnedActors)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	static int32 MeshCounter = 0;
	FString UniqueLabel = FString::Printf(TEXT("ClothMeshActor_%d"), MeshCounter++);

	FActorSpawnParameters SpawnParams;
	AClothPatternMeshActor* MeshActor = World->SpawnActor<AClothPatternMeshActor>(SpawnParams);
	if (!MeshActor) return;

	OutSpawnedActors.Add(MeshActor);

	// Transfer ownership of mesh data
	MeshActor->DynamicMesh        = MoveTemp(DynamicMesh);
	MeshActor->LastSeamVertexIDs = MoveTemp(SeamVertexIDs);

	MeshActor->SetFolderPath(FName(TEXT("GeneratedClothActors")));

#if WITH_EDITOR
	MeshActor->SetActorLabel(UniqueLabel);
#endif

	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	for (const FVector& V : Vertices)
	{
		Normals.Add(FVector::UpVector);
		UV0.Add(FVector2D(V.X * 0.01f, V.Y * 0.01f));
		VertexColors.Add(FLinearColor::White);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
	}

	MeshActor->MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UV0, VertexColors, Tangents, true);
}





// second version but with steiner points, grid spaced constrained delaunay
void CanvasMesh::TriangulateAndBuildMesh(
	const FInterpCurve<FVector2D>& Shape,
	bool bRecordSeam,
	int32 StartPointIdx2D,
	int32 EndPointIdx2D,
	/* out */ TArray<int32>& LastSeamVertexIDs,
	/* out */ FDynamicMesh3& LastBuiltMesh,
	/* optional */ TArray<int32> LastBuiltSeamVertexIDs)
{
	if (Shape.Points.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
		return;
	}

	TArray<FVector2f> PolyVerts;
	const int SamplesPerSegment = 10;
	// Step 3: Build DynamicMesh
	FDynamicMesh3 Mesh;

	
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

	LastSeamVertexIDs.Empty();
	TArray<int32> VertexIDs;

	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
	{
		float In0 = Shape.Points[Seg].InVal;
		float In1 = Shape.Points[Seg + 1].InVal;

		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
		{
			float Alpha = float(i) / SamplesPerSegment;
			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
			PolyVerts.Add(FVector2f(P2.X, P2.Y));

			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
			VertexIDs.Add(VID);

			// record seam if this sample falls in the integer [MinSample,MaxSample] range
			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
			{
				LastSeamVertexIDs.Add(VID);
			}
		}
	}

	// RIGHT HERE: remember how many boundary points you have
	int32 OriginalBoundaryCount = PolyVerts.Num();
	
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
	const int32 GridRes = 20;    // 10×10 grid → up to 100 interior seeds
	int32 Added = 0;

	// --- sample on a regular grid, keep only centers inside the original polygon
	for (int32 iy = 0; iy < GridRes; ++iy)
	{
		float fy = (iy + 0.5f) / float(GridRes);
		float Y  = FMath::Lerp(MinY, MaxY, fy);

		for (int32 ix = 0; ix < GridRes; ++ix)
		{
			float fx = (ix + 0.5f) / float(GridRes);
			float X  = FMath::Lerp(MinX, MaxX, fx);

			FVector2f Cand(X, Y);
			if ( IsPointInPolygon(Cand, BoundaryOnly) )
			{
				// add to the full list
				PolyVerts.Add(Cand);

				// let your Delaunay/CDT see it:
				int32 VID = Mesh.AppendVertex(FVector3d(Cand.X, Cand.Y, 0));
				VertexIDs.Add(VID);

				++Added;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Placed %d grid‐based interior samples"), Added);

	// Build the list of constrained edges on the original boundary:
	TArray<UE::Geometry::FIndex2i> BoundaryEdges;
	BoundaryEdges.Reserve(OriginalBoundaryCount);
	for (int32 i = 0; i < OriginalBoundaryCount; ++i)
	{
		BoundaryEdges.Add(
			UE::Geometry::FIndex2i(i, (i + 1) % OriginalBoundaryCount)
		);
	}
	

	// --- 2) Set up and run the Constrained Delaunay ---
	UE::Geometry::TConstrainedDelaunay2<float> CDT;
	
	
	CDT.Vertices      = PolyVerts;          // TArray<TVector2<float>>
	CDT.Edges         = BoundaryEdges;      // TArray<FIndex2i>
	CDT.bOrientedEdges = true;              // enforce input edge orientation
	CDT.FillRule = UE::Geometry::TConstrainedDelaunay2<float>::EFillRule::Odd;  

	// CDT.FillRule      = EFillRule::EvenOdd;  
	CDT.bOutputCCW    = true;               // get CCW‐wound triangles

	// If you want to cut out hole‐loops, you can fill CDT.HoleEdges similarly.

	// Run the triangulation:
	bool bOK = CDT.Triangulate();
	if (!bOK || CDT.Triangles.Num() == 0)
	{
	    UE_LOG(LogTemp, Error, TEXT("CDT failed to triangulate shape"));
	    return;
	}

	// --- 3) Move it into an FDynamicMesh3 ---
	UE::Geometry::FDynamicMesh3 MeshOut;
	MeshOut.EnableTriangleGroups();
	// MeshOut.SetAllowBowties(true);  // if you expect split‐bowties

	// Append all vertices:
	for (const UE::Geometry::TVector2<float>& V2 : CDT.Vertices)
	{
	    MeshOut.AppendVertex(FVector3d(V2.X, V2.Y, 0));
	}

	// Append all triangles:
	for (const UE::Geometry::FIndex3i& Tri : CDT.Triangles)
	{
	    // Tri is CCW if bOutputCCW==true
	    MeshOut.AppendTriangle(Tri.A, Tri.B, Tri.C);
	}

	// --- 4) Extract to your ProceduralMeshComponent as before ---
	TArray<FVector> Vertices;
	TArray<int32>   Indices;
	Vertices.Reserve(MeshOut.VertexCount());
	for (int vid : MeshOut.VertexIndicesItr())
	{
	    FVector3d P = MeshOut.GetVertex(vid);
	    Vertices.Add(FVector(P.X, P.Y, P.Z));
	}
	for (int tid : MeshOut.TriangleIndicesItr())
	{
	    auto T = MeshOut.GetTriangle(tid);
	    // already CCW, so push A→B→C
	    Indices.Add(T.C);
	    Indices.Add(T.B);
	    Indices.Add(T.A);
	}

	LastBuiltMesh           = MoveTemp(Mesh);
	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);

	// CreateProceduralMesh(Vertices, Indices);
	CreateProceduralMesh(
	Vertices,
	Indices,
	MoveTemp(LastBuiltMesh),
	MoveTemp(LastBuiltSeamVertexIDs),
	SpawnedPatternActors
		);
}


void CanvasMesh::TriangulateAndBuildAllMeshes(
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	const FInterpCurve<FVector2D>& CurvePoints,
	TArray<FDynamicMesh3>& OutMeshes)
{

	for (const FInterpCurve<FVector2D>& Shape : CompletedShapes)
	{
		FDynamicMesh3 Mesh;
		TArray<int32> SeamVerts;
		TArray<int32> DummySeamVerts;

		TriangulateAndBuildMesh(
			Shape,
			false,
			0, 0,
			SeamVerts,
			Mesh,
			DummySeamVerts
			);
		OutMeshes.Add(Mesh);
	}

	if (CurvePoints.Points.Num() >= 3)
	{
		FDynamicMesh3 Mesh;
		TArray<int32> SeamVerts;
		TArray<int32> DummySeamVerts;

		TriangulateAndBuildMesh(
			CurvePoints,
			false,
			0, 0,
			SeamVerts,
			Mesh,
			DummySeamVerts
			);
		OutMeshes.Add(Mesh);
	}
}