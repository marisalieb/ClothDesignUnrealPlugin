#include "Misc/AutomationTest.h"
#include "Containers/Array.h"
#include "Math/Vector2D.h"
#include "Math/UnrealMathUtility.h"

// Replace these includes with the correct paths in the project:
#include "Canvas/CanvasState.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasState_EqualityTest, "CanvasState.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasState_EqualityTest::RunTest(const FString& Parameters)
{
	FCanvasState StateA;
	StateA.SelectedPointIndex = 1;
	StateA.PanOffset = FVector2D(10.f, 20.f);
	StateA.ZoomFactor = 1.0f;

	FCanvasState StateB = StateA; // Copy

	// Should be equal after direct copy
	TestTrue(TEXT("Copied states should be equal"), StateA == StateB);

	// Change a field slightly beyond tolerance
	StateB.ZoomFactor = 1.001f; // Greater than FMath::IsNearlyEqual tolerance
	TestFalse(TEXT("States should differ when ZoomFactor differs beyond tolerance"), StateA == StateB);

	// // Restore and test with floating-point tolerance
	// StateB.ZoomFactor = 1.0f + 1.e-6f;
	// TestTrue(TEXT("States should be equal when ZoomFactor differs within tolerance"), StateA == StateB);

	return true;
}