#ifndef FCanvasUtils_H
#define FCanvasUtils_H

#include "CanvasState.h"
#include "DynamicMesh/DynamicMesh3.h"

struct FCanvasUtils
{
	
public:
	static void SaveStateForUndo(
	  TArray<FCanvasState>& UndoStack,
	  TArray<FCanvasState>& RedoStack,
	  const FCanvasState& CurrentState);

	static bool Undo(
	  TArray<FCanvasState>& UndoStack,
	  TArray<FCanvasState>& RedoStack,
	  FCanvasState& CurrentState);

	static bool Redo(
	  TArray<FCanvasState>& UndoStack,
	  TArray<FCanvasState>& RedoStack,
	  FCanvasState& CurrentState);

	// recalc n-point tangents
	static void RecalculateNTangents(
		FInterpCurve<FVector2D>& Curve,
		const TArray<bool>&      bBezierFlags);

	static FVector3d ComputeAreaWeightedCentroid(const UE::Geometry::FDynamicMesh3& Mesh);
	static void CenterMeshVerticesToOrigin(TArray<FVector>& Vertices, const FVector& PivotFrom);
	static void TranslateDynamicMeshBy(UE::Geometry::FDynamicMesh3& Mesh, const FVector3d& Offset);

};

#endif

