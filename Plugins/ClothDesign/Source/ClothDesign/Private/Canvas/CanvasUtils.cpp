#include "Canvas/CanvasUtils.h"

void FCanvasUtils::SaveStateForUndo(
	TArray<FCanvasState>& UndoStack,
	TArray<FCanvasState>& RedoStack,
	const FCanvasState& CurrentState)
{
	UndoStack.Add(CurrentState);
	RedoStack.Empty();
}


bool FCanvasUtils::Undo(
	TArray<FCanvasState>& UndoStack,
	TArray<FCanvasState>& RedoStack,
	FCanvasState& CurrentState)
{
	if (UndoStack.Num() > 0)
	{
		RedoStack.Add(CurrentState);
		CurrentState = UndoStack.Pop();
		return true;
	}
	return false;
}

bool FCanvasUtils::Redo(
	TArray<FCanvasState>& UndoStack,
	TArray<FCanvasState>& RedoStack,
	FCanvasState& CurrentState)
{
	if (RedoStack.Num() > 0)
	{
		UndoStack.Add(CurrentState);
		CurrentState = RedoStack.Pop();
		return true;
	}
	return false;
}


void FCanvasUtils::RecalculateNTangents(
	FInterpCurve<FVector2D>& Curve,
	const TArray<bool>&      bBezierFlags)
{
	int32 Num = Curve.Points.Num();
	if (Num < 2) return;

	for (int32 i = 0; i < Num; ++i)
	{
		// Only operate on N‑points
		if (bBezierFlags[i]) continue;

		// Prev
		if (i > 0)
		{
			FVector2D Prev = Curve.Points[i-1].OutVal;
			FVector2D Curr = Curve.Points[i  ].OutVal;
			Curve.Points[i].ArriveTangent = (Curr - Prev) * 0.5f;
		}
		else
		{
			Curve.Points[i].ArriveTangent = FVector2D::ZeroVector;
		}

		// Next
		if (i < Num - 1)
		{
			FVector2D Curr = Curve.Points[i  ].OutVal;
			FVector2D Next = Curve.Points[i+1].OutVal;
			Curve.Points[i].LeaveTangent = (Next - Curr) * 0.5f;
		}
		else
		{
			Curve.Points[i].LeaveTangent = FVector2D::ZeroVector;
		}
	}
}
