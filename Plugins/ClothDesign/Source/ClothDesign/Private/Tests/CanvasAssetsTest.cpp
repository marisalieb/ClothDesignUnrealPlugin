
#include "Misc/AutomationTest.h"
#include "Canvas/CanvasAssets.h"
#include "ClothShapeAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveShapeAsset_BadPackage, 
    "CanvasAssets.SaveShapeAsset.BadPackage", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveShapeAsset_BadPackage::RunTest(const FString& Parameters)
{
    // Passing nonsense path should fail
    TArray<FInterpCurve<FVector2D>> Shapes;
    TArray<TArray<bool>> Flags;
    FInterpCurve<FVector2D> Curve;
    TArray<bool> Beziers;

    bool bResult = FCanvasAssets::SaveShapeAsset(
                  TEXT("/Engine/Invalid:Path"), // illegal colon
        TEXT("TestAsset"),
        Shapes, Flags, Curve, Beziers);

    TestFalse("Invalid package should not save", bResult);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveAndLoad_RoundTrip, 
    "CanvasAssets.SaveAndLoad", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveAndLoad_RoundTrip::RunTest(const FString& Parameters)
{
    FInterpCurve<FVector2D> Curve;
    Curve.AddPoint(0.f, FVector2D(10, 20));
    Curve.AddPoint(1.f, FVector2D(30, 40));

    TArray<FInterpCurve<FVector2D>> Shapes;
    TArray<TArray<bool>> Flags;
    {
        FInterpCurve<FVector2D> Shape;
        Shape.AddPoint(0.f, FVector2D(5,5));
        Shape.AddPoint(1.f, FVector2D(15,15));
        Shapes.Add(Shape);

        TArray<bool> ShapeFlags;
        ShapeFlags.Add(true);
        ShapeFlags.Add(false);
        Flags.Add(ShapeFlags);
    }

    TArray<bool> Beziers;
    Beziers.Add(true);
    Beziers.Add(false);

    bool bSaved = FCanvasAssets::SaveShapeAsset(
        TEXT("UnitTest"), 
        TEXT("TestingAsset"),
        Shapes, Flags, Curve, Beziers);

    TestTrue("Asset should save successfully", bSaved);

    // Load it back
    UClothShapeAsset* Loaded = LoadObject<UClothShapeAsset>(
    nullptr, TEXT("/Game/ClothDesignAssets/UnitTest/TestingAsset.TestingAsset"));
    
    
    TestNotNull("Asset should exist after save", Loaded);

    FCanvasState State;
    bool bLoaded = FCanvasAssets::LoadCanvasState(Loaded, State);
    TestTrue("Should load state", bLoaded);
    TestEqual("Loaded one completed shape", State.CompletedShapes.Num(), 1);
    TestEqual("Loaded curve points", State.CurvePoints.Points.Num(), 2);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadCanvasState_NullAsset, 
    "CanvasAssets.LoadCanvasState.NullAsset", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLoadCanvasState_NullAsset::RunTest(const FString& Parameters)
{
    FCanvasState Test;
    bool bLoaded = FCanvasAssets::LoadCanvasState(nullptr, Test);
    TestFalse("Null input should fail", bLoaded);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssetManager_Selection, 
    "CanvasAssets.AssetManager.Selection", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAssetManager_Selection::RunTest(const FString& Parameters)
{
    FCanvasAssetManager Manager;
    FCanvasState Test;

    // No asset selected
    TestEqual("Path should be empty when nothing selected", 
        Manager.GetSelectedShapeAssetPath(), FString());


    // Fake asset data that isnt UClothShapeAsset should fail
    FAssetData FakeData;
    bool bResult = Manager.OnShapeAssetSelected(FakeData, Test);
    TestFalse("Non cloth asset should fail", bResult);

    return true;
}
