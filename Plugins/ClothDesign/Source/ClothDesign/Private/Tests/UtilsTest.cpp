// CanvasUtilsTests.cpp
#include "Misc/AutomationTest.h"
#include "Containers/Array.h"
#include "Math/Vector2D.h"
#include "Math/UnrealMathUtility.h"

// Replace these includes with the correct paths in the project:
#include "Canvas/CanvasUtils.h"
#include "Canvas/CanvasState.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasUtils_UndoRedoTest, "Project.CanvasUtils.UndoRedo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasUtils_UndoRedoTest::RunTest(const FString& Parameters)
{
	// Prepare stacks and a current state
	TArray<FCanvasState> UndoStack;
	TArray<FCanvasState> RedoStack;
	FCanvasState Current;
	Current.SelectedPointIndex = 5;
	Current.PanOffset = FVector2D(10.f, 20.f);
	Current.ZoomFactor = 2.0f;

	// Save a copy to compare later
	FCanvasState Saved = Current;

	// Call SaveStateForUndo
	FCanvasUtils::SaveStateForUndo(UndoStack, RedoStack, Current);

	TestTrue(TEXT("UndoStack should have one entry after SaveStateForUndo"), UndoStack.Num() == 1);
	TestTrue(TEXT("RedoStack should be empty after SaveStateForUndo"), RedoStack.Num() == 0);
	TestTrue(TEXT("Saved state equals top of UndoStack"), UndoStack[0] == Saved);

	// Modify Current then Undo
	Current.SelectedPointIndex = 2;
	bool bUndo = FCanvasUtils::Undo(UndoStack, RedoStack, Current);

	TestTrue(TEXT("Undo should succeed when UndoStack not empty"), bUndo);
	TestTrue(TEXT("RedoStack should now contain previous Current"), RedoStack.Num() == 1);
	TestTrue(TEXT("Current should equal saved state after undo"), Current == Saved);

	// Undo again -> should fail because UndoStack is empty now
	bool bUndoWhenEmpty = FCanvasUtils::Undo(UndoStack, RedoStack, Current);
	TestFalse(TEXT("Undo should fail when UndoStack is empty"), bUndoWhenEmpty);

	// Redo (we have one item in RedoStack from earlier)
	bool bRedo = FCanvasUtils::Redo(UndoStack, RedoStack, Current);
	TestTrue(TEXT("Redo should succeed when RedoStack not empty"), bRedo);
	TestTrue(TEXT("UndoStack should have one entry after redo"), UndoStack.Num() == 1);

	// Redo again -> should fail (RedoStack now empty)
	bool bRedoWhenEmpty = FCanvasUtils::Redo(UndoStack, RedoStack, Current);
	TestFalse(TEXT("Redo should fail when RedoStack is empty"), bRedoWhenEmpty);

	return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasUtils_RecalculateNTangentsTest, "Project.CanvasUtils.RecalculateNTangents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasUtils_RecalculateNTangentsTest::RunTest(const FString& Parameters)
{
	// Build a simple 3-point non-bezier curve:
	// A = (0,0), B = (1,2), C = (2,0)
	FInterpCurve<FVector2D> Curve;
	Curve.Points.AddDefaulted(); // point 0
	Curve.Points.AddDefaulted(); // point 1
	Curve.Points.AddDefaulted(); // point 2

	Curve.Points[0].OutVal = FVector2D(0.f, 0.f);
	Curve.Points[1].OutVal = FVector2D(1.f, 2.f);
	Curve.Points[2].OutVal = FVector2D(2.f, 0.f);

	// All points are N-points (not bezier)
	TArray<bool> bBezierFlags;
	bBezierFlags.Init(false, Curve.Points.Num());

	// Call function under test
	FCanvasUtils::RecalculateNTangents(Curve, bBezierFlags);

	// Expected tangents:
	// p0: Arrive = (0,0), Leave = (B-A)*0.5 = (0.5, 1)
	// p1: Arrive = (B-A)*0.5 = (0.5, 1), Leave = (C-B)*0.5 = (0.5, -1)
	// p2: Arrive = (C-B)*0.5 = (0.5, -1), Leave = (0,0)
	const float Tolerance = KINDA_SMALL_NUMBER;

	TestTrue(TEXT("Point 0 arrive should be zero"), Curve.Points[0].ArriveTangent.Equals(FVector2D::ZeroVector, Tolerance));
	TestTrue(TEXT("Point 0 leave should be (0.5,1)"), Curve.Points[0].LeaveTangent.Equals(FVector2D(0.5f, 1.0f), Tolerance));

	TestTrue(TEXT("Point 1 arrive should be (0.5,1)"), Curve.Points[1].ArriveTangent.Equals(FVector2D(0.5f, 1.0f), Tolerance));
	TestTrue(TEXT("Point 1 leave should be (0.5,-1)"), Curve.Points[1].LeaveTangent.Equals(FVector2D(0.5f, -1.0f), Tolerance));

	TestTrue(TEXT("Point 2 arrive should be (0.5,-1)"), Curve.Points[2].ArriveTangent.Equals(FVector2D(0.5f, -1.0f), Tolerance));
	TestTrue(TEXT("Point 2 leave should be zero"), Curve.Points[2].LeaveTangent.Equals(FVector2D::ZeroVector, Tolerance));

	return true;
}

// // ---------- Undo/Redo tests ----------
// IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasUndoRedoTest, "Project.Canvas.UndoRedo.Basic",
//     EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
//
// bool FCanvasUndoRedoTest::RunTest(const FString& Parameters)
// {
//     // Prepare stacks and initial current state
//     TArray<FCanvasState> UndoStack;
//     TArray<FCanvasState> RedoStack;
//
//     FCanvasState InitialState;
//     InitialState.SelectedPointIndex = 5;
//     InitialState.PanOffset = FVector2D(10.f, 20.f);
//     InitialState.ZoomFactor = 1.0f;
//
//     // Save the initial state for undo -- should push onto Undo and clear Redo
//     FCanvasUtils::SaveStateForUndo(UndoStack, RedoStack, InitialState);
//
//     TestTrue(TEXT("Undo stack should contain one element after SaveStateForUndo"), UndoStack.Num() == 1);
//     TestTrue(TEXT("Redo stack should be empty after SaveStateForUndo"), RedoStack.Num() == 0);
//
//     // Change current state to simulate user making an edit
//     FCanvasState EditedState = InitialState;
//     EditedState.ZoomFactor = 2.5f;
//     EditedState.SelectedPointIndex = 7;
//
//     // Perform Undo: should push EditedState into Redo and pop InitialState back into Current
//     bool bUndid = FCanvasUtils::Undo(UndoStack, RedoStack, EditedState);
//     TestTrue(TEXT("Undo should succeed when UndoStack not empty"), bUndid);
//     TestTrue(TEXT("After Undo, current state's ZoomFactor should equal initial state's ZoomFactor"),
//              FMath::IsNearlyEqual(EditedState.ZoomFactor, 1.0f));
//
//     TestTrue(TEXT("Redo stack should contain one element after Undo"), RedoStack.Num() == 1);
//     TestTrue(TEXT("Undo stack should be empty after Undo (because we popped)"), UndoStack.Num() == 0);
//
//     // Now Redo: should restore EditedState and re-add current (InitialState) into Undo
//     bool bRedid = FCanvasUtils::Redo(UndoStack, RedoStack, EditedState);
//     TestTrue(TEXT("Redo should succeed when RedoStack not empty"), bRedid);
//     TestTrue(TEXT("After Redo, current state's ZoomFactor should equal edited state's ZoomFactor"),
//              FMath::IsNearlyEqual(EditedState.ZoomFactor, 2.5f));
//
//     TestTrue(TEXT("Undo stack should contain one element after Redo"), UndoStack.Num() == 1);
//     TestTrue(TEXT("Redo stack should be empty after Redo"), RedoStack.Num() == 0);
//
//     // Undo when UndoStack is empty should return false
//     UndoStack.Empty();
//     FCanvasState DummyState = EditedState;
//     TestFalse(TEXT("Undo should return false when UndoStack is empty"), FCanvasUtils::Undo(UndoStack, RedoStack, DummyState));
//
//     // Redo when RedoStack is empty should return false
//     RedoStack.Empty();
//     TestFalse(TEXT("Redo should return false when RedoStack is empty"), F