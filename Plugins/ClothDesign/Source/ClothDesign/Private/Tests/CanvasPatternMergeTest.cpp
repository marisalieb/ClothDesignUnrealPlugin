#include "Misc/AutomationTest.h"
#include "PatternCreation/PatternMerge.h"
#include "PatternMesh.h"
#include "PatternSewingConstraint.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatternMergeTest, 
	"CanvasPatternMerge.BasicTest", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatternMergeTest::RunTest(const FString& Parameters)
{
	FPatternMerge::TestActors.Empty();
	FPatternMerge::TestSeams.Empty();

	TWeakObjectPtr<APatternMesh> MockActor;
	FPatternSewingConstraint MockSeam;

	FPatternMerge::TestActors.Add(MockActor);
	FPatternMerge::TestSeams.Add(MockSeam);

	FPatternMerge PatternMerge;

	PatternMerge.MergeSewnGroups();
	
	TestTrue(TEXT("TestActors array should still contain mock actor"), FPatternMerge::TestActors.Num() > 0);
	TestTrue(TEXT("TestSeams array should still contain mock seam"), FPatternMerge::TestSeams.Num() > 0);
	

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPatternMergeMixedTest, 
	"CanvasPatternMerge.TwoActorsAndSeams", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFPatternMergeMixedTest::RunTest(const FString& Parameters)
{
	FPatternMerge::TestActors.Empty();
	FPatternMerge::TestSeams.Empty();

	FPatternMerge::TestActors.Add(nullptr);
	FPatternMerge::TestActors.Add(nullptr);

	FPatternSewingConstraint Seam1, Seam2;
	FPatternMerge::TestSeams.Add(Seam1);
	FPatternMerge::TestSeams.Add(Seam2);

	FPatternMerge PatternMerge;
	PatternMerge.MergeSewnGroups();

	TestEqual(TEXT("Number of actors should remain 2"), FPatternMerge::TestActors.Num(), 2);
	TestEqual(TEXT("Number of seams should remain 2"), FPatternMerge::TestSeams.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPatternMergeEmptyTest, 
	"CanvasPatternMerge.EmptyArrays", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFPatternMergeEmptyTest::RunTest(const FString& Parameters)
{
	FPatternMerge::TestActors.Empty();
	FPatternMerge::TestSeams.Empty();

	FPatternMerge PatternMerge;
	PatternMerge.MergeSewnGroups();

	TestEqual(TEXT("TestActors should remain empty"), FPatternMerge::TestActors.Num(), 0);
	TestEqual(TEXT("TestSeams should remain empty"), FPatternMerge::TestSeams.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPatternMergeActorsOnlyTest, 
	"CanvasPatternMerge.MultipleActorsNoSeams", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFPatternMergeActorsOnlyTest::RunTest(const FString& Parameters)
{
	FPatternMerge::TestActors.Empty();
	FPatternMerge::TestSeams.Empty();

	FPatternMerge::TestActors.Add(nullptr);
	FPatternMerge::TestActors.Add(nullptr);

	FPatternMerge PatternMerge;
	PatternMerge.MergeSewnGroups();

	TestEqual(TEXT("Number of actors should remain 2"), FPatternMerge::TestActors.Num(), 2);
	TestEqual(TEXT("Number of seams should remain 0"), FPatternMerge::TestSeams.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPatternMergeSeamsOnlyTest, 
	"CanvasPatternMerge.MultipleSeamsNoActors", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFPatternMergeSeamsOnlyTest::RunTest(const FString& Parameters)
{
	FPatternMerge::TestActors.Empty();
	FPatternMerge::TestSeams.Empty();

	FPatternSewingConstraint Seam1, Seam2;
	FPatternMerge::TestSeams.Add(Seam1);
	FPatternMerge::TestSeams.Add(Seam2);

	FPatternMerge PatternMerge;
	PatternMerge.MergeSewnGroups();

	TestEqual(TEXT("Number of actors should remain 0"), FPatternMerge::TestActors.Num(), 0);
	TestEqual(TEXT("Number of seams should remain 2"), FPatternMerge::TestSeams.Num(), 2);

	return true;
}





IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPatternMergeRealisticTest,
    "CanvasPatternMerge.RealisticMergeScenario",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFPatternMergeRealisticTest::RunTest(const FString& Parameters)
{
    FPatternMerge::TestActors.Empty();
    FPatternMerge::TestSeams.Empty();

    // create 3 PatternMesh actors
    TWeakObjectPtr<APatternMesh> Actor1 = NewObject<APatternMesh>();
    TWeakObjectPtr<APatternMesh> Actor2 = NewObject<APatternMesh>();
    TWeakObjectPtr<APatternMesh> Actor3 = NewObject<APatternMesh>();

    FPatternMerge::TestActors.Add(Actor1);
    FPatternMerge::TestActors.Add(Actor2);
    FPatternMerge::TestActors.Add(Actor3);

    // create seams
    FPatternSewingConstraint Seam1;
    Seam1.MeshA = Actor1->MeshComponent;
    Seam1.MeshB = Actor2->MeshComponent;

    FPatternSewingConstraint Seam2;
    Seam2.MeshA = Actor2->MeshComponent;
    Seam2.MeshB = Actor3->MeshComponent;

    FPatternMerge::TestSeams.Add(Seam1);
    FPatternMerge::TestSeams.Add(Seam2);

    FPatternMerge PatternMerge;
    PatternMerge.MergeSewnGroups();

    TestEqual(TEXT("All actors should remain in the array"), FPatternMerge::TestActors.Num(), 3);
    TestEqual(TEXT("All seams should remain in the array"), FPatternMerge::TestSeams.Num(), 2);

    bool bActor2Connected = false;
    for (const FPatternSewingConstraint& Seam : FPatternMerge::TestSeams)
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
