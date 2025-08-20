#include "Misc/AutomationTest.h"
#include "Canvas/CanvasInputHandler.h"
#include "ClothDesignCanvas.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputHandler_PanSetsFlag,
    "CanvasInputHandler.PanSetsFlag",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputHandler_PanSetsFlag::RunTest(const FString& Parameters)
{
    SClothDesignCanvas TestCanvas;
    FCanvasInputHandler Handler(&TestCanvas);

    TSet<FKey> PressedButtons;
    PressedButtons.Add(EKeys::LeftMouseButton);

    FPointerEvent FakeMouse(
        0, // user index
        0, // pointer index
        FVector2D(50, 60),  // screen pos
        FVector2D(40, 50), // last screen pos
        PressedButtons,
        EKeys::LeftMouseButton,
        0.0f, // wheel delta
        FModifierKeysState()
    );
    
    Handler.HandlePan(FakeMouse);

    TestTrue("Canvas should be panning", TestCanvas.bIsPanning);
    TestEqual("Last mouse position stored", TestCanvas.LastMousePos, FVector2D(50, 60));
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputHandler_DrawAddsPoint,
    "CanvasInputHandler.DrawAddsPoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputHandler_DrawAddsPoint::RunTest(const FString& Parameters)
{
    SClothDesignCanvas TestCanvas;
    FCanvasInputHandler Handler(&TestCanvas);
    
    TSet<FKey> PressedButtons;
    PressedButtons.Add(EKeys::LeftMouseButton);

    FGeometry Geo = FGeometry::MakeRoot(
        FVector2D(100, 100), // size of widget
        FSlateLayoutTransform() 
        );
    
    FPointerEvent Mouse(
        0,
        0,
        FVector2D(10, 20), // screen pos
        FVector2D(10, 20), // last screen pos
        PressedButtons,
        EKeys::LeftMouseButton,
        0.0f,
        FModifierKeysState()
    );
    
    Handler.HandleDraw(Geo, Mouse);

    // convert to expected point
    FVector2D Local = Geo.AbsoluteToLocal(Mouse.GetScreenSpacePosition());
    FVector2D Expected = Local / TestCanvas.ZoomFactor;

    
    TestEqual("Curve should have 1 point", TestCanvas.CurvePoints.Points.Num(), 1);
    TestEqual("Point position stored", TestCanvas.CurvePoints.Points[0].OutVal, Expected);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputHandler_SewNoPoint,
    "CanvasInputHandler.SewNoPoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputHandler_SewNoPoint::RunTest(const FString& Parameters)
{
    SClothDesignCanvas TestCanvas;
    TestCanvas.ZoomFactor = 1.f;
    FCanvasInputHandler Handler(&TestCanvas);

    // click far from any point
    FReply Reply = Handler.HandleSew(FVector2D(999,999));

    TestEqual("SeamClickState should remain None", (int32)TestCanvas.GetSewingManager().SeamClickState, (int32)ESeamClickState::None);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputHandler_SelectPoint,
    "CanvasInputHandler.SelectPoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputHandler_SelectPoint::RunTest(const FString& Parameters)
{
    SClothDesignCanvas TestCanvas;
    FInterpCurvePoint<FVector2D> Pt;
    Pt.OutVal = FVector2D(5,5);
    TestCanvas.CurvePoints.Points.Add(Pt);

    FCanvasInputHandler Handler(&TestCanvas);

    // click near the point
    Handler.HandleSelect(FVector2D(5,5));

    TestTrue("Canvas should mark a point selected", TestCanvas.bIsShapeSelected);
    TestEqual("SelectedPointIndex should be 0", TestCanvas.SelectedPointIndex, 0);
    return true;
}

