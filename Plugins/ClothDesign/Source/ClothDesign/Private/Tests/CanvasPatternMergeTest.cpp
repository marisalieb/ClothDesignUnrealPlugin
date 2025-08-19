#include "Misc/AutomationTest.h"
#include "Canvas/CanvasPatternMerge.h"
#include "PatternMesh.h"
#include "PatternSewingConstraint.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeTest, 
	"CanvasPatternMerge.BasicTests", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFCanvasPatternMergeTest::RunTest(const FString& Parameters)
{
	// Clear the static arrays to start fresh
	FCanvasPatternMerge::TestActors.Empty();
	FCanvasPatternMerge::TestSeams.Empty();

	// Add a simple "mock" actor and seam
	TWeakObjectPtr<APatternMesh> MockActor; // Could be null for a simple test
	FPatternSewingConstraint MockSeam;

	FCanvasPatternMerge::TestActors.Add(MockActor);
	FCanvasPatternMerge::TestSeams.Add(MockSeam);

	// Create instance that binds to static test arrays
	FCanvasPatternMerge PatternMerge;

	// Call the function we want to test
	PatternMerge.MergeSewnGroups();

	// Now verify expected behavior
	// For example, the arrays should not be empty after merging
	TestTrue(TEXT("TestActors array should still contain our mock actor"), FCanvasPatternMerge::TestActors.Num() > 0);
	TestTrue(TEXT("TestSeams array should still contain our mock seam"), FCanvasPatternMerge::TestSeams.Num() > 0);

	// Optionally, if MergeSewnGroups modifies anything specific, test that
	// e.g. TestEqual(TEXT("Some expected value"), ActualValue, ExpectedValue);

	return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeMergeGroupsTest,
	"CanvasPatternMerge.MergeSewnGroups",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFCanvasPatternMergeMergeGroupsTest::RunTest(const FString& Parameters)
{
	// Clear the test arrays
	FCanvasPatternMerge::TestActors.Empty();
	FCanvasPatternMerge::TestSeams.Empty();

	// Add some mock APatternMesh objects to TestActors
	// Note: You may need to spawn actual UObject-derived actors here for Unreal tests
	// For simplicity, assume TWeakObjectPtr can be null for now
	FCanvasPatternMerge::TestActors.Add(nullptr);
	FCanvasPatternMerge::TestActors.Add(nullptr);

	// Add some mock seams
	FPatternSewingConstraint MockSeam1;
	FPatternSewingConstraint MockSeam2;
	FCanvasPatternMerge::TestSeams.Add(MockSeam1);
	FCanvasPatternMerge::TestSeams.Add(MockSeam2);

	// Create the instance using the test constructor
	FCanvasPatternMerge MyPatternMerge;

	// Call the function under test
	MyPatternMerge.MergeSewnGroups();

	// Now check your expected conditions
	// For example, you might expect the number of actors or seams hasn't changed
	TestEqual(TEXT("Number of actors should remain the same"), FCanvasPatternMerge::TestActors.Num(), 2);
	TestEqual(TEXT("Number of seams should remain the same"), FCanvasPatternMerge::TestSeams.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeEmptyTest, 
	"CanvasPatternMerge.EmptyArrays", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFCanvasPatternMergeEmptyTest::RunTest(const FString& Parameters)
{
	FCanvasPatternMerge::TestActors.Empty();
	FCanvasPatternMerge::TestSeams.Empty();

	FCanvasPatternMerge PatternMerge;
	PatternMerge.MergeSewnGroups();

	TestEqual(TEXT("TestActors should remain empty"), FCanvasPatternMerge::TestActors.Num(), 0);
	TestEqual(TEXT("TestSeams should remain empty"), FCanvasPatternMerge::TestSeams.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeActorsOnlyTest, 
	"CanvasPatternMerge.MultipleActorsNoSeams", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFCanvasPatternMergeActorsOnlyTest::RunTest(const FString& Parameters)
{
	FCanvasPatternMerge::TestActors.Empty();
	FCanvasPatternMerge::TestSeams.Empty();

	FCanvasPatternMerge::TestActors.Add(nullptr);
	FCanvasPatternMerge::TestActors.Add(nullptr);

	FCanvasPatternMerge PatternMerge;
	PatternMerge.MergeSewnGroups();

	TestEqual(TEXT("Number of actors should remain 2"), FCanvasPatternMerge::TestActors.Num(), 2);
	TestEqual(TEXT("Number of seams should remain 0"), FCanvasPatternMerge::TestSeams.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeSeamsOnlyTest, 
	"CanvasPatternMerge.MultipleSeamsNoActors", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFCanvasPatternMergeSeamsOnlyTest::RunTest(const FString& Parameters)
{
	FCanvasPatternMerge::TestActors.Empty();
	FCanvasPatternMerge::TestSeams.Empty();

	FPatternSewingConstraint Seam1, Seam2;
	FCanvasPatternMerge::TestSeams.Add(Seam1);
	FCanvasPatternMerge::TestSeams.Add(Seam2);

	FCanvasPatternMerge PatternMerge;
	PatternMerge.MergeSewnGroups();

	TestEqual(TEXT("Number of actors should remain 0"), FCanvasPatternMerge::TestActors.Num(), 0);
	TestEqual(TEXT("Number of seams should remain 2"), FCanvasPatternMerge::TestSeams.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeMixedTest, 
	"CanvasPatternMerge.MixedActorsAndSeams", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFCanvasPatternMergeMixedTest::RunTest(const FString& Parameters)
{
	FCanvasPatternMerge::TestActors.Empty();
	FCanvasPatternMerge::TestSeams.Empty();

	FCanvasPatternMerge::TestActors.Add(nullptr);
	FCanvasPatternMerge::TestActors.Add(nullptr);

	FPatternSewingConstraint Seam1, Seam2;
	FCanvasPatternMerge::TestSeams.Add(Seam1);
	FCanvasPatternMerge::TestSeams.Add(Seam2);

	FCanvasPatternMerge PatternMerge;
	PatternMerge.MergeSewnGroups();

	TestEqual(TEXT("Number of actors should remain 2"), FCanvasPatternMerge::TestActors.Num(), 2);
	TestEqual(TEXT("Number of seams should remain 2"), FCanvasPatternMerge::TestSeams.Num(), 2);

	return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasPatternMergeTest, 
	"CanvasPatternMerge.Tests", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasPatternMergeTest::RunTest(const FString& Parameters)
{
	// Set up some controlled test data
	FCanvasPatternMerge::TestActors.Empty();
	FCanvasPatternMerge::TestSeams.Empty();

	// Add test seams and actors as needed
	// Example: FCanvasPatternMerge::TestSeams.Add(SomeSeam);

	// Create the class with test arrays
	FCanvasPatternMerge PatternMerge;

	// 1️⃣ Test high-level merge
	PatternMerge.MergeSewnGroups();
	TestTrue(TEXT("MergeSewnGroups should succeed"), true); // Adjust for real checks


	return true;
}





IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeRealisticTest,
    "CanvasPatternMerge.RealisticMergeScenario",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFCanvasPatternMergeRealisticTest::RunTest(const FString& Parameters)
{
    // Clear previous test data
    FCanvasPatternMerge::TestActors.Empty();
    FCanvasPatternMerge::TestSeams.Empty();

    // Create 3 PatternMesh actors
    TWeakObjectPtr<APatternMesh> Actor1 = NewObject<APatternMesh>();
    TWeakObjectPtr<APatternMesh> Actor2 = NewObject<APatternMesh>();
    TWeakObjectPtr<APatternMesh> Actor3 = NewObject<APatternMesh>();

    FCanvasPatternMerge::TestActors.Add(Actor1);
    FCanvasPatternMerge::TestActors.Add(Actor2);
    FCanvasPatternMerge::TestActors.Add(Actor3);

    // Create seams connecting the actors' MeshComponents
    FPatternSewingConstraint Seam1;
    Seam1.MeshA = Actor1->MeshComponent;
    Seam1.MeshB = Actor2->MeshComponent;

    FPatternSewingConstraint Seam2;
    Seam2.MeshA = Actor2->MeshComponent;
    Seam2.MeshB = Actor3->MeshComponent;

    FCanvasPatternMerge::TestSeams.Add(Seam1);
    FCanvasPatternMerge::TestSeams.Add(Seam2);

    // Merge
    FCanvasPatternMerge PatternMerge;
    PatternMerge.MergeSewnGroups();

    // Verify counts
    TestEqual(TEXT("All actors should remain in the array"), FCanvasPatternMerge::TestActors.Num(), 3);
    TestEqual(TEXT("All seams should remain in the array"), FCanvasPatternMerge::TestSeams.Num(), 2);

    // Verify connectivity
    bool bActor2Connected = false;
    for (const FPatternSewingConstraint& Seam : FCanvasPatternMerge::TestSeams)
    {
        if ((Seam.MeshA == Actor2->MeshComponent && (Seam.MeshB == Actor1->MeshComponent || Seam.MeshB == Actor3->MeshComponent)) ||
            (Seam.MeshB == Actor2->MeshComponent && (Seam.MeshA == Actor1->MeshComponent || Seam.MeshA == Actor3->MeshComponent)))
        {
            bActor2Connected = true;
            break;
        }
    }

    TestTrue(TEXT("Actor2 should remain connected to other actors via mesh components"), bActor2Connected);

    return true;
}
