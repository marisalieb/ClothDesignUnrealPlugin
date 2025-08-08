#include "Canvas/CanvasPaint.h"
#include "ClothDesignCanvas.h"
#include "Rendering/DrawElements.h"


// class members
const FLinearColor GridColour(0.215f, 0.215f, 0.215f, 0.6f);
const FLinearColor GridColourSmall(0.081f, 0.081f, 0.081f, 0.4f);

// ---- POINTS AND LINES -----
const FLinearColor LineColour(0.6059f, 1.f, 0.0f, 1.f);
const FLinearColor CompletedLineColour(0.26304559f, 0.3405508f, 0.05165f, 1.f);

const FLinearColor PointColour(0.831, .0f, 1.f, 1.f);
const FLinearColor PostCurrentPointColour(0.263463f, .15208f, 0.5659f, 1.f);

const FLinearColor BezierHandleColour(0.43229f, 0.54342f, 0.0f, 1.f);
const FLinearColor CompletedBezierHandleColour(0.1025f, 0.1288f, 0.0f, 1.f);


int32 FCanvasPaint::DrawBackground(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer)
{
    if (!Canvas->BackgroundTexture.IsValid())
    {
        return Layer;
    }

    const FVector2D NativeImageSize(
        Canvas->BackgroundTexture->GetSizeX(),
        Canvas->BackgroundTexture->GetSizeY());
    
    FVector2D WorldSize = NativeImageSize * Canvas->BackgroundImageScale;
    
    FVector2D WorldTopLeft   = FVector2D::ZeroVector;

    FVector2D ScreenTopLeft = Canvas->TransformPoint(WorldTopLeft) ;// * Canvas->ZoomFactor;
    const FVector2D ScreenBottomRight = Canvas->TransformPoint(WorldTopLeft + WorldSize);
    // FVector2D ScreenSize = WorldSize * Canvas->ZoomFactor;
    const FVector2D ScreenSize = ScreenBottomRight - ScreenTopLeft;

    FSlateBrush Brush;
    Brush.SetResourceObject(Canvas->BackgroundTexture.Get());
    Brush.ImageSize = NativeImageSize;
    Brush.TintColor = FSlateColor(FLinearColor(1,1,1,.35f)); // opacity

    FPaintGeometry ImageGeo = Geo.ToPaintGeometry(
            FVector2f(ScreenSize),
            FSlateLayoutTransform(FVector2f(ScreenTopLeft)));
    
    FSlateDrawElement::MakeBox(
        OutDraw,
        Layer,
        ImageGeo,
        &Brush);
    
    return Layer + 1;
}

void FCanvasPaint::DrawGridLines(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer,
    bool bVertical,
    float Spacing,
    const FLinearColor& Color,
    bool bSkipMajor
)
{
    const FVector2D Size = Geo.GetLocalSize();

    FVector2D WorldTopLeft = Canvas->InverseTransformPoint(FVector2D::ZeroVector);
    FVector2D WorldBottomRight = Canvas->InverseTransformPoint(Size);
    
    float StartX = FMath::FloorToFloat(WorldTopLeft.X / WorldGridSpacing)*WorldGridSpacing;
    float EndX   = WorldBottomRight.X;
    float StartY = FMath::FloorToFloat(WorldTopLeft.Y / WorldGridSpacing) * WorldGridSpacing;
    float EndY   = WorldBottomRight.Y;
    
    
    for (float w = bVertical ? StartX : StartY; w <= (bVertical ? EndX : EndY); w += Spacing)
    {
        if (bSkipMajor && FMath::IsNearlyZero(FMath::Fmod(w, WorldGridSpacing), 0.01f))
            continue;

        FVector2D A_world = bVertical ? FVector2D(w, WorldTopLeft.Y) : FVector2D(WorldTopLeft.X, w);
        FVector2D B_world = bVertical ? FVector2D(w, WorldBottomRight.Y) : FVector2D(WorldBottomRight.X, w);

        FVector2D A_screen = Canvas->TransformPoint(A_world);
        FVector2D B_screen = Canvas->TransformPoint(B_world);

        FVector2D Start = bVertical ? FVector2D(A_screen.X, 0) : FVector2D(0, A_screen.Y);
        FVector2D End   = bVertical ? FVector2D(B_screen.X, Size.Y) : FVector2D(Size.X, B_screen.Y);

        FSlateDrawElement::MakeLines(
            OutDraw, Layer,
            Geo.ToPaintGeometry(),
            { Start, End },
            ESlateDrawEffect::None, Color, true, 2.0f
        );
    }
}


