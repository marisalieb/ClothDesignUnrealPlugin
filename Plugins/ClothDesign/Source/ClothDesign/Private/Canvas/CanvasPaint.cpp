#include "Canvas/CanvasPaint.h"
#include "ClothDesignCanvas.h"
#include "Rendering/DrawElements.h"


// class members
// POINTS AND LINES
const FLinearColor FCanvasPaint::GridColour(0.215f, 0.215f, 0.215f, 0.6f);
const FLinearColor FCanvasPaint::GridColourSmall(0.081f, 0.081f, 0.081f, 0.4f);

const FLinearColor FCanvasPaint::LineColour(0.6059f, 1.f, 0.0f, 1.f);
// const FLinearColor FCanvasPaint::CompletedLineColour(0.43229f, 0.54342f, 0.0f, 1.f);
// const FLinearColor FCanvasPaint::CompletedLineColour(0.326304559f, 0.43405508f, 0.05165f, 1.f);
const FLinearColor FCanvasPaint::CompletedLineColour(0.26304559f, 0.3405508f, 0.05165f, 1.f);

const FLinearColor FCanvasPaint::PointColour(0.831, .0f, 1.f, 1.f);
const FLinearColor FCanvasPaint::PostCurrentPointColour(0.263463f, .15208f, 0.5659f, 1.f);

const FLinearColor FCanvasPaint::BezierHandleColour(0.43229f, 0.54342f, 0.0f, 1.f);
const FLinearColor FCanvasPaint::CompletedBezierHandleColour(0.1025f, 0.1288f, 0.0f, 1.f);

const FLinearColor FCanvasPaint::SewingLineColour(0.831, .0f, 1.f, 1.f);
const FLinearColor FCanvasPaint::SewingPointColour(0.9f, .0f, .240f, 1.f);

// const FLinearColor FCanvasPaint::SewingLineColour(0.99, .340f, .0f, .8f);
// const FLinearColor FCanvasPaint::SewingPointColour(1.f, .0f, .0f, 1.f);
// const FLinearColor FCanvasPaint::SewingLineColour(0.7f, .0f, 1.f, 1.f);
// const FLinearColor FCanvasPaint::SewingPointColour(0.9f, .0f, .50f, 1.f);


int32 FCanvasPaint::DrawBackground(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer) const
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

    FVector2D ScreenTopLeft = Canvas->TransformPoint(WorldTopLeft) ;
    const FVector2D ScreenBottomRight = Canvas->TransformPoint(WorldTopLeft + WorldSize);
    const FVector2D ScreenSize = ScreenBottomRight - ScreenTopLeft;

    FSlateBrush Brush;
    Brush.SetResourceObject(Canvas->BackgroundTexture.Get());
    Brush.ImageSize = NativeImageSize;
    Brush.TintColor = FSlateColor(FLinearColor(1,1,1,1)); // fully white
    FLinearColor DrawTint(1,1,1,0.25f); // actual opacity

    FPaintGeometry ImageGeo = Geo.ToPaintGeometry(
            FVector2f(ScreenSize),
            FSlateLayoutTransform(FVector2f(ScreenTopLeft)));
    
    FSlateDrawElement::MakeBox(
        OutDraw,
        Layer,
        ImageGeo,
        &Brush,
        ESlateDrawEffect::None,
        DrawTint);
    
    return Layer + 1;
}

void FCanvasPaint::DrawGridLines(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer,
    bool bVertical,
    float Spacing,
    const FLinearColor& Color,
    bool bSkipMajor) const
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
    int32 Layer) const
{
    Layer = DrawBackground(Geo, OutDraw, Layer);
    
    DrawGridLines(Geo, OutDraw, Layer, true,  WorldGridSpacing,      GridColour,      false); // Vertical major
    DrawGridLines(Geo, OutDraw, Layer, false, WorldGridSpacing,      GridColour,      false); // Horizontal major
    DrawGridLines(Geo, OutDraw, Layer, true,  SubGridSpacing,        GridColourSmall, true);  // Vertical minor
    DrawGridLines(Geo, OutDraw, Layer, false, SubGridSpacing,        GridColourSmall, true);  // Horizontal minor

    return Layer + 1;
}


