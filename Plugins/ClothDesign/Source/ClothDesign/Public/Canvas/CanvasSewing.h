#ifndef FCanvasSewing_H
#define FCanvasSewing_H

#include "PatternSewingConstraint.h"
#include "PatternMesh.h"

/*
 * Thesis reference:
 * See Chapter 4.7 for detailed explanations.
 */


// Forward declaration
class SClothDesignCanvas;

/**
 * @brief Holds the start and end indices of a shape edge.
 */
struct FEdgeIndices
{
	int32 Start; ///< Index of the starting vertex
	int32 End;   ///< Index of the ending vertex
};

/**
 * @brief Defines a seam connection between two shapes.
 */
struct FSeamDefinition
{
	int32 ShapeA;       ///< Index of the first shape
	FEdgeIndices EdgeA; ///< Edge on the first shape
	int32 ShapeB;       ///< Index of the second shape
	FEdgeIndices EdgeB; ///< Edge on the second shape
};

/**
 * @brief Target point selected for seam creation.
 */
struct FClickTarget
{
	int32 ShapeIndex = INDEX_NONE; ///< Index of the shape clicked
	int32 PointIndex = INDEX_NONE; ///< Index of the point clicked
};

/**
 * @brief Current click state during seam creation.
 */
enum class ESeamClickState : uint8
{
	None,         ///< No points clicked yet
	ClickedAStart,///< Start point of shape A clicked
	ClickedAEnd,  ///< End point of shape A clicked
	ClickedBStart,///< Start point of shape B clicked
	ClickedBEnd   ///< End point of shape B clicked
};



/**
 * @brief Handles seam definition and alignment between canvas shapes.
 * 
 * This class manages user interactions and pattern meshes to define,
 * validate, and merge seams between shapes. It ensures that sewing operations
 * are logically consistent and supports visual previews of seams.
 */
class FCanvasSewing
{
public:
    /** All seam definitions created on the canvas. */
    TArray<FSeamDefinition> SeamDefinitions;

    /** All sewing constraints that have been defined for the patterns. */
    TArray<FPatternSewingConstraint> AllDefinedSeams;

    /** Current click state in the seam creation workflow. */
    ESeamClickState SeamClickState = ESeamClickState::None;

    /** Click targets for the seam endpoints. */
    FClickTarget AStartTarget, AEndTarget, BStartTarget, BEndTarget;

    /** References to the spawned pattern mesh actors. */
    TArray<TWeakObjectPtr<APatternMesh>> SpawnedPatternActors;

    /** Current preview points for the seam under construction. */
    TMap<int32, TSet<int32>> CurrentSeamPreviewPoints;

    /**
     * @brief Finalises a seam definition using the given targets.
     * 
     * Aligns selected endpoints from two shapes to create a valid seam.
     * This allows consistent sewing operations and previews on the canvas.
     *
     * @param AStart The start point target on shape A.
     * @param AEnd The end point target on shape A.
     * @param BStart The start point target on shape B.
     * @param BEnd The end point target on shape B.
     * @param CurvePoints The current curve points defining the shape.
     * @param CompletedShapes All completed shapes currently on the canvas.
     * @param SpawnedPatternActors The spawned pattern mesh actors to update.
     */
    void FinaliseSeamDefinitionByTargets(
        const FClickTarget& AStart,
        const FClickTarget& AEnd,
        const FClickTarget& BStart,
        const FClickTarget& BEnd,
        const FInterpCurve<FVector2D>& CurvePoints,
        const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
        const TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors);

    /**
     * @brief Builds and aligns all seams defined on the canvas.
     * 
     * Ensures that all seam meshes are correctly oriented and consistent,
     * enabling valid merging and visualisation.
     */
    void BuildAndAlignAllSeams();

    /**
     * @brief Clears all seams from the canvas.
     * 
     * Resets seam definitions and preview points, allowing a fresh start.
     */
    void ClearAllSeams();

    /**
     * @brief Merges the pattern meshes based on sewn seams.
     * 
     * Combines separate pattern pieces into unified meshes where seams allow.
     */
    void MergeSewnPatternPieces();

    /**
     * @brief Builds sets of sewn points for all defined seams.
     * 
     * @param OutSewn Output map from shape index to the set of sewn points.
     */
    void BuildSewnPointSets(TMap<int32, TSet<int32>>& OutSewn) const;

    /**
     * @brief Adds a preview point for the currently defined seam.
     * 
     * Enables visual feedback of seam construction without finalising.
     *
     * @param ShapeIndex The index of the shape containing the point.
     * @param PointIndex The index of the point to preview.
     */
    void AddPreviewPoint(int32 ShapeIndex, int32 PointIndex);

    /**
     * @brief Validates that meshes can be sewn for the given targets.
     * 
     * Optionally shows a dialog if invalid configurations are detected.
     *
     * @param AStart The start target on shape A.
     * @param BStart The start target on shape B.
     * @param SpawnedPatternActors Pattern meshes involved in the validation.
     * @param bShowDialog If true, display warnings for invalid seams.
     * @return True if the meshes are valid for sewing, false otherwise.
     */
    bool ValidateMeshesForTargets(
        const FClickTarget& AStart,
        const FClickTarget& BStart,
        const TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors,
        bool bShowDialog);

    /**
     * @brief Validates that a specific shape mesh is suitable for sewing.
     * 
     * Optionally shows a dialog if the shape is invalid.
     *
     * @param ShapeIndex The index of the shape to validate.
     * @param SpawnedPatternActors Pattern meshes involved in the validation.
     * @param bShowDialog If true, display warnings for invalid configurations.
     * @return True if the shape mesh is valid, false otherwise.
     */
    bool ValidateMeshForShape(
        int32 ShapeIndex,
        const TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors,
        bool bShowDialog);

private:
    /**
     * @brief Aligns two pattern meshes to match their seam endpoints.
     * 
     * Ensures that sewn meshes will fit together correctly in 3D space.
     *
     * @param A First mesh to align.
     * @param B Second mesh to align.
     */
    static void AlignSeamMeshes(APatternMesh* A, APatternMesh* B);

    /**
     * @brief Builds and aligns a single seam based on its constraint.
     * 
     * Called internally when finalising or merging seams.
     *
     * @param Seam The sewing constraint defining the seam.
     */
    void BuildAndAlignSeam(const FPatternSewingConstraint& Seam);

    /** Test helper class for unit testing seam functionality. */
    friend class FCanvasSewingTestHelper;
};

#endif
