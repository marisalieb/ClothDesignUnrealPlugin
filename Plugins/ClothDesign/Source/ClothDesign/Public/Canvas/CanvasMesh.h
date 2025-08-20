#ifndef FCanvasMesh_H
#define FCanvasMesh_H

#include "PatternMesh.h"
#include "ConstrainedDelaunay2.h"
#include "MeshOpPreviewHelpers.h" 


struct FCanvasMesh
{
	
public:
	static void TriangulateAndBuildAllMeshes(
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
		TArray<FDynamicMesh3>& OutMeshes,
		TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors);

	
private:
	static bool IsPointInPolygon(
		const FVector2f& Test, 
		const TArray<FVector2f>& Poly);
	
	static void SampleShapeCurve(
		const FInterpCurve<FVector2D>& Shape,
		bool bRecordSeam,
		int32 StartPointIdx2D,
		int32 EndPointIdx2D,
		int SamplesPerSegment,
		TArray<FVector2f>& OutPolyVerts,
		TArray<int32>& OutSeamVertexIDs,
		TArray<int32>& OutVertexIDs,
		FDynamicMesh3& Mesh);

	static void AddGridInteriorPoints(
		TArray<FVector2f>& PolyVerts,
		int32 OriginalBoundaryCount,
		TArray<int32>& OutVertexIDs,
		FDynamicMesh3& Mesh);

	static void BuildBoundaryEdges(
		int32 OriginalBoundaryCount,
		TArray<UE::Geometry::FIndex2i>& OutBoundaryEdges);

	static void RunConstrainedDelaunay(
		const TArray<FVector2f>& PolyVerts,
		const TArray<UE::Geometry::FIndex2i>& BoundaryEdges,
		UE::Geometry::TConstrainedDelaunay2<float>& OutCDT);
	
	static void ConvertCDTToDynamicMesh(
		const UE::Geometry::TConstrainedDelaunay2<float>& CDT,
		FDynamicMesh3& OutMesh,
		TArray<int32>& OutPolyIndexToVID);


	static void ExtractVerticesAndIndices(
		const FDynamicMesh3& MeshOut,
		TArray<FVector>& OutVertices,
		TArray<int32>& OutIndices);

	
	static void CreateProceduralMesh(
		const TArray<FVector>& Vertices,
		const TArray<int32>& Indices,
		FDynamicMesh3&& DynamicMesh,
		TArray<int32>&& SeamVertexIDs,
		const TArray<FVector2f>& BoundarySamples2D,
		const TArray<int32>& BoundarySampleVIDs,
		TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors,
		FVector MeshCentroid);
	
	static void TriangulateAndBuildMesh(
		const FInterpCurve<FVector2D>& Shape,
		bool bRecordSeam ,
		int32 StartPointIdx2D,
		int32 EndPointIdx2D,
		TArray<int32>& LastSeamVertexIDs,
		FDynamicMesh3& LastBuiltMesh,
		TArray<int32>& LastBuiltSeamVertexIDs,
	    TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors);

	friend class FCanvasMeshTests;

};



#endif