int32 FCanvasPaint::DrawGrid(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer)
{
    Layer = DrawBackground(Geo, OutDraw, Layer);
    
    DrawGridLines(Geo, OutDraw, Layer, true,  WorldGridSpacing,      GridColour,      false); // Vertical major
    DrawGridLines(Geo, OutDraw, Layer, false, WorldGridSpacing,      GridColour,      false); // Horizontal major
    DrawGridLines(Geo, OutDraw, Layer, true,  SubGridSpacing,        GridColourSmall, true);  // Vertical minor
    DrawGridLines(Geo, OutDraw, Layer, false, SubGridSpacing,        GridColourSmall, true);  // Horizontal minor

    return Layer + 1;
}


int32 FCanvasPaint::DrawCompletedShapes(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer)
{
    // Quick refs
    const auto& Shapes       = Canvas->CompletedShapes;
    const auto& BezierFlags  = Canvas->CompletedBezierFlags;
    const int32 NumShapes    = Shapes.Num();
    
    // --- Section 1: Draw each completed shape's edges ---
    for (int32 ShapeIdx = 0; ShapeIdx < NumShapes; ++ShapeIdx)
    {
        const auto& Shape = Shapes[ShapeIdx];
        //const auto& Flags = BezierFlags[ShapeIdx];
        const int32 NumPts = Shape.Points.Num();
        if (NumPts < 2) continue;

        const int SamplesPerSegment = 10;
        for (int32 Seg = 0; Seg < NumPts - 1; ++Seg)
        {
            float AIn = Shape.Points[Seg].InVal;
            float BIn = Shape.Points[Seg + 1].InVal;

            for (int i = 0; i < SamplesPerSegment; ++i)
            {
                float t0 = FMath::Lerp(AIn, BIn, float(i)   / SamplesPerSegment);
                float t1 = FMath::Lerp(AIn, BIn, float(i+1) / SamplesPerSegment);

                FVector2D P0 = Canvas->TransformPoint(Shape.Eval(t0));
                FVector2D P1 = Canvas->TransformPoint(Shape.Eval(t1));

                FPaintGeometry LineGeo = Geo.ToPaintGeometry();

                FSlateDrawElement::MakeLines(
                    OutDraw, Layer,
                    LineGeo,
                    TArray<FVector2f>{ FVector2f(P0), FVector2f(P1) },
                    ESlateDrawEffect::None,
                    CompletedLineColour,
                    true, 2.0f
                );
            }
        }

        // Close the loop if >2 points
        if (NumPts > 2)
        {
            FVector2D LastPt = Canvas->TransformPoint(Shape.Points.Last().OutVal);
            FVector2D FirstPt= Canvas->TransformPoint(Shape.Points[0].OutVal);
            
            FPaintGeometry LoopGeo = Geo.ToPaintGeometry();

            FSlateDrawElement::MakeLines(
                OutDraw, Layer,
                LoopGeo,
                TArray<FVector2f>{ FVector2f(LastPt), FVector2f(FirstPt) },
                ESlateDrawEffect::None,
                FLinearColor::Black,
                true, 2.0f
            );
        }

        ++Layer;
    }

    // --- Section 2: Draw each shape's points ---
    for (int32 ShapeIdx = 0; ShapeIdx < NumShapes; ++ShapeIdx)
    {
        const auto& Shape = Shapes[ShapeIdx];
        for (const auto& Pt : Shape.Points)
        {
            FVector2D Pos = Canvas->TransformPoint(Pt.OutVal);

            FVector2f BoxPos = FVector2f(Pos - FVector2D(3, 3));
            FVector2f BoxSize = FVector2f(6, 6);
            FSlateLayoutTransform LayoutTransform(BoxPos);

            FPaintGeometry BoxGeo = Geo.ToPaintGeometry(BoxSize, LayoutTransform);
            
            FSlateDrawElement::MakeBox(
                OutDraw, Layer,
                BoxGeo,
                FCoreStyle::Get().GetBrush("WhiteBrush"),
                ESlateDrawEffect::None,
                PostCurrentPointColour
            );
        }
        ++Layer;
    }

    // --- Section 3: Draw each shape's Bezier handles ---
    for (int32 ShapeIdx = 0; ShapeIdx < NumShapes; ++ShapeIdx)
    {
        const auto& Shape = Shapes[ShapeIdx];
        const auto& Flags = BezierFlags[ShapeIdx];

        for (int32 i = 0; i < Shape.Points.Num(); ++i)
        {
            if (!Flags[i]) continue; // skip N-points entirely

            const auto& Pt = Shape.Points[i];
            FVector2D World   = Pt.OutVal;
            FVector2D Screen  = Canvas->TransformPoint(World);
            FVector2D H1      = Canvas->TransformPoint(World - Pt.ArriveTangent);
            FVector2D H2      = Canvas->TransformPoint(World + Pt.LeaveTangent);
            
            FPaintGeometry HandleLineGeo = Geo.ToPaintGeometry();

            
            // Lines to handles
            FSlateDrawElement::MakeLines(
                OutDraw, Layer,
                HandleLineGeo,
                TArray<FVector2f>{ FVector2f(Screen), FVector2f(H1) },
                ESlateDrawEffect::None,
                CompletedBezierHandleColour,
                true, 1.0f
            );
            FSlateDrawElement::MakeLines(
                OutDraw, Layer,
                HandleLineGeo,
                TArray<FVector2f>{ FVector2f(Screen), FVector2f(H2) },
                ESlateDrawEffect::None,
                CompletedBezierHandleColour,
                true, 1.0f
            );

            // Boxes at handle endpoints
            for (auto& PtScreen : {H1, H2})
            {
                FVector2f BoxPos = FVector2f(PtScreen - FVector2D(3, 3));
                FVector2f BoxSize = FVector2f(6, 6);
                FSlateLayoutTransform LayoutTransform(BoxPos);

                FPaintGeometry BoxGeo = Geo.ToPaintGeometry(BoxSize, LayoutTransform);
                FSlateDrawElement::MakeBox(
                    OutDraw, Layer,
                    BoxGeo,
                    FCoreStyle::Get().GetBrush("WhiteBrush"),
                    ESlateDrawEffect::None,
                    PostCurrentPointColour
                );
            }
        }
        ++Layer;
    }

    return Layer;
}



