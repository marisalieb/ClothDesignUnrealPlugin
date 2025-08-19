// CanvasUtilsTests.cpp
#include "Misc/AutomationTest.h"
#include "Containers/Array.h"
#include "Math/Vector2D.h"
#include "Math/UnrealMathUtility.h"

// Replace these includes with the correct paths in the project:
#include "PatternSewingConstraint.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatternSewingConstraint_DefaultsTest, "PatternSewingConstraint.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatternSewingConstraint_DefaultsTest::RunTest(const FString& Parameters)
{
	FPatternSewingConstraint Constraint;

	// // By default, UObject* properties should be nullptr
	// TestNull(TEXT("MeshA should be null by default"), Constraint.MeshA);
	// TestNull(TEXT("MeshB should be null by default"), Constraint.MeshB);
	//
	// // Integers default to zero
	// TestEqual(TEXT("VertexIndexA should default to 0"), Constraint.VertexIndexA, 0);
	// TestEqual(TEXT("VertexIndexB should default to 0"), Constraint.VertexIndexB, 0);
	
	// Arrays should start empty
	TestTrue(TEXT("ScreenPointsA should be empty"), Constraint.ScreenPointsA.Num() == 0);
	TestTrue(TEXT("ScreenPointsB should be empty"), Constraint.ScreenPointsB.Num() == 0);

	return true;
}
