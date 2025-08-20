#include "ClothDesignCanvas.h"

#include "Misc/AutomationTest.h"
#include "Math/InterpCurve.h"
#include "UObject/Package.h"
#include "Engine/Texture2D.h"

// this tests the main canvas class so SClothDesignCanvas

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothCanvas_TransformInverseTest,
    "ClothDesignCanvas.TransformInverse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothCanvas_TransformInverseTest::RunTest(const FString& Parameters)
{
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

    // should return the new completed shape index
    int32 NewIndex = Canvas.FinaliseCurrentShape( false, nullptr);
    TestTrue(TEXT("Finalise should return valid index"), NewIndex == Canvas.CompletedShapes.Num() - 1);
    TestEqual(TEXT("CompletedShapes should have 1 shape"), Canvas.CompletedShapes.Num(), 1);
    TestEqual(TEXT("CurvePoints should be cleared after finalize"), Canvas.CurvePoints.Points.Num(), 0);
    TestEqual(TEXT("bUseBezierPerPoint cleared after finalize"), Canvas.bUseBezierPerPoint.Num(), 0);

    // completed shape should have two points matching the originals
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

    Canvas.ZoomFactor = 3.5f;
    Canvas.PanOffset = FVector2D(12.f, -4.f);
    Canvas.SelectedPointIndex = 42;

    // add completed shape
    FInterpCurve<FVector2D> Shape;
    {
        FInterpCurvePoint<FVector2D> P0; P0.InVal = 0.f; P0.OutVal = FVector2D(1.f, 2.f);
        Shape.Points.Add(P0);
    }
    Canvas.CompletedShapes.Add(Shape);
    Canvas.CompletedBezierFlags.Add(TArray<bool>{ true });

    FCanvasState State = Canvas.GetCurrentCanvasState();

    Canvas.ZoomFactor = 1.f;
    Canvas.PanOffset = FVector2D::ZeroVector;
    Canvas.SelectedPointIndex = INDEX_NONE;
    Canvas.CompletedShapes.Empty();
    Canvas.CompletedBezierFlags.Empty();

    Canvas.RestoreCanvasState(State);

    TestEqual(TEXT("ZoomFactor restored"), Canvas.ZoomFactor, State.ZoomFactor);
    TestEqual(TEXT("PanOffset restored"), Canvas.PanOffset, State.PanOffset);
    TestEqual(TEXT("SelectedPointIndex restored"), Canvas.SelectedPointIndex, INDEX_NONE); 
    TestEqual(TEXT("CompletedShapes restored count"), Canvas.CompletedShapes.Num(), State.CompletedShapes.Num());

    return true;
}


// background texture  
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothCanvas_BackgroundTextureAndScaleTest,
    "ClothDesignCanvas.BackgroundTextureAndScale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothCanvas_BackgroundTextureAndScaleTest::RunTest(const FString& Parameters)
{
    SClothDesignCanvas Canvas;

    TestTrue(TEXT("Default background path empty"), Canvas.GetSelectedTexturePath().IsEmpty());

    // create texture object
    UTexture2D* Tex = NewObject<UTexture2D>(GetTransientPackage(), NAME_None);
    TestNotNull(TEXT("Texture object created"), Tex);

    FAssetData Asset(Tex);
    Canvas.OnBackgroundTextureSelected(Asset);

    TestTrue(TEXT("BackgroundTexture should be valid after selection"), Canvas.BackgroundTexture.IsValid());
    TestEqual(TEXT("GetSelectedTexturePath returns path"), Canvas.GetSelectedTexturePath(), Tex->GetPathName());

    // scale change
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