int32 FCanvasPaint::DrawCurrentCurve(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer)
{
    const auto& CurvePoints = Canvas->CurvePoints;
    const auto& bUseBezierPerPoint = Canvas->bUseBezierPerPoint;
    
	if (CurvePoints.Points.Num() >= 2)
	{
		const int SamplesPerSegment = 10; // Smoothness

		for (int SegIndex = 0; SegIndex < CurvePoints.Points.Num() - 1; ++SegIndex)
		{
			float StartInVal = CurvePoints.Points[SegIndex].InVal;
			float EndInVal   = CurvePoints.Points[SegIndex + 1].InVal;

			for (int i = 0; i < SamplesPerSegment; ++i)
			{
				float t0 = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
				float t1   = FMath::Lerp(StartInVal, EndInVal, float(i + 1) / SamplesPerSegment);


			    FVector2D P0 = Canvas->TransformPoint(CurvePoints.Eval(t0));
			    FVector2D P1 = Canvas->TransformPoint(CurvePoints.Eval(t1));
			    
			    FPaintGeometry LineGeo = Geo.ToPaintGeometry();

                
			    FSlateDrawElement::MakeLines(
                    OutDraw, Layer,
                    LineGeo,
                    TArray<FVector2f>{ FVector2f(P0), FVector2f(P1) },
                    ESlateDrawEffect::None,
                    LineColour,
                    true, 2.0f
                );
			}
		}

		++Layer;
	}

    // Close the loop with a straight line if >2 points 
    if (CurvePoints.Points.Num() > 2)
    {
        FVector2D LastPt = Canvas->TransformPoint(CurvePoints.Points.Last().OutVal);
        FVector2D FirstPt= Canvas->TransformPoint(CurvePoints.Points[0].OutVal);

        FPaintGeometry LoopGeo = Geo.ToPaintGeometry();

        FSlateDrawElement::MakeLines(
            OutDraw, Layer,
            LoopGeo,
            TArray<FVector2f>{ FVector2f(LastPt), FVector2f(FirstPt) },
            ESlateDrawEffect::None,
            FLinearColor::Black,
            true, 2.0f
        );
    }

    ++Layer;

    
        // // --- Section 3: Draw shape's Bezier handles ---
	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
    {
        if (!bUseBezierPerPoint[i]) continue; // skip N-points entirely

       // const auto& Pt = Shape.Points[i];
	    const auto& Pt = CurvePoints.Points[i];

        FVector2D World   = Pt.OutVal;
        FVector2D Screen  = Canvas->TransformPoint(World);
        FVector2D H1      = Canvas->TransformPoint(World - Pt.ArriveTangent);
        FVector2D H2      = Canvas->TransformPoint(World + Pt.LeaveTangent);
	    
	    // FVector2D Pos = Canvas->TransformPoint(CurvePoints.Points[i].OutVal);

	   FPaintGeometry HandleLineGeo = Geo.ToPaintGeometry();

        // Lines to handles
        FSlateDrawElement::MakeLines(
            OutDraw, Layer,
            HandleLineGeo,
            TArray<FVector2f>{ FVector2f(Screen), FVector2f(H1) },
            ESlateDrawEffect::None,
            BezierHandleColour,
            true, 1.0f
        );
        FSlateDrawElement::MakeLines(
            OutDraw, Layer,
            HandleLineGeo,
            TArray<FVector2f>{ FVector2f(Screen), FVector2f(H2) },
            ESlateDrawEffect::None,
            BezierHandleColour,
            true, 1.0f
        );

        // Boxes at handle endpoints
        for (auto& PtScreen : {H1, H2})
        {
    	    // FPaintGeometry BoxGeo = Geo.ToPaintGeometry(Pos - FVector2D(3, 3), FVector2D(6, 6));
            FVector2f BoxPos = FVector2f(PtScreen - FVector2D(3, 3));
            FVector2f BoxSize = FVector2f(6, 6);
            FSlateLayoutTransform LayoutTransform(BoxPos);

            FPaintGeometry BoxGeo = Geo.ToPaintGeometry(BoxSize, LayoutTransform);

            FSlateDrawElement::MakeBox(
                OutDraw, Layer,
                BoxGeo,
                FCoreStyle::Get().GetBrush("WhiteBrush"),
                ESlateDrawEffect::None,
                PointColour
            );
        }
    }
    ++Layer;
    

    // --- Section 2: Draw shape's points ---
	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
    {
	    FVector2D Pos = Canvas->TransformPoint(CurvePoints.Points[i].OutVal);
	    
	    FVector2f BoxPos = FVector2f(Pos - FVector2D(3, 3));
	    FVector2f BoxSize = FVector2f(6, 6);
	    FSlateLayoutTransform LayoutTransform(BoxPos);

	    FPaintGeometry BoxGeo = Geo.ToPaintGeometry(BoxSize, LayoutTransform);
	    
        FSlateDrawElement::MakeBox(
            OutDraw, Layer,
            BoxGeo,
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            PointColour
        );
        ++Layer;
    }
    return Layer + 1;
}

