#include "ClothDesignCanvas.h"

#include "Misc/AutomationTest.h"
#include "Math/InterpCurve.h"
#include "UObject/Package.h"
#include "Engine/Texture2D.h"

// tests the main canvas class so SClothDesignCanvas


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothCanvas_TransformInverseTest,
    "ClothDesignCanvas.TransformInverse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothCanvas_TransformInverseTest::RunTest(const FString& Parameters)
{
    // Create canvas instance (we only call pure-data methods here)
    SClothDesignCanvas Canvas;

    // set known transform
    Canvas.ZoomFactor = 2.0f;
    Canvas.PanOffset = FVector2D(10.f, 20.f);

    const FVector2D Input(5.f, 7.f);
    FVector2D World = Canvas.TransformPoint(Input);
    FVector2D ExpectedWorld = (Input * Canvas.ZoomFactor) + Canvas.PanOffset;
    TestEqual(TEXT("TransformPoint should scale and offset"), World, ExpectedWorld);

    FVector2D Back = Canvas.InverseTransformPoint(World);
    TestEqual(TEXT("InverseTransformPoint should invert TransformPoint"), Back, Input);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothCanvas_FinaliseShapeTest,
    "ClothDesignCanvas.FinaliseShape",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothCanvas_FinaliseShapeTest::RunTest(const FString& Parameters)
{
    SClothDesignCanvas Canvas;

    // Prepare an in-progress shape with two points (the function expects at least 2)
    Canvas.CurvePoints.Points.Empty();
    Canvas.bUseBezierPerPoint.Empty();

    {
        FInterpCurvePoint<FVector2D> P0;
        P0.InVal = 0.f; P0.OutVal = FVector2D(0.f, 0.f);
        Canvas.CurvePoints.Points.Add(P0);
        Canvas.bUseBezierPerPoint.Add(false);
    }
    {
        FInterpCurvePoint<FVector2D> P1;
        P1.InVal = 1.f; P1.OutVal = FVector2D(10.f, 0.f);
        Canvas.CurvePoints.Points.Add(P1);
        Canvas.bUseBezierPerPoint.Add(false);
    }

    // Should return the new completed-shape index
    int32 NewIndex = Canvas.FinaliseCurrentShape(/*bGenerateNow=*/ false, /*OutSpawnedActors=*/ nullptr);
    TestTrue(TEXT("Finalise should return valid index"), NewIndex == Canvas.CompletedShapes.Num() - 1);
    TestEqual(TEXT("CompletedShapes should have 1 shape"), Canvas.CompletedShapes.Num(), 1);
    TestEqual(TEXT("CurvePoints should be cleared after finalize"), Canvas.CurvePoints.Points.Num(), 0);
    TestEqual(TEXT("bUseBezierPerPoint cleared after finalize"), Canvas.bUseBezierPerPoint.Num(), 0);

    // Completed shape should have two points matching the originals
    const FInterpCurve<FVector2D>& Completed = Canvas.CompletedShapes[NewIndex];
    TestEqual(TEXT("Completed shape point count"), Completed.Points.Num(), 2);
    TestEqual(TEXT("Completed[0] OutVal"), Completed.Points[0].OutVal, FVector2D(0.f, 0.f));
    TestEqual(TEXT("Completed[1] OutVal"), Completed.Points[1].OutVal, FVector2D(10.f, 0.f));

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothCanvas_StateRoundtripTest,
    "ClothDesignCanvas.StateRoundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothCanvas_StateRoundtripTest::RunTest(const FString& Parameters)
{
    SClothDesignCanvas Canvas;

    // Populate some state
    Canvas.ZoomFactor = 3.5f;
    Canvas.PanOffset = FVector2D(12.f, -4.f);
    Canvas.SelectedPointIndex = 42;

    // Add a completed shape
    FInterpCurve<FVector2D> Shape;
    {
        FInterpCurvePoint<FVector2D> P0; P0.InVal = 0.f; P0.OutVal = FVector2D(1.f, 2.f);
        Shape.Points.Add(P0);
    }
    Canvas.CompletedShapes.Add(Shape);
    Canvas.CompletedBezierFlags.Add(TArray<bool>{ true });

    // Grab state
    FCanvasState State = Canvas.GetCurrentCanvasState();

    // Mutate canvas
    Canvas.ZoomFactor = 1.f;
    Canvas.PanOffset = FVector2D::ZeroVector;
    Canvas.SelectedPointIndex = INDEX_NONE;
    Canvas.CompletedShapes.Empty();
    Canvas.CompletedBezierFlags.Empty();

    // Restore
    Canvas.RestoreCanvasState(State);

    // Verify roundtrip
    TestEqual(TEXT("ZoomFactor restored"), Canvas.ZoomFactor, State.ZoomFactor);
    TestEqual(TEXT("PanOffset restored"), Canvas.PanOffset, State.PanOffset);
    TestEqual(TEXT("SelectedPointIndex restored"), Canvas.SelectedPointIndex, INDEX_NONE); 
      // note: RestoreCanvasState sets SelectedPointIndex to INDEX_NONE after copying; we expect that behaviour
    TestEqual(TEXT("CompletedShapes restored count"), Canvas.CompletedShapes.Num(), State.CompletedShapes.Num());

    return true;
}


// Background texture & scale tests -- use concrete UTexture2D (MaterialInterface is abstract!)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothCanvas_BackgroundTextureAndScaleTest,
    "ClothDesignCanvas.BackgroundTextureAndScale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothCanvas_BackgroundTextureAndScaleTest::RunTest(const FString& Parameters)
{
    SClothDesignCanvas Canvas;

    // Ensure default
    TestTrue(TEXT("Default background path empty"), Canvas.GetSelectedTexturePath().IsEmpty());

    // Create a texture object (transient)
    UTexture2D* Tex = NewObject<UTexture2D>(GetTransientPackage(), NAME_None);
    TestNotNull(TEXT("Texture object created"), Tex);

    // Wrap as asset data and call selection
    FAssetData Asset(Tex);
    Canvas.OnBackgroundTextureSelected(Asset);

    TestTrue(TEXT("BackgroundTexture should be valid after selection"), Canvas.BackgroundTexture.IsValid());
    TestEqual(TEXT("GetSelectedTexturePath returns path"), Canvas.GetSelectedTexturePath(), Tex->GetPathName());

    // Scale change
    Canvas.OnBackgroundImageScaleChanged(2.5f);
    TestTrue(TEXT("BackgroundImageScale updated"), Canvas.GetBackgroundImageScale().IsSet());
    TestEqual(TEXT("BackgroundImageScale value"), Canvas.GetBackgroundImageScale().GetValue(), 2.5f);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothCanvas_ModeButtonTest,
    "ClothDesignCanvas.ModeButton",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothCanvas_ModeButtonTest::RunTest(const FString& Parameters)
{
    SClothDesignCanvas Canvas;

    TestEqual(TEXT("Default mode is Draw"), Canvas.GetCurrentMode(), SClothDesignCanvas::EClothEditorMode::Draw);

    Canvas.OnModeButtonClicked(SClothDesignCanvas::EClothEditorMode::Sew);
    TestEqual(TEXT("Mode switched to Sew"), Canvas.GetCurrentMode(), SClothDesignCanvas::EClothEditorMode::Sew);

    Canvas.OnModeButtonClicked(SClothDesignCanvas::EClothEditorMode::Select);
    TestEqual(TEXT("Mode switched to Select"), Canvas.GetCurrentMode(), SClothDesignCanvas::EClothEditorMode::Select);

    return true;
}
