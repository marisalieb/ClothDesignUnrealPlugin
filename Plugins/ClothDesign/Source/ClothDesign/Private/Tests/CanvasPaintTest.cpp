
#include "Misc/AutomationTest.h"
#include "Canvas/CanvasPaint.h"
#include "ClothDesignCanvas.h"
#include "Rendering/DrawElements.h"

// Simple mock class for Canvas
class SMockCanvas : public SClothDesignCanvas
{
public:
    SMockCanvas()
    {
        BackgroundTexture = nullptr; 
        BackgroundImageScale = 1.f;
    }

    FVector2D TransformPoint(const FVector2D& Point) const override
    {
        return Point * ZoomFactor; // simple scaling
    }

    FVector2D InverseTransformPoint(const FVector2D& Point) const override
    {
        return Point / ZoomFactor;
    }

    float ZoomFactor = 1.f;
};


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasPaintTest, "CanvasPaintTests.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasPaintTest::RunTest(const FString& Parameters)
{
    SMockCanvas MockCanvas;

    // Construct FCanvasPaint with the mock canvas
    FCanvasPaint Paint(&MockCanvas);
    
    // ---- Test BuildShortestArcSegments ----
    TSet<int32> Segments;
    Paint.BuildShortestArcSegments(1, 4, 6, Segments);
    TestTrue(TEXT("BuildShortestArcSegments outputs expected number of segments"), Segments.Num() == 3);


    return true;
}

