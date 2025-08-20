#include "Misc/AutomationTest.h"
#include "Canvas/CanvasPatternMerge.h"
#include "PatternMesh.h"
#include "PatternSewingConstraint.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasPatternMergeTest, 
	"CanvasPatternMerge.BasicTest", 
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasPatternMergeTest::RunTest(const FString& Parameters)
{
	FCanvasPatternMerge::TestActors.Empty();
	FCanvasPatternMerge::TestSeams.Empty();

	TWeakObjectPtr<APatternMesh> MockActor;
	FPatternSewingConstraint MockSeam;

	FCanvasPatternMerge::TestActors.Add(MockActor);
	FCanvasPatternMerge::TestSeams.Add(MockSeam);

	FCanvasPatternMerge PatternMerge;

	PatternMerge.MergeSewnGroups();
	
	TestTrue(TEXT("TestActors array should still contain mock actor"), FCanvasPatternMerge::TestActors.Num() > 0);
	TestTrue(TEXT("TestSeams array should still contain mock seam"), FCanvasPatternMerge::TestSeams.Num() > 0);
	

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeMixedTest, 
	"CanvasPatternMerge.TwoActorsAndSeams", 
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





IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCanvasPatternMergeRealisticTest,
    "CanvasPatternMerge.RealisticMergeScenario",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFCanvasPatternMergeRealisticTest::RunTest(const FString& Parameters)
{
    FCanvasPatternMerge::TestActors.Empty();
    FCanvasPatternMerge::TestSeams.Empty();

    // create 3 PatternMesh actors
    TWeakObjectPtr<APatternMesh> Actor1 = NewObject<APatternMesh>();
    TWeakObjectPtr<APatternMesh> Actor2 = NewObject<APatternMesh>();
    TWeakObjectPtr<APatternMesh> Actor3 = NewObject<APatternMesh>();

    FCanvasPatternMerge::TestActors.Add(Actor1);
    FCanvasPatternMerge::TestActors.Add(Actor2);
    FCanvasPatternMerge::TestActors.Add(Actor3);

    // create seams
    FPatternSewingConstraint Seam1;
    Seam1.MeshA = Actor1->MeshComponent;
    Seam1.MeshB = Actor2->MeshComponent;

    FPatternSewingConstraint Seam2;
    Seam2.MeshA = Actor2->MeshComponent;
    Seam2.MeshB = Actor3->MeshComponent;

    FCanvasPatternMerge::TestSeams.Add(Seam1);
    FCanvasPatternMerge::TestSeams.Add(Seam2);

    FCanvasPatternMerge PatternMerge;
    PatternMerge.MergeSewnGroups();

    TestEqual(TEXT("All actors should remain in the array"), FCanvasPatternMerge::TestActors.Num(), 3);
    TestEqual(TEXT("All seams should remain in the array"), FCanvasPatternMerge::TestSeams.Num(), 2);

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