// for highlighting the shapes edge segment that is sewn
// Return set of segment start indices that lie on the shortest arc from startIdx -> endIdx
void FCanvasPaint::BuildShortestArcSegments(
    int32 StartIdx, int32 EndIdx,
    int32 NumPts, TSet<int32>& OutSegments)
{
    if (NumPts <= 1) return;
    if (StartIdx == EndIdx) return;

    auto mod = [&](int x){ return (x % NumPts + NumPts) % NumPts; };

    int s = mod(StartIdx), e = mod(EndIdx);

    int distForward = (e - s + NumPts) % NumPts;
    int distBackward = (s - e + NumPts) % NumPts;

    // Choose the shorter arc (if equal, prefer forward)
    bool useForward = (distForward <= distBackward);
    int len = useForward ? distForward : distBackward;
    
    // collect segment indices for the arc
    for (int k = 0; k < len; ++k)
    {
        int segStart = useForward ? mod(s + k) : mod(s - k - 1 + NumPts); 
        // If forward: segments are (s,s+1), (s+1,s+2), ... 
        // If backward: choose segments going backward: (s-1,s), (s-2,s-1), ...
        // define segment index by the smaller index in the forward sense:
        if (useForward)
            OutSegments.Add(segStart); // segment from segStart -> segStart+1
        else
            OutSegments.Add(mod(segStart)); // segment from segStart -> segStart+1 (still same representation)
    }
}


