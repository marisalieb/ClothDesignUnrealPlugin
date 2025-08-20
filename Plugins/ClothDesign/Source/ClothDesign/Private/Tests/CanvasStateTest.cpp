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

	FCanvasState StateB = StateA;

	// should be equal after direct copy
	TestTrue(TEXT("Copied states should be equal"), StateA == StateB);

	// change field slightly beyond tolerance
	StateB.ZoomFactor = 1.001f;
	TestFalse(TEXT("States should differ when ZoomFactor differs beyond tolerance"), StateA == StateB);
	

	return true;
}