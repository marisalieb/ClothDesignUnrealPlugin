#pragma once

#include "CanvasState.h"

struct FCanvasUtils
{
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

};
