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