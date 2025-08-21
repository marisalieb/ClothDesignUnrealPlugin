#ifndef FMeshTriangulation_H
#define FMeshTriangulation_H

#include "PatternMesh.h"
#include "ConstrainedDelaunay2.h"
#include "MeshOpPreviewHelpers.h" 

/**
 *
 * @note Thesis reference:
 * See Chapter 4.6 for detailed explanations.
 */

/**
 * @brief Handles triangulation and procedural mesh generation from canvas shapes.
 * 
 * This class centralises all operations needed to convert user-drawn 2D shapes into
 * fully triangulated 3D meshes. By encapsulating this functionality, it ensures that
 * mesh construction is consistent, reusable, and maintainable.
 */
class FMeshTriangulation
{
public:
    /**
     * @brief Converts completed shapes into dynamic meshes and spawns corresponding actors.
     * @param CompletedShapes Curves representing the completed shapes on the canvas.
     * @param OutMeshes Array to receive generated dynamic meshes.
     * @param OutSpawnedActors Array to receive spawned mesh actors.
     * 
     * This method abstracts the complex workflow of triangulation, vertex sampling,
     * and actor creation, so that canvas shapes can be visualised and further manipulated.
     */
    static void TriangulateAndBuildAllMeshes(
        const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
        TArray<FDynamicMesh3>& OutMeshes,
        TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors);

private:
    /**
     * @brief Checks whether a 2D point lies inside a polygon.
     * @param Test The point to check.
     * @param Poly The polygon to test against.
     * @return True if the point is inside the polygon.
     * 
     * Used internally to verify point locations during triangulation and vertex placement.
     */
    static bool IsPointInPolygon(
        const FVector2f& Test, 
        const TArray<FVector2f>& Poly);

    /**
     * @brief Samples points along a shape curve for mesh construction.
     * @param Shape The curve to sample.
     * @param bRecordSeam Whether to record seam vertices for later processing.
     * @param StartPointIdx2D Index to start sampling from.
     * @param EndPointIdx2D Index to stop sampling.
     * @param SamplesPerSegment Number of samples per curve segment.
     * @param OutPolyVerts Output array of polygon vertices.
     * @param OutSeamVertexIDs Output array of seam vertex IDs.
     * @param OutVertexIDs Output array of vertex IDs.
     * @param Mesh Mesh being constructed.
     * 
     * Provides a controlled way to convert curves into polygon vertices, maintaining
     * seam and vertex information needed for triangulation.
     */
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

    /**
     * @brief Adds interior points to a polygon to prepare for triangulation.
     * @param PolyVerts Polygon vertices.
     * @param OriginalBoundaryCount Number of boundary vertices.
     * @param OutVertexIDs Output array of vertex IDs after adding interior points.
     * @param Mesh Mesh being constructed.
     * 
     * Ensures triangulation produces well-formed meshes by populating interior points.
     */
    static void AddGridInteriorPoints(
        TArray<FVector2f>& PolyVerts,
        int32 OriginalBoundaryCount,
        TArray<int32>& OutVertexIDs,
        FDynamicMesh3& Mesh);

    /**
     * @brief Generates the boundary edges for a polygon.
     * @param OriginalBoundaryCount Number of vertices on the original boundary.
     * @param OutBoundaryEdges Output array of polygon boundary edges.
     * 
     * Boundary edges are needed for constrained Delaunay triangulation.
     */
    static void BuildBoundaryEdges(
        int32 OriginalBoundaryCount,
        TArray<UE::Geometry::FIndex2i>& OutBoundaryEdges);

    /**
     * @brief Performs constrained Delaunay triangulation on polygon vertices.
     * @param PolyVerts Polygon vertices.
     * @param BoundaryEdges Edges that constrain the triangulation.
     * @param OutCDT Output CDT object.
     * 
     * Converts polygon vertices into a triangulated mesh structure suitable for 3D mesh generation.
     */
    static void RunConstrainedDelaunay(
        const TArray<FVector2f>& PolyVerts,
        const TArray<UE::Geometry::FIndex2i>& BoundaryEdges,
        UE::Geometry::TConstrainedDelaunay2<float>& OutCDT);

    /**
     * @brief Converts a CDT into a dynamic mesh.
     * @param CDT The triangulated CDT.
     * @param OutMesh Output dynamic mesh.
     * @param OutPolyIndexToVID Mapping from polygon indices to vertex IDs.
     * 
     * Bridges the gap between 2D triangulation data and the final dynamic 3D mesh.
     */
    static void ConvertCDTToDynamicMesh(
        const UE::Geometry::TConstrainedDelaunay2<float>& CDT,
        FDynamicMesh3& OutMesh,
        TArray<int32>& OutPolyIndexToVID);

    /**
     * @brief Extracts vertices and indices from a dynamic mesh for procedural mesh generation.
     * @param MeshOut Input mesh.
     * @param OutVertices Output array of vertex positions.
     * @param OutIndices Output array of triangle indices.
     * 
     * Prepares mesh data for use with procedural mesh components or actor creation.
     */
    static void ExtractVerticesAndIndices(
        const FDynamicMesh3& MeshOut,
        TArray<FVector>& OutVertices,
        TArray<int32>& OutIndices);

    /**
     * @brief Creates a procedural mesh actor in the scene.
     * @param Vertices Mesh vertices.
     * @param Indices Triangle indices.
     * @param DynamicMesh The source dynamic mesh.
     * @param SeamVertexIDs IDs of seam vertices.
     * @param BoundarySamples2D Boundary samples of the original shape.
     * @param BoundarySampleVIDs Corresponding vertex IDs for boundary samples.
     * @param OutSpawnedActors Array to receive spawned actor references.
     * @param MeshCentroid Centroid of the mesh for positioning.
     * 
     * Encapsulates actor creation and mesh assignment, separating procedural generation
     * from low-level mesh data manipulation.
     */
    static void CreateProceduralMesh(
        const TArray<FVector>& Vertices,
        const TArray<int32>& Indices,
        FDynamicMesh3&& DynamicMesh,
        TArray<int32>&& SeamVertexIDs,
        const TArray<FVector2f>& BoundarySamples2D,
        const TArray<int32>& BoundarySampleVIDs,
        TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors,
        FVector MeshCentroid);

    /**
     * @brief Triangulates a single shape and updates the last built mesh and seam data.
     * @param Shape The shape to triangulate.
     * @param bRecordSeam Whether to record seam vertices.
     * @param StartPointIdx2D Start index for sampling.
     * @param EndPointIdx2D End index for sampling.
     * @param LastSeamVertexIDs Last recorded seam vertex IDs.
     * @param LastBuiltMesh The last constructed mesh to update.
     * @param LastBuiltSeamVertexIDs Seam vertices for the last mesh.
     * @param OutSpawnedActors Array to receive spawned actors.
     * 
     * Provides incremental triangulation and mesh updates to support complex shape sequences.
     */
    static void TriangulateAndBuildMesh(
        const FInterpCurve<FVector2D>& Shape,
        bool bRecordSeam,
        int32 StartPointIdx2D,
        int32 EndPointIdx2D,
        TArray<int32>& LastSeamVertexIDs,
        FDynamicMesh3& LastBuiltMesh,
        TArray<int32>& LastBuiltSeamVertexIDs,
        TArray<TWeakObjectPtr<APatternMesh>>& OutSpawnedActors);

    friend class FMeshTriangulationTests; /**< Allows the test class to access private mesh internals. */
};



#endif
