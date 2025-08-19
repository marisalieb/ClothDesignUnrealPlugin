#include "Misc/AutomationTest.h"
#include "Containers/Array.h"
#include "Math/Vector2D.h"
#include "Math/UnrealMathUtility.h"
#include "PatternSewingConstraint.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatternSewingConstraint_DefaultsTest, "PatternSewingConstraint.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatternSewingConstraint_DefaultsTest::RunTest(const FString& Parameters)
{
	FPatternSewingConstraint Constraint;

	// By default, UObject* properties should be nullptr
	TestNull(TEXT("MeshA should be null by default"), Constraint.MeshA);
	TestNull(TEXT("MeshB should be null by default"), Constraint.MeshB);
	
	// Integers default to zero
	TestEqual(TEXT("VertexIndexA should default to 0"), Constraint.VertexIndexA, 0);
	TestEqual(TEXT("VertexIndexB should default to 0"), Constraint.VertexIndexB, 0);
	
	//
	// Arrays should start empty
	TestTrue(TEXT("ScreenPointsA should be empty"), Constraint.ScreenPointsA.Num() == 0);
	TestTrue(TEXT("ScreenPointsB should be empty"), Constraint.ScreenPointsB.Num() == 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatternSewingConstraint_AssignmentTest,
	"PatternSewingConstraint.Assignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatternSewingConstraint_AssignmentTest::RunTest(const FString& Parameters)
{
	// Create a constraint and fill it
	FPatternSewingConstraint Constraint;

	Constraint.MeshA = NewObject<UProceduralMeshComponent>();
	Constraint.VertexIndexA = 5;

	Constraint.MeshB = NewObject<UProceduralMeshComponent>();
	Constraint.VertexIndexB = 42;

	Constraint.ScreenPointsA = { FVector2D(10, 20), FVector2D(30, 40) };
	Constraint.ScreenPointsB = { FVector2D(50, 60) };

	// Validate
	TestNotNull(TEXT("MeshA should be set"), Constraint.MeshA);
	TestEqual(TEXT("VertexIndexA should equal 5"), Constraint.VertexIndexA, 5);

	TestNotNull(TEXT("MeshB should be set"), Constraint.MeshB);
	TestEqual(TEXT("VertexIndexB should equal 42"), Constraint.VertexIndexB, 42);

	TestEqual(TEXT("ScreenPointsA count should match"), Constraint.ScreenPointsA.Num(), 2);
	TestEqual(TEXT("First ScreenPointsA element"), Constraint.ScreenPointsA[0], FVector2D(10, 20));

	TestEqual(TEXT("ScreenPointsB count should match"), Constraint.ScreenPointsB.Num(), 1);
	TestEqual(TEXT("First ScreenPointsB element"), Constraint.ScreenPointsB[0], FVector2D(50, 60));

	return true;
}
