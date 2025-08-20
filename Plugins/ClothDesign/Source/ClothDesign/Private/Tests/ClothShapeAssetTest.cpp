#include "Misc/AutomationTest.h"
#include "ClothShapeAsset.h" // adjust include path

// test defaults for FCurvePointData 
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurvePointData_DefaultsTest, 
    "ClothShapeAsset.FCurvePointData.Defaults", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCurvePointData_DefaultsTest::RunTest(const FString& Parameters)
{
    FCurvePointData Point;

    TestEqual(TEXT("Default InputKey is 0.0f"), Point.InputKey, 0.0f);
    TestTrue(TEXT("Default Position is ZeroVector"), Point.Position == FVector2D::ZeroVector);
    TestTrue(TEXT("Default ArriveTangent is ZeroVector"), Point.ArriveTangent == FVector2D::ZeroVector);
    TestTrue(TEXT("Default LeaveTangent is ZeroVector"), Point.LeaveTangent == FVector2D::ZeroVector);
    TestFalse(TEXT("Default bUseBezier is false"), Point.bUseBezier);

    return true;
}

// test defaults for FShapeData
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShapeData_DefaultsTest, 
    "ClothShapeAsset.FShapeData.Defaults", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShapeData_DefaultsTest::RunTest(const FString& Parameters)
{
    FShapeData Shape;
    TestTrue(TEXT("CompletedClothShape should be empty by default"), Shape.CompletedClothShape.Num() == 0);
    return true;
}

// test defaults for UClothShapeAsset 
IMPLEMENT_SIMPLE_AUTOMATION_TEST(UClothShapeAsset_DefaultsTest, 
    "ClothShapeAsset.UClothShapeAsset.Defaults", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool UClothShapeAsset_DefaultsTest::RunTest(const FString& Parameters)
{
    UClothShapeAsset* Asset = NewObject<UClothShapeAsset>();

    TestNotNull(TEXT("Asset should be created"), Asset);
    TestTrue(TEXT("ClothCurvePoints should be empty by default"), Asset->ClothCurvePoints.Num() == 0);
    TestTrue(TEXT("ClothShapes should be empty by default"), Asset->ClothShapes.Num() == 0);

    return true;
}

//adding and reading data
IMPLEMENT_SIMPLE_AUTOMATION_TEST(UClothShapeAsset_AddDataTest, 
    "ClothShapeAsset.UClothShapeAsset.AddData", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool UClothShapeAsset_AddDataTest::RunTest(const FString& Parameters)
{
    UClothShapeAsset* Asset = NewObject<UClothShapeAsset>();

    // Add a point
    FCurvePointData Point;
    Point.InputKey = 5.0f;
    Point.Position = FVector2D(10.0f, 20.0f);
    Asset->ClothCurvePoints.Add(Point);

    TestEqual(TEXT("ClothCurvePoints should have 1 element"), Asset->ClothCurvePoints.Num(), 1);
    TestEqual(TEXT("Stored point InputKey matches"), Asset->ClothCurvePoints[0].InputKey, 5.0f);
    TestTrue(TEXT("Stored point Position matches"), Asset->ClothCurvePoints[0].Position == FVector2D(10.0f, 20.0f));

    return true;
}
