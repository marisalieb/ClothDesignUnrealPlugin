#include "Misc/AutomationTest.h"
#include "Canvas/CanvasSewing.h"
#include "Canvas/CanvasMesh.h"

#include "CoreMinimal.h"
#include "PatternMesh.h"
#include "Engine/World.h"


// Test fixture for FCanvasSewing
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasSewing_FinaliseSeamDefinition_Test,
	"CanvasSewing.FinaliseSeamDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasSewing_FinaliseSeamDefinition_Test::RunTest(const FString& Parameters)
{
	// Arrange
	FCanvasSewing Sewing;

	// Fake curve points
	FInterpCurve<FVector2D> CurvePoints;
	CurvePoints.Points.Add(FInterpCurvePoint(0.f, FVector2D(0,0)));
	CurvePoints.Points.Add(FInterpCurvePoint(1.f, FVector2D(10,10)));

	TArray<FInterpCurve<FVector2D>> CompletedShapes;
	CompletedShapes.Add(CurvePoints); // Shape 0
	CompletedShapes.Add(CurvePoints); // Shape 1
	
	// Fake mesh actors
	TArray<TWeakObjectPtr<APatternMesh>> SpawnedMeshes;
	APatternMesh* MeshA = NewObject<APatternMesh>();
	APatternMesh* MeshB = NewObject<APatternMesh>();
	SpawnedMeshes.Add(MeshA);
	SpawnedMeshes.Add(MeshB);

	FClickTarget AStart; AStart.ShapeIndex = 0; AStart.PointIndex = 0;
	FClickTarget AEnd;   AEnd.ShapeIndex = 0; AEnd.PointIndex = 1;
	FClickTarget BStart; BStart.ShapeIndex = 1; BStart.PointIndex = 0;
	FClickTarget BEnd;   BEnd.ShapeIndex = 1; BEnd.PointIndex = 1;

	// Act
	Sewing.FinaliseSeamDefinitionByTargets(AStart, AEnd, BStart, BEnd, CurvePoints, CompletedShapes, SpawnedMeshes);

	// Assert
	TestEqual(TEXT("One seam should be defined"), Sewing.SeamDefinitions.Num(), 1);
	TestEqual(TEXT("AllDefinedSeams should contain one seam"), Sewing.AllDefinedSeams.Num(), 1);
	TestEqual(TEXT("First seam's ShapeA index should be 0"), Sewing.SeamDefinitions[0].ShapeA, 0);
	TestEqual(TEXT("First seam's ShapeB index should be 1"), Sewing.SeamDefinitions[0].ShapeB, 1);

	return true;
}




#if WITH_DEV_AUTOMATION_TESTS

class FCanvasSewingTestHelper
{

public:

	static void CallAlign(APatternMesh* MeshA, APatternMesh* MeshB)
    {
        FCanvasSewing::AlignSeamMeshes(MeshA, MeshB);
    }
	
	static void CallBuildAndAlignSeam(FCanvasSewing& SewingInstance, const FPatternSewingConstraint& Seam)
	{
		SewingInstance.BuildAndAlignSeam(Seam);
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlignSeamMeshesTest, 
    "CanvasSewing.AlignSeamMeshes", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAlignSeamMeshesTest::RunTest(const FString& Parameters)
{
    // ---- Minimal dynamic mesh setup ----
    APatternMesh* MeshA = NewObject<APatternMesh>();
    APatternMesh* MeshB = NewObject<APatternMesh>();

    // Add 3 vertices to MeshA
    int32 v0A = MeshA->DynamicMesh.AppendVertex(FVector3d(0,0,0));
    int32 v1A = MeshA->DynamicMesh.AppendVertex(FVector3d(10,0,0));
    int32 v2A = MeshA->DynamicMesh.AppendVertex(FVector3d(20,0,0));

    // Add 3 vertices to MeshB (slightly offset)
    int32 v0B = MeshB->DynamicMesh.AppendVertex(FVector3d(1,0,0));
    int32 v1B = MeshB->DynamicMesh.AppendVertex(FVector3d(11,0,0));
    int32 v2B = MeshB->DynamicMesh.AppendVertex(FVector3d(21,0,0));

    // Assign seam IDs (must match valid indices in the DynamicMesh)
    MeshA->LastSeamVertexIDs = { v0A, v1A, v2A };
    MeshB->LastSeamVertexIDs = { v0B, v1B, v2B };

    // Call through the helper
    FCanvasSewingTestHelper::CallAlign(MeshA, MeshB);
    
    // ---- Assertions ----
    FVector alignedA = MeshA->GetActorTransform().TransformPosition(MeshA->DynamicMesh.GetVertex(v0A));
    FVector alignedB = MeshB->GetActorTransform().TransformPosition(MeshB->DynamicMesh.GetVertex(v0B));

    TestTrue(TEXT("MeshB first seam vertex aligns with MeshA"),
        alignedA.Equals(alignedB, 0.1f));

    return true;
}






IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuildAndAlignSeamTest, "CanvasSewing.BuildAndAlignSeam", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuildAndAlignSeamTest::RunTest(const FString& Parameters)
{
	// Create two simple pattern meshes
	APatternMesh* PatternA = NewObject<APatternMesh>();
	PatternA->MeshComponent = NewObject<UProceduralMeshComponent>(PatternA);
	PatternA->BoundarySamplePoints2D.Add(FVector2f(0,0));
	PatternA->BoundarySampleVertexIDs.Add(0);
	PatternA->DynamicMesh.AppendVertex(FVector3d(0,0,0));

	APatternMesh* PatternB = NewObject<APatternMesh>();
	PatternB->MeshComponent = NewObject<UProceduralMeshComponent>(PatternB);
	PatternB->BoundarySamplePoints2D.Add(FVector2f(1,1));
	PatternB->BoundarySampleVertexIDs.Add(0);
	PatternB->DynamicMesh.AppendVertex(FVector3d(1,1,0));

	// Setup the sewing instance and register actors
	FCanvasSewing Sewing;
	Sewing.SpawnedPatternActors.Add(PatternA);
	Sewing.SpawnedPatternActors.Add(PatternB);

	// Create a simple seam
	FPatternSewingConstraint Seam;
	Seam.MeshA = PatternA->MeshComponent;
	Seam.MeshB = PatternB->MeshComponent;
	Seam.ScreenPointsA.Add(FVector2D(0,0));
	Seam.ScreenPointsA.Add(FVector2D(0,1));
	Seam.ScreenPointsB.Add(FVector2D(1,0));
	Seam.ScreenPointsB.Add(FVector2D(1,1));

	// Run the function
	FCanvasSewingTestHelper::CallBuildAndAlignSeam(Sewing, Seam);

	// Validate results
	TestTrue(TEXT("ActorA LastSeamVertexIDs populated"), PatternA->LastSeamVertexIDs.Num() > 0);
	TestTrue(TEXT("ActorB LastSeamVertexIDs populated"), PatternB->LastSeamVertexIDs.Num() > 0);

	return true;
}





#endif
