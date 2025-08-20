#ifndef FCanvasUtils_H
#define FCanvasUtils_H

#include "CanvasState.h"
#include "DynamicMesh/DynamicMesh3.h"

// /*
//  * Thesis reference:
//  * See Chapter 4.9.2 for detailed explanations.
//  */

/**
 * @file CanvasUtils.h
 *
 * @note Thesis reference:
 * See Chapter 4.9.2 for detailed explanations.
 */


/**
 * @brief Utility functions for managing canvas state and mesh operations.
 * 
 * This class centralises common operations on canvas states and mesh geometry,
 * providing support for undo/redo functionality and geometric transformations,
 * so that these behaviours are consistent and maintainable across the application.
 */
class FCanvasUtils
{
	
public:
	/**
	 * @brief Saves the current canvas state for undo purposes.
	 * @param UndoStack Stack storing previous canvas states.
	 * @param RedoStack Stack storing states that can be redone.
	 * @param CurrentState The current canvas state to save.
	 * 
	 * This method ensures that changes can be reverted by maintaining a history of canvas states,
	 * preventing loss of user work and enabling undo functionality.
	 */
	static void SaveStateForUndo(
	  TArray<FCanvasState>& UndoStack,
	  TArray<FCanvasState>& RedoStack,
	  const FCanvasState& CurrentState);

	/**
	 * @brief Reverts the canvas to the previous state.
	 * @param UndoStack Stack storing previous canvas states.
	 * @param RedoStack Stack storing states that can be redone.
	 * @param CurrentState The canvas state to update with the previous state.
	 * @return True if the undo operation was successful, false otherwise.
	 * 
	 * Undo exists to allow users to safely experiment with designs without fear of making irreversible changes.
	 */
	static bool Undo(
	  TArray<FCanvasState>& UndoStack,
	  TArray<FCanvasState>& RedoStack,
	  FCanvasState& CurrentState);

	/**
	 * @brief Reapplies a previously undone canvas state.
	 * @param UndoStack Stack storing previous canvas states.
	 * @param RedoStack Stack storing states that can be redone.
	 * @param CurrentState The canvas state to update with the redone state.
	 * @return True if the redo operation was successful, false otherwise.
	 * 
	 * Redo complements undo by allowing users to restore states they have reverted,
	 * supporting flexible design experimentation.
	 */
	static bool Redo(
	  TArray<FCanvasState>& UndoStack,
	  TArray<FCanvasState>& RedoStack,
	  FCanvasState& CurrentState);

	/**
	 * @brief Recalculates tangents for an non-bezier/linear point curve.
	 * @param Curve The curve to update.
	 * @param bBezierFlags Flags indicating which points are Bezier handles.
	 * 
	 * This ensures that curve tangents remain consistent after edits,
	 * preserving smoothness and predictable behaviour in drawn paths.
	 */
	static void RecalculateNTangents(
		FInterpCurve<FVector2D>& Curve,
		const TArray<bool>&      bBezierFlags);

	/**
	 * @brief Computes the area-weighted centroid of a dynamic mesh.
	 * @param Mesh The mesh to compute the centroid for.
	 * @return The 3D position of the centroid.
	 * 
	 * The centroid provides a geometric reference for positioning and transformations,
	 * allowing operations like centering or rotation to behave intuitively.
	 */
	static FVector3d ComputeAreaWeightedCentroid(const UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * @brief Moves mesh vertices so that the mesh is centred at a reference point.
	 * @param Vertices Array of vertex positions to adjust.
	 * @param PivotFrom The reference point from which to centre the vertices.
	 * 
	 * Centering vertices simplifies subsequent transformations and ensures consistent alignment
	 * relative to the chosen pivot.
	 */
	static void CenterMeshVerticesToOrigin(TArray<FVector>& Vertices, const FVector& PivotFrom);

	/**
	 * @brief Translates a dynamic mesh by a given offset.
	 * @param Mesh The mesh to translate.
	 * @param Offset The vector by which to move the mesh.
	 * 
	 * Translation exists to reposition geometry within the scene, 
	 * supporting layout adjustments, centering, or coordinated transformations.
	 */
	static void TranslateDynamicMeshBy(UE::Geometry::FDynamicMesh3& Mesh, const FVector3d& Offset);

};

#endif