int32 FCanvasPaint::DrawCompletedShapes(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer) const
{
    const TArray<FInterpCurve<FVector2D>>& Shapes       = Canvas->CompletedShapes;
    const TArray<TArray<bool>>& BezierFlags  = Canvas->CompletedBezierFlags;
    const int32 NumShapes    = Shapes.Num();

    const TArray<FSeamDefinition>& SeamDefs = Canvas->GetSewingManager().SeamDefinitions;

    for (int32 ShapeIdx = 0; ShapeIdx < NumShapes; ++ShapeIdx)
    {
        const FInterpCurve<FVector2D>& Shape = Shapes[ShapeIdx];
        const int32 NumPts = Shape.Points.Num();
        if (NumPts < 2) continue;

        TSet<int32> SegmentsToHighlight;
        for (const FSeamDefinition& SD : SeamDefs)
        {
            if (SD.ShapeA == ShapeIdx)
            {
                BuildShortestArcSegments(SD.EdgeA.Start, SD.EdgeA.End, NumPts, SegmentsToHighlight);
            }
            if (SD.ShapeB == ShapeIdx)
            {
                BuildShortestArcSegments(SD.EdgeB.Start, SD.EdgeB.End, NumPts, SegmentsToHighlight);
            }
        }

        const bool bClosed = (NumPts > 2);

        for (int32 Seg = 0; Seg < NumPts - 1; ++Seg)
        {
            bool bThisSegIsSewn = SegmentsToHighlight.Contains(Seg);

            FLinearColor LineCol = bThisSegIsSewn ? SewingLineColour : CompletedLineColour;

            // draw samples for this segment
            float AIn = Shape.Points[Seg].InVal;
            float BIn = Shape.Points[Seg + 1].InVal;
            constexpr int SamplesPerSegment = 10;
            
            for (int i = 0; i < SamplesPerSegment; ++i)
            {
                float t0 = FMath::Lerp(AIn, BIn, static_cast<float>(i) / SamplesPerSegment);
                float t1 = FMath::Lerp(AIn, BIn, static_cast<float>(i + 1) / SamplesPerSegment);
                
                FVector2D P0 = Canvas->TransformPoint(Shape.Eval(t0));
                FVector2D P1 = Canvas->TransformPoint(Shape.Eval(t1));
                FPaintGeometry LineGeo = Geo.ToPaintGeometry();

                FSlateDrawElement::MakeLines(
                    OutDraw, Layer,
                    LineGeo,
                    TArray<FVector2f>{ FVector2f(P0), FVector2f(P1) },
                    ESlateDrawEffect::None,
                    LineCol,
                    true, 2.0f
                );
            }
        }



        if (bClosed)
        {
            int32 seg = NumPts - 1; 
            bool bThisSegIsSewn = SegmentsToHighlight.Contains(seg);
            FLinearColor LineCol = bThisSegIsSewn ? SewingLineColour : FLinearColor::Black;

            FVector2D LastPt = Canvas->TransformPoint(Shape.Points.Last().OutVal);
            FVector2D FirstPt = Canvas->TransformPoint(Shape.Points[0].OutVal);
            FPaintGeometry LoopGeo = Geo.ToPaintGeometry();
            FSlateDrawElement::MakeLines(
                OutDraw, Layer,
                LoopGeo,
                TArray<FVector2f>{ FVector2f(LastPt), FVector2f(FirstPt) },
                ESlateDrawEffect::None,
                LineCol,
                true, 2.0f
            );
        }

        ++Layer;
    }    

    
    // Draw each shape's Bezier handles
    for (int32 ShapeIdx = 0; ShapeIdx < NumShapes; ++ShapeIdx)
    {
        const FInterpCurve<FVector2D>& Shape = Shapes[ShapeIdx];
        const TArray<bool>& Flags = BezierFlags[ShapeIdx];

        for (int32 i = 0; i < Shape.Points.Num(); ++i)
        {
            if (!Flags[i]) continue; // skip N-points entirely

            const FInterpCurvePoint<UE::Math::TVector2<double>>& Pt = Shape.Points[i];
            FVector2D World = Pt.OutVal;
            FVector2D Screen = Canvas->TransformPoint(World);
            FVector2D H1 = Canvas->TransformPoint(World - Pt.ArriveTangent);
            FVector2D H2 = Canvas->TransformPoint(World + Pt.LeaveTangent);
            
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
            for (const UE::Math::TVector2<double>& PtScreen : {H1, H2})
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
    
    
    // --- Section 2: Draw each shape's points ---
    for (int32 ShapeIdx = 0; ShapeIdx < NumShapes; ++ShapeIdx)
    {
        const FInterpCurve<FVector2D>& Shape = Shapes[ShapeIdx];
        
        for (int32 PtIdx = 0; PtIdx < Shape.Points.Num(); ++PtIdx)
        {
            bool bIsSewn = false;

            // check permanent sewn points
            if (const TSet<int32>* SewnSet = Canvas->SewnPointIndicesPerShape.Find(ShapeIdx))
            {
                if (SewnSet->Contains(PtIdx))
                    bIsSewn = true;
            }

            // Check preview points (currently being sewn)
            if (const TSet<int32>* PreviewSet = Canvas->GetSewingManager().CurrentSeamPreviewPoints.Find(ShapeIdx))
            {
                if (PreviewSet->Contains(PtIdx))
                    bIsSewn = true;
            }
            
            FVector2D Pos = Canvas->TransformPoint(Shape.Points[PtIdx].OutVal);
            FVector2f BoxPos = FVector2f(Pos - FVector2D(3, 3));
            FVector2f BoxSize = FVector2f(6, 6);
            FSlateLayoutTransform LayoutTransform(BoxPos);

            FPaintGeometry BoxGeo = Geo.ToPaintGeometry(BoxSize, LayoutTransform);
            // Choose highlight if this point is sewn
            FLinearColor UseColor = bIsSewn ?
                            SewingPointColour : PostCurrentPointColour;

            FSlateDrawElement::MakeBox(
                OutDraw, Layer,
                BoxGeo,
                FCoreStyle::Get().GetBrush("WhiteBrush"),
                ESlateDrawEffect::None,
                UseColor
            );
        }
        ++Layer;
    }

    
    return Layer;
}

int FCanvasPaint::DrawFinalisedSeamLines(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer) const
{

    const FPatternSewing& Sewing = Canvas->GetSewingManager();
    const TArray<FSeamDefinition>& Seams = Sewing.SeamDefinitions;
    

    for (int32 s = 0; s < Seams.Num(); ++s)
    {
        const FSeamDefinition& SD = Seams[s];

        // helper to get a 2D point (pattern-space) from a (shapeIndex, pointIndex)
        auto GetPatternPoint2D = [&](int32 ShapeIndex, int32 PtIdx, FVector2D& OutPt) -> bool
        {
            if (PtIdx == INDEX_NONE) return false;
            if (ShapeIndex == INDEX_NONE)
            {
                if (Canvas->CurvePoints.Points.IsValidIndex(PtIdx))
                {
                    OutPt = Canvas->CurvePoints.Points[PtIdx].OutVal;
                    return true;
                }
                return false;
            }
            if (!Canvas->CompletedShapes.IsValidIndex(ShapeIndex)) return false;
            const FInterpCurve<FVector2D>& Shape = Canvas->CompletedShapes[ShapeIndex];
            if (!Shape.Points.IsValidIndex(PtIdx)) return false;
            OutPt = Shape.Points[PtIdx].OutVal;
            return true;
        };

        FVector2D Astart2D, Bstart2D, Aend2D, Bend2D;
        bool bOkAstart = GetPatternPoint2D(SD.ShapeA, SD.EdgeA.Start, Astart2D);
        bool bOkBstart = GetPatternPoint2D(SD.ShapeB, SD.EdgeB.Start, Bstart2D);
        bool bOkAend   = GetPatternPoint2D(SD.ShapeA, SD.EdgeA.End,   Aend2D);
        bool bOkBend   = GetPatternPoint2D(SD.ShapeB, SD.EdgeB.End,   Bend2D);

        constexpr float LineThickness = 2.5f;

        bool bSelected = (Canvas->SelectedSeamIndex == s);
        FLinearColor ThisCol = bSelected ? FLinearColor::Yellow : SewingLineColour;
        float ThisThickness = bSelected ? (LineThickness + 1.5f) : LineThickness;

        
        // draw start-start line if both endpoints valid
        if (bOkAstart && bOkBstart)
        {
            FVector2D Ps = Canvas->TransformPoint(Astart2D);
            FVector2D Pe = Canvas->TransformPoint(Bstart2D);
            FPaintGeometry LineGeo = Geo.ToPaintGeometry();

            FSlateDrawElement::MakeLines(
                OutDraw, Layer,
                LineGeo,
                TArray<FVector2f>{ FVector2f(Ps), FVector2f(Pe) },
                ESlateDrawEffect::None,
                ThisCol,
                true, ThisThickness
            );
        }

        // draw end-end line if both endpoints valid
        if (bOkAend && bOkBend)
        {
            FVector2D Ps = Canvas->TransformPoint(Aend2D);
            FVector2D Pe = Canvas->TransformPoint(Bend2D);
            FPaintGeometry LineGeo = Geo.ToPaintGeometry();

            FSlateDrawElement::MakeLines(
                OutDraw, Layer,
                LineGeo,
                TArray<FVector2f>{ FVector2f(Ps), FVector2f(Pe) },
                ESlateDrawEffect::None,
                ThisCol,
                true, ThisThickness
            );
        }
        
    }
    return Layer + 1;
}


int32 FCanvasPaint::DrawCurrentShape(
    const FGeometry& Geo,
    FSlateWindowElementList& OutDraw,
    int32 Layer) const
{
    const FInterpCurve<FVector2D>& CurvePoints = Canvas->CurvePoints;
    const TArray<bool>& bUseBezierPerPoint = Canvas->bUseBezierPerPoint;
    
	if (CurvePoints.Points.Num() >= 2)
	{

		for (int SegIndex = 0; SegIndex < CurvePoints.Points.Num() - 1; ++SegIndex)
		{
			float StartInVal = CurvePoints.Points[SegIndex].InVal;
			float EndInVal   = CurvePoints.Points[SegIndex + 1].InVal;
		    constexpr int SamplesPerSegment = 10; // Smoothness

			for (int i = 0; i < SamplesPerSegment; ++i)
			{
			    float t0 = FMath::Lerp(StartInVal, EndInVal, static_cast<float>(i) / SamplesPerSegment);
			    float t1 = FMath::Lerp(StartInVal, EndInVal, static_cast<float>(i + 1) / SamplesPerSegment);

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

    
        // //  Draw shape's Bezier handles 
	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
    {
        if (!bUseBezierPerPoint[i]) continue; // skip N-points entirely

	    const FInterpCurvePoint<UE::Math::TVector2<double>>& Pt = CurvePoints.Points[i];

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
        for (const UE::Math::TVector2<double>& PtScreen : {H1, H2})
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
                PointColour
            );
        }
    }
    ++Layer;
    

    // Draw shape's points
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

