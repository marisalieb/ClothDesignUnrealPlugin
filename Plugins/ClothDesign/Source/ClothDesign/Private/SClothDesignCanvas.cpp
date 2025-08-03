#include "SClothDesignCanvas.h"
#include "AClothPatternMeshActor.h"


#include "CompGeom/PolygonTriangulation.h"
#include "CompGeom/Delaunay2.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshEditor.h"
#include "Remesher.h"
#include "DynamicMesh/DynamicMeshOctree3.h"

#include "CoreMinimal.h"
#include "Math/MathFwd.h"
#include "UObject/Class.h"
// #include "SClothShapeCanvas.h"
#include "Rendering/DrawElements.h"

// using namespace UE::Geometry;
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Engine/SkeletalMesh.h"
#include "ClothingSimulationInteractor.h"
#include "ClothingSimulation.h"
#include "ClothingSimulationFactory.h"
#include "ClothingAssetBase.h"
#include "SkeletalRenderPublic.h"
#include "ChaosCloth/ChaosClothingSimulation.h"
#include "ChaosCloth/ChaosClothingSimulationInteractor.h"
#include "ChaosCloth/ChaosClothingSimulationSolver.h"
#include "ChaosCloth/ChaosClothingSimulationCloth.h"
#include "ChaosCloth/ChaosClothingSimulation.h"
#include "ChaosCloth/ChaosClothingSimulation.h"          // for FClothingSimulation
#include "ChaosCloth/ChaosClothingSimulationCloth.h"     // for FClothingSimulationCloth
#include "ChaosCloth/ChaosClothingSimulationInteractor.h"
#include "ClothingSimulationFactory.h"                   // UClothingSimulationInteractor

#include "Chaos/PBDParticles.h"


void SClothDesignCanvas::Construct(const FArguments& InArgs)
{
	// Optional: set focusable, mouse events, etc.
	// Example: just an empty canvas with custom drawing
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			// You could add child widgets here, if needed
			SNullWidget::NullWidget
		]
	];
	
	this->SetEnabled(true);
	this->SetCanTick(true);
	

	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));

	// CurvePoints.Points.Empty();

}

FVector2D SClothDesignCanvas::TransformPoint(const FVector2D& Point) const
{
	return (Point * ZoomFactor) + PanOffset;
	// return (Point - PanOffset) * ZoomFactor;
}

FVector2D SClothDesignCanvas::InverseTransformPoint(const FVector2D& ScreenPoint) const
{
	return (ScreenPoint - PanOffset) / ZoomFactor;
	// return (ScreenPoint / ZoomFactor) + PanOffset;
}


int32 SClothDesignCanvas::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const_cast<SClothDesignCanvas*>(this)->LastGeometry = AllottedGeometry;
	
	// Respect parent clipping
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;


	FSlateClippingZone ClippingZone(AllottedGeometry);
	OutDrawElements.PushClip(ClippingZone);
	
	// background image
	if (BackgroundTexture.IsValid())
	{
		FVector2D NativeImageSize(BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY());

		// Apply user scale to the world size of the background image
		FVector2D WorldImageSize = NativeImageSize * BackgroundImageScale;

		// Define top-left in world space (0,0 or any other logic if needed)
		FVector2D WorldTopLeft = FVector2D(0.f, 0.f);

		// Convert world size and position to screen space
		FVector2D ScreenTopLeft = TransformPoint(WorldTopLeft);
		FVector2D ScreenSize     = WorldImageSize * ZoomFactor;
		
		FSlateBrush Brush;
		Brush.SetResourceObject(BackgroundTexture.Get());
		
		// // Brush.ImageSize = FVector2D(BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY());GetSizeY

		// FVector2D ImageSize = FVector2D(BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY()) * BackgroundImageScale;
		// Brush.ImageSize = ImageSize;
		Brush.ImageSize = NativeImageSize; // Keep the native size here, scaling happens in the draw element


		Brush.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.35f)); // opacity
		
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
		    // AllottedGeometry.ToPaintGeometry(FVector2D(0, 0), ImageSize),
		    AllottedGeometry.ToPaintGeometry(ScreenTopLeft, ScreenSize),
		    &Brush,
			ESlateDrawEffect::None,
			Brush.GetTint(InWidgetStyle)
		);
	
		++LayerId;
	}
	
	
	// Grid color
	const FLinearColor GridColor(0.215f, 0.215f, 0.215f, 0.6f);
	const FLinearColor GridColorSmall(0.081f, 0.081f, 0.081f, 0.4f);
	
	// --- Draw Grid ---
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	// const float NonScaledGridSpacing = 100.f; // spacing in pixels

	// const float GridSpacing = NonScaledGridSpacing* ZoomFactor; // spacing in pixels

	const FVector2D TopLeftScreen     = FVector2D(0, 0);
	const FVector2D BottomRightScreen = Size;

	// Convert those to “world” (canvas) coordinates
	FVector2D WorldTopLeft     = InverseTransformPoint(TopLeftScreen);
	FVector2D WorldBottomRight = InverseTransformPoint(BottomRightScreen);
	const float WorldGridSpacing = 100.0f;  // e.g. every 100 “canvas” units

	
	float StartX = FMath::FloorToFloat(WorldTopLeft.X / WorldGridSpacing) * WorldGridSpacing;
	float EndX   = WorldBottomRight.X;

	float StartY = FMath::FloorToFloat(WorldTopLeft.Y / WorldGridSpacing) * WorldGridSpacing;
	float EndY   = WorldBottomRight.Y;

	
	// Vertical lines
	for (float wx = StartX; wx <= EndX; wx += WorldGridSpacing)
	{
		// world‑space endpoints
		FVector2D A_world(wx, WorldTopLeft.Y);
		FVector2D B_world(wx, WorldBottomRight.Y);

		// to screen
		float Ax = TransformPoint(A_world).X;
		float Bx = TransformPoint(B_world).X;

		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{ FVector2D(Ax, 0), FVector2D(Bx, Size.Y) },
			ESlateDrawEffect::None, GridColor, true, 2.0f
		);
	}

	// Horizontal lines
	for (float wy = StartY; wy <= EndY; wy += WorldGridSpacing)
	{
		FVector2D A_world(WorldTopLeft.X, wy);
		FVector2D B_world(WorldBottomRight.X, wy);

		float Ay = TransformPoint(A_world).Y;
		float By = TransformPoint(B_world).Y;

		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{ FVector2D(0, Ay), FVector2D(Size.X, By) },
			ESlateDrawEffect::None, GridColor, true, 2.0f
		);
	}

	
	// --- Smaller Grid Lines ---
	const int32 NumSubdivisions = 10;
	const float SubGridSpacing = WorldGridSpacing / NumSubdivisions;

	// Vertical small lines
	for (float wx = StartX; wx <= EndX; wx += SubGridSpacing)
	{
		if (FMath::IsNearlyZero(FMath::Fmod(wx, WorldGridSpacing), 0.01f))
			continue; // Skip major lines to avoid drawing twice

		FVector2D A_world(wx, WorldTopLeft.Y);
		FVector2D B_world(wx, WorldBottomRight.Y);

		float Ax = TransformPoint(A_world).X;
		float Bx = TransformPoint(B_world).X;

		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{ FVector2D(Ax, 0), FVector2D(Bx, Size.Y) },
			ESlateDrawEffect::None, GridColorSmall, true, 1.0f
		);
	}

	// Horizontal small lines
	for (float wy = StartY; wy <= EndY; wy += SubGridSpacing)
	{
		if (FMath::IsNearlyZero(FMath::Fmod(wy, WorldGridSpacing), 0.01f))
			continue; // Skip major lines

		FVector2D A_world(WorldTopLeft.X, wy);
		FVector2D B_world(WorldBottomRight.X, wy);

		float Ay = TransformPoint(A_world).Y;
		float By = TransformPoint(B_world).Y;

		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{ FVector2D(0, Ay), FVector2D(Size.X, By) },
			ESlateDrawEffect::None, GridColorSmall, true, 1.0f
		);
	}

	// --- Advance Layer for shapes ---
	LayerId++;




	// ---- POINTS AND LINES -----
	
	// const FLinearColor LineColour(0.659f, 0.808f, 0.365f, 1.f);
	// const FLinearColor CompletedLineColour(0.4559f, 0.5508f, 0.165f, 1.f);
	//
	// const FLinearColor PointColour(0.686f, 1.f, 0.0f, 1.f);
	// const FLinearColor PostCurrentPointColour(0.355f, .59f, 0.0f, 1.f);
	//
	// const FLinearColor BezierHandleColour(0.229f, 0.342f, 0.0f, 1.f);

	// const FLinearColor LineColour(0.659f, 0.808f, 0.365f, 1.f);
	const FLinearColor LineColour(0.6059f, 1.f, 0.0f, 1.f);
	const FLinearColor CompletedLineColour(0.26304559f, 0.3405508f, 0.05165f, 1.f);

	const FLinearColor PointColour(0.831, .0f, 1.f, 1.f);
	const FLinearColor PostCurrentPointColour(0.263463f, .15208f, 0.5659f, 1.f);

	const FLinearColor BezierHandleColour(0.43229f, 0.54342f, 0.0f, 1.f);
	const FLinearColor CompletedBezierHandleColour(0.1025f, 0.1288f, 0.0f, 1.f);

	
	// Draw completed shapes first
	// Sample each Bézier‐style segment into straight‐line chunks and draw them
	// 	section 1: completed shapes, draw shape points and edge lines
	for (int32 ShapeIdx = 0; ShapeIdx < CompletedShapes.Num(); ++ShapeIdx)
	{
		const auto& Shape = CompletedShapes[ShapeIdx];
		const auto& BezierFlags = CompletedBezierFlags[ShapeIdx];
		int32 NumPts = Shape.Points.Num();
		if (NumPts < 2) continue;

		if (Shape.Points.Num() >= 2)
		{
			const int SamplesPerSegment = 10;

			for (int SegIndex = 0; SegIndex < Shape.Points.Num() - 1; ++SegIndex)
			{
				float StartInVal = Shape.Points[SegIndex].InVal;
				float EndInVal = Shape.Points[SegIndex + 1].InVal;

				for (int i = 0; i < SamplesPerSegment; ++i)
				{
					float AlphaStart = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
					float AlphaEnd   = FMath::Lerp(StartInVal, EndInVal, float(i + 1) / SamplesPerSegment);

					FVector2D P1 = TransformPoint(Shape.Eval(AlphaStart));
					FVector2D P2 = TransformPoint(Shape.Eval(AlphaEnd));

					FSlateDrawElement::MakeLines(
						OutDrawElements, LayerId,
						AllottedGeometry.ToPaintGeometry(),
						{ P1, P2 },
						ESlateDrawEffect::None,
						CompletedLineColour,
						true, 2.0f
					);
				}
			}
		}
		
		// Optional: close the shape
		if (Shape.Points.Num() > 2)
		{
			const FVector2D LastPt = TransformPoint(Shape.Points.Last().OutVal);
			const FVector2D FirstPt = TransformPoint(Shape.Points[0].OutVal);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				{ LastPt, FirstPt },
				ESlateDrawEffect::None,
				FLinearColor::Black,
				true,
				2.0f
			);
		}

		++LayerId;
	}
	
	// Draw interactive points and bezier handles for all completed shapes
	// section 2: completed shapes, bezier handles and 
	for (int32 ShapeIdx = 0; ShapeIdx < CompletedShapes.Num(); ++ShapeIdx)
	{
		const auto& Shape = CompletedShapes[ShapeIdx];
		for (int32 i = 0; i < Shape.Points.Num(); ++i)
		{
			FVector2D DrawPos = TransformPoint(Shape.Points[i].OutVal);
			FSlateDrawElement::MakeBox(
				OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(DrawPos - FVector2D(3,3), FVector2D(6,6)),
				FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None,
				PostCurrentPointColour
			);
		}
		++LayerId;
	}
	// Completed shapes handles
	for (int32 ShapeIdx = 0; ShapeIdx < CompletedShapes.Num(); ++ShapeIdx)
	{
		const auto& Shape       = CompletedShapes[ShapeIdx];
		const auto& BezierFlags = CompletedBezierFlags[ShapeIdx];
		for (int32 i = 0; i < Shape.Points.Num(); ++i)
		{
			if (!BezierFlags[i]) continue;  // skip N-points entirely
        
			const auto& Pt    = Shape.Points[i];
			FVector2D World   = Pt.OutVal;
			FVector2D DrawPt = TransformPoint(World);
			FVector2D H1      = TransformPoint(World - Pt.ArriveTangent);
			FVector2D H2      = TransformPoint(World + Pt.LeaveTangent);

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				{ DrawPt, H1 }, ESlateDrawEffect::None, CompletedBezierHandleColour, true, 1.0f);

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				{ DrawPt, H2 }, ESlateDrawEffect::None, CompletedBezierHandleColour, true, 1.0f);

			// Draw handle boxes
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(H1 - FVector2D(3, 3), FVector2D(6, 6)),
				FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, PostCurrentPointColour);

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(H2 - FVector2D(3, 3), FVector2D(6, 6)),
				FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, PostCurrentPointColour);
		}
		++LayerId;
	}
	// for (int32 ShapeIdx = 0; ShapeIdx < CompletedShapes.Num(); ++ShapeIdx)
	// {
	// 	const auto& Shape = CompletedShapes[ShapeIdx];
	// 	const auto& BezierFlags = CompletedBezierFlags[ShapeIdx];
	// 	int32 NumPts = Shape.Points.Num();
	// 	if (NumPts < 2) continue;
	// 	
	// 	for (int32 i = 0; i < Shape.Points.Num(); ++i)
	// 	{
	// 		if (!BezierFlags[i])  // <-- use the completed-shape flags
	// 		{
	// 			continue;
	// 		}
	// 		
	// 		const auto& Pt = Shape.Points[i];
	// 		FVector2D DrawPos = TransformPoint(Pt.OutVal);
	// 		// FVector2D ArriveHandle = TransformPoint(Pt.OutVal - Pt.ArriveTangent);
	// 		// FVector2D LeaveHandle  = TransformPoint(Pt.OutVal + Pt.LeaveTangent);
	//
	// 		FVector2D World  = Pt.OutVal;
	// 		FVector2D DrawPt = TransformPoint(World);
	// 		
	// 		//FLinearColor BoxColor = PostCurrentPointColour; // Or color by selection if implemented
	//
	// 		// Draw point box
	// 		FSlateDrawElement::MakeBox(
	// 			OutDrawElements,
	// 			LayerId,
	// 			AllottedGeometry.ToPaintGeometry(DrawPos - FVector2D(3, 3), FVector2D(6, 6)),
	// 			FCoreStyle::Get().GetBrush("WhiteBrush"),
	// 			ESlateDrawEffect::None,
	// 			PostCurrentPointColour
	// 		);
	// 		
	// 	
	// 		// your existing Bezier‐handle code:
	// 		FVector2D H1 = TransformPoint(World - Pt.ArriveTangent);
	// 		FVector2D H2 = TransformPoint(World + Pt.LeaveTangent);
	// 		
	// 		// Draw handle lines
	// 		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
	// 			{ DrawPt, H1 }, ESlateDrawEffect::None, BezierHandleColour, true, 1.0f);
	//
	// 		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
	// 			{ DrawPt, H2 }, ESlateDrawEffect::None, BezierHandleColour, true, 1.0f);
	//
	// 		// Draw handle boxes
	// 		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
	// 			AllottedGeometry.ToPaintGeometry(H1 - FVector2D(3, 3), FVector2D(6, 6)),
	// 			FCoreStyle::Get().GetBrush("WhiteBrush"),
	// 			ESlateDrawEffect::None, PostCurrentPointColour);
	//
	// 		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
	// 			AllottedGeometry.ToPaintGeometry(H2 - FVector2D(3, 3), FVector2D(6, 6)),
	// 			FCoreStyle::Get().GetBrush("WhiteBrush"),
	// 			ESlateDrawEffect::None, PostCurrentPointColour);
	//
	// 	}
	// }
			
			// else
			// {
			// 	// “polygon” handles pointing to neighbors:
			// 	FVector2D Prev = (i>0)            ? TransformPoint(Shape.Points[i-1].OutVal) 
			// 									: DrawPt;
			// 	FVector2D Next = (i<NumPts-1)    ? TransformPoint(Shape.Points[i+1].OutVal) 
			// 									: DrawPt;
			// 	FVector2D H1 = (DrawPt + Prev) * 0.5f;
			// 	FVector2D H2 = (DrawPt + Next) * 0.5f;
			// 	
			// 	// Draw handle lines
			// 	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			// 		{ DrawPt, H1 }, ESlateDrawEffect::None, BezierHandleColour, true, 1.0f);
			//
			// 	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			// 		{ DrawPt, H2 }, ESlateDrawEffect::None, BezierHandleColour, true, 1.0f);
			//
			// 	// Draw handle boxes
			// 	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			// 		AllottedGeometry.ToPaintGeometry(H1 - FVector2D(3, 3), FVector2D(6, 6)),
			// 		FCoreStyle::Get().GetBrush("WhiteBrush"),
			// 		ESlateDrawEffect::None, PostCurrentPointColour);
			//
			// 	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			// 		AllottedGeometry.ToPaintGeometry(H2 - FVector2D(3, 3), FVector2D(6, 6)),
			// 		FCoreStyle::Get().GetBrush("WhiteBrush"),
			// 		ESlateDrawEffect::None, PostCurrentPointColour);
			// }
			

	
	// curve points
	// 	section 3: in progress shape, draw shape points and edge lines
	if (CurvePoints.Points.Num() >= 2)
	{
		const int SamplesPerSegment = 10; // Smoothness

		for (int SegIndex = 0; SegIndex < CurvePoints.Points.Num() - 1; ++SegIndex)
		{
			float StartInVal = CurvePoints.Points[SegIndex].InVal;
			float EndInVal   = CurvePoints.Points[SegIndex + 1].InVal;

			for (int i = 0; i < SamplesPerSegment; ++i)
			{
				float AlphaStart = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
				float AlphaEnd   = FMath::Lerp(StartInVal, EndInVal, float(i + 1) / SamplesPerSegment);

				FVector2D P1 = TransformPoint(CurvePoints.Eval(AlphaStart));
				FVector2D P2 = TransformPoint(CurvePoints.Eval(AlphaEnd));

				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId,
					AllottedGeometry.ToPaintGeometry(),
					{ P1, P2 },
					ESlateDrawEffect::None,
					LineColour,
					true, 2.0f
				);
			}
		}

		++LayerId;
	}

	// close in progress shape with straight line
	if (CurvePoints.Points.Num() > 2)
	{
		const FVector2D LastPt = TransformPoint(CurvePoints.Points.Last().OutVal);
		const FVector2D FirstPt = TransformPoint(CurvePoints.Points[0].OutVal);
	
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{ LastPt, FirstPt },
			ESlateDrawEffect::None,
			FLinearColor::Black,
			true,
			2.0f
		);
		++LayerId;
	}


	// // 4. Draw interactive points as boxes (highlight selected)
	// for (int32 i = 0; i < Points.Num(); ++i)
	// {
	// 	FVector2D DrawPos = TransformPoint(Points[i]);
	// 	FLinearColor Color = (i == SelectedPointIndex) ? FLinearColor::Yellow : FLinearColor::White;
	//
	// 	FSlateDrawElement::MakeBox(
	// 		OutDrawElements,
	// 		LayerId,
	// 		AllottedGeometry.ToPaintGeometry(DrawPos - FVector2D(3, 3), FVector2D(6, 6)),
	// 		FCoreStyle::Get().GetBrush("WhiteBrush"),
	// 		ESlateDrawEffect::None,
	// 		Color
	// 	);
	// }


	// bezier points and lines
	// section 4: in progress shape, bezier handles and boxes
	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
	{
		FVector2D DrawPos = TransformPoint(CurvePoints.Points[i].OutVal);
		//FLinearColor Color = PointColour;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(DrawPos - FVector2D(3, 3), FVector2D(6, 6)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			PointColour
		);
	}
	

	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
	{
		if (!bUseBezierPerPoint[i])
		{
			// Optionally skip drawing any handle visuals
			continue;
		 }
		
		const auto& Pt = CurvePoints.Points[i];
		const FVector2D Pos = TransformPoint(Pt.OutVal);
		const FVector2D ArriveHandle = TransformPoint(Pt.OutVal - Pt.ArriveTangent);
		const FVector2D LeaveHandle  = TransformPoint(Pt.OutVal + Pt.LeaveTangent);

		// Draw lines from point to each handle
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			{ Pos, ArriveHandle }, ESlateDrawEffect::None, BezierHandleColour, true, 1.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			{ Pos, LeaveHandle }, ESlateDrawEffect::None, BezierHandleColour, true, 1.0f);

		// Draw the handles as small draggable boxes
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(ArriveHandle - FVector2D(3, 3), FVector2D(6, 6)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None, PointColour);

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(LeaveHandle - FVector2D(3, 3), FVector2D(6, 6)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None, PointColour);
	}
	
	// ensureAlways(CurvePoints.Points.Num() == bUseBezierPerPoint.Num());



	


	OutDrawElements.PopClip(); // end clipping
	return LayerId;
}
// Onpaint end


void SClothDesignCanvas::RecalculateNTangents(
	FInterpCurve<FVector2D>& Curve,
	const TArray<bool>&      bBezierFlags)
{
	int32 Num = Curve.Points.Num();
	if (Num < 2) return;

	for (int32 i = 0; i < Num; ++i)
	{
		// Only operate on N‑points
		if (bBezierFlags[i]) continue;

		// Prev Δ
		if (i > 0)
		{
			FVector2D Prev = Curve.Points[i-1].OutVal;
			FVector2D Curr = Curve.Points[i  ].OutVal;
			Curve.Points[i].ArriveTangent = (Curr - Prev) * 0.5f;
		}
		else
		{
			Curve.Points[i].ArriveTangent = FVector2D::ZeroVector;
		}

		// Next Δ
		if (i < Num - 1)
		{
			FVector2D Curr = Curve.Points[i  ].OutVal;
			FVector2D Next = Curve.Points[i+1].OutVal;
			Curve.Points[i].LeaveTangent = (Next - Curr) * 0.5f;
		}
		else
		{
			Curve.Points[i].LeaveTangent = FVector2D::ZeroVector;
		}
	}
}



TOptional<float> SClothDesignCanvas::GetBackgroundImageScale() const
{
	return BackgroundImageScale;
}

void SClothDesignCanvas::OnBackgroundImageScaleChanged(float NewScale)
{
	BackgroundImageScale = NewScale;
	// Force redraw after scale change
	this->Invalidate(EInvalidateWidget::LayoutAndVolatility);

}

FString SClothDesignCanvas::GetSelectedTexturePath() const
{
	return BackgroundTexture.IsValid() ? BackgroundTexture->GetPathName() : FString();
}

void SClothDesignCanvas::OnBackgroundTextureSelected(const FAssetData& AssetData)
{
	BackgroundTexture = Cast<UTexture2D>(AssetData.GetAsset());

	if (BackgroundTexture.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Selected texture: %s"), *BackgroundTexture->GetName());
	}
}




FReply SClothDesignCanvas::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const float ScrollDelta = MouseEvent.GetWheelDelta();
	const float ZoomDelta = 0.1f; // How fast to zoom

	// Get mouse position relative to the widget
	const FVector2D MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	// Compute the world position under the mouse before zoom
	const FVector2D WorldBeforeZoom = (MousePos - PanOffset) / ZoomFactor;

	// Adjust zoom factor
	ZoomFactor = FMath::Clamp(ZoomFactor + ScrollDelta * ZoomDelta, 0.1f, 10.0f);

	// Recalculate pan offset to keep zoom centered under mouse
	PanOffset = MousePos - WorldBeforeZoom * ZoomFactor;

	// Repaint
	Invalidate(EInvalidateWidget::LayoutAndVolatility);

	return FReply::Handled();
}



FReply SClothDesignCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMouseButtonDown fired. Mode: %d"), (int32)CurrentMode);


	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bIsPanning = true;
		LastMousePos = MouseEvent.GetScreenSpacePosition();
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D LocalClickPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D CanvasClickPos = InverseTransformPoint(LocalClickPos);

		
		// /*
		// When the user hits Enter, push CurvePoints to CompletedShapes and clear it to start a new one.
		// In rendering and triangulation, loop over both CompletedShapes and CurvePoints.
		//  */
		
		if (CurrentMode == EClothEditorMode::Draw)
		{
			SaveStateForUndo();

			// 1) add new point on current shape
			FInterpCurvePoint<FVector2D> NewPoint;
			NewPoint.InVal = CurvePoints.Points.Num();
			NewPoint.OutVal = CanvasClickPos;
			NewPoint.InterpMode = CIM_CurveAuto;

			CurvePoints.Points.Add(NewPoint);
			bUseBezierPerPoint.Add(bUseBezierPoints);
			if (!bUseBezierPoints)
			{
				RecalculateNTangents(CurvePoints, bUseBezierPerPoint);
			}
			else if (bUseBezierPoints)
			{
				CurvePoints.AutoSetTangents();

			}
			
			// 2) ALSO initialize the first/last tangents on current shape
			int32 NumPts = CurvePoints.Points.Num();
			if (NumPts >= 2)
			{
				// first point
				{
					FVector2D Delta = CurvePoints.Points[1].OutVal - CurvePoints.Points[0].OutVal;
					CurvePoints.Points[0].ArriveTangent = FVector2D::ZeroVector;
					CurvePoints.Points[0].LeaveTangent  = Delta * 0.5f;
				}
				// last point
				{
					int32 LastIdx = NumPts - 1;
					FVector2D Delta = CurvePoints.Points[LastIdx].OutVal - CurvePoints.Points[LastIdx - 1].OutVal;
					CurvePoints.Points[LastIdx].ArriveTangent = Delta * 0.5f;
					CurvePoints.Points[LastIdx].LeaveTangent  = FVector2D::ZeroVector;
				}
			}
			
			// Invalidate(EInvalidateWidgetReason::Paint); //!!
			UE_LOG(LogTemp, Warning, TEXT("Draw mode: Added point at (%f, %f)"), CanvasClickPos.X, CanvasClickPos.Y);
			return FReply::Handled();
		}
		
		// else if (CurrentMode == EClothEditorMode::Sew)
		// {
		//
		// 	//const FVector2D ClickPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		//
		// 	switch (SeamClickState)
		// 	{
		// 	case ESeamClickState::None:
		// 		AStart = CanvasClickPos;
		// 		SeamClickState = ESeamClickState::ClickedAStart;
		// 		break;
		//
		// 	case ESeamClickState::ClickedAStart:
		// 		AEnd = CanvasClickPos;
		// 		SeamClickState = ESeamClickState::ClickedAEnd;
		// 		break;
		//
		// 	case ESeamClickState::ClickedAEnd:
		// 		BStart = CanvasClickPos;
		// 		SeamClickState = ESeamClickState::ClickedBStart;
		// 		break;
		//
		// 	case ESeamClickState::ClickedBStart:
		// 		BEnd = CanvasClickPos;
		// 		SeamClickState = ESeamClickState::ClickedBEnd;
		// 		bIsSeamReady = true;
		//
		// 		// Now you can finalize the seam
		// 		FinalizeSeamDefinition(AStart, AEnd, BStart, BEnd);
		// 		SeamClickState = ESeamClickState::None; // Reset for next seam
		// 		break;
		// 	}
		//
		// 	return FReply::Handled();
		// }
		
		else if (CurrentMode == EClothEditorMode::Sew)
		{
		    // 1) Find nearest control point across all shapes + current
		    const float SelRadius = 10.0f / ZoomFactor;
		    const float BestRadiusSq = SelRadius * SelRadius;

		    int32  BestShape   = INDEX_NONE;
		    int32  BestPoint   = INDEX_NONE;
		    float  BestDistSq  = BestRadiusSq;

		    // -- Check current in-progress shape first --
		    for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
		    {
		        FVector2D Pt = CurvePoints.Points[i].OutVal;   //TransformPoint(CurvePoints.Points[i].OutVal);
		        float D2 = FVector2D::DistSquared(Pt, CanvasClickPos);
		    	
		        if (D2 < BestDistSq)
		        {
		            BestDistSq = D2;
		            BestShape  = INDEX_NONE;
		            BestPoint  = i;
		        }
		    }

		    // -- Then each completed shape --
		    for (int32 s = 0; s < CompletedShapes.Num(); ++s)
		    {
		        for (int32 i = 0; i < CompletedShapes[s].Points.Num(); ++i)
		        {
		            FVector2D Pt = CompletedShapes[s].Points[i].OutVal;   //TransformPoint(CompletedShapes[s].Points[i].OutVal);
		            float D2 = FVector2D::DistSquared(Pt, CanvasClickPos);
		        	
		            if (D2 < BestDistSq)
		            {
		                BestDistSq = D2;
		                BestShape  = s;
		                BestPoint  = i;
		            }
		        }
		    }

		    // If we didn't hit anything, just consume the click and exit
		    if (BestPoint == INDEX_NONE)
		    {
		        UE_LOG(LogTemp, Verbose, TEXT("Sew click missed all control points."));
		        return FReply::Handled();
		    }

		    // 2) Advance our 4-click state, storing shape+point each time
		    switch (SeamClickState)
		    {
		    case ESeamClickState::None:
		        AStartTarget = { BestShape, BestPoint };
		        SeamClickState = ESeamClickState::ClickedAStart;
		        UE_LOG(LogTemp, Log, TEXT("Sew: AStart = [%d,%d]"), BestShape, BestPoint);
		        break;

		    case ESeamClickState::ClickedAStart:
		        AEndTarget = { BestShape, BestPoint };
		        SeamClickState = ESeamClickState::ClickedAEnd;
		        UE_LOG(LogTemp, Log, TEXT("Sew: AEnd   = [%d,%d]"), BestShape, BestPoint);
		        break;

		    case ESeamClickState::ClickedAEnd:
		        BStartTarget = { BestShape, BestPoint };
		        SeamClickState = ESeamClickState::ClickedBStart;
		        UE_LOG(LogTemp, Log, TEXT("Sew: BStart = [%d,%d]"), BestShape, BestPoint);
		        break;

		    case ESeamClickState::ClickedBStart:
		        BEndTarget = { BestShape, BestPoint };
		        SeamClickState = ESeamClickState::ClickedBEnd;
		        bIsSeamReady = true;
		        UE_LOG(LogTemp, Log, TEXT("Sew: BEnd   = [%d,%d]"), BestShape, BestPoint);

		        // 3) Finalize: now you have (shape,index) for all four clicks
		        FinalizeSeamDefinitionByTargets(AStartTarget, AEndTarget, BStartTarget, BEndTarget);

		        // Reset for next seam
		        SeamClickState = ESeamClickState::None;
		        break;
		    }

		    return FReply::Handled();
		}

		else if (CurrentMode == EClothEditorMode::Select)
		{
			// Check completed shapes first
			for (int32 ShapeIndex = 0; ShapeIndex < CompletedShapes.Num(); ++ShapeIndex)
			{
				const auto& Shape = CompletedShapes[ShapeIndex];

				for (int32 i = 0; i < Shape.Points.Num(); ++i)
				{
					const auto& Pt = Shape.Points[i];
					// const FVector2D PointPos = Pt.OutVal;
					// FVector2D Arrive = PointPos - Pt.ArriveTangent;
					// FVector2D Leave = PointPos + Pt.LeaveTangent;
					FVector2D World = Pt.OutVal;
					//const auto& Shape = CompletedShapes[i];
					const auto& BezierFlags = CompletedBezierFlags[ShapeIndex];
					int32 NumPts = Shape.Points.Num();
					
					// arrive handle:
					FVector2D Arrive = BezierFlags[i]
						? (World - Pt.ArriveTangent)
						: (World + (i>0 ? Shape.Points[i-1].OutVal : World)) * 0.5f;

					// leave handle:
					FVector2D Leave = BezierFlags[i]
						? (World + Pt.LeaveTangent)
						: (World + (i<NumPts-1 ? Shape.Points[i+1].OutVal : World)) * 0.5f;
					

					if (!BezierFlags[i]){continue;}

					if (FVector2D::Distance(CanvasClickPos, Arrive) < TangentHandleRadius / ZoomFactor)
					{
						SaveStateForUndo();
						SelectedShapeIndex = ShapeIndex;
						SelectedPointIndex = i;
						SelectedTangentHandle = ETangentHandle::Arrive;
						bIsDraggingTangent = true;

						return FReply::Handled()
									.CaptureMouse(SharedThis(this))
						            .SetUserFocus(AsShared(), EFocusCause::Mouse);
;
					}
					else if (FVector2D::Distance(CanvasClickPos, Leave) < TangentHandleRadius / ZoomFactor)
					{
						SaveStateForUndo();
						SelectedShapeIndex = ShapeIndex;
						SelectedPointIndex = i;
						SelectedTangentHandle = ETangentHandle::Leave;
						bIsDraggingTangent = true;

						return FReply::Handled()
							.CaptureMouse(SharedThis(this))
							.SetUserFocus(AsShared(), EFocusCause::Mouse);
						;
					}
				}
			}

			// Also check current in-progress shape (if needed)
			for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
			{
				const auto& Pt = CurvePoints.Points[i];
				const FVector2D PointPos = Pt.OutVal;
				FVector2D Arrive = PointPos - Pt.ArriveTangent;
				FVector2D Leave = PointPos + Pt.LeaveTangent;
	
				if (!bUseBezierPerPoint[i])
				{
					continue; // Don't allow selecting N-point handles
				}
				
				if (FVector2D::Distance(CanvasClickPos, Arrive) < TangentHandleRadius / ZoomFactor)
				{
					SaveStateForUndo();
					SelectedShapeIndex = INDEX_NONE; // current shape
					SelectedPointIndex = i;
					SelectedTangentHandle = ETangentHandle::Arrive;
					bIsDraggingTangent = true;

					return FReply::Handled()
							.CaptureMouse(SharedThis(this))
							.SetUserFocus(AsShared(), EFocusCause::Mouse);
					
				}
				else if (FVector2D::Distance(CanvasClickPos, Leave) < TangentHandleRadius / ZoomFactor)
				{
					SaveStateForUndo();
					SelectedShapeIndex = INDEX_NONE;
					SelectedPointIndex = i;
					SelectedTangentHandle = ETangentHandle::Leave;
					bIsDraggingTangent = true;

					return FReply::Handled()
							.CaptureMouse(SharedThis(this))
							.SetUserFocus(AsShared(), EFocusCause::Mouse);
				}
			}

			const float SelectionRadius = 10.0f;

			// Check completed shapes
			for (int32 ShapeIndex = 0; ShapeIndex < CompletedShapes.Num(); ++ShapeIndex)
			{
				const auto& Shape = CompletedShapes[ShapeIndex];

				for (int32 i = 0; i < Shape.Points.Num(); ++i)
				{
					FVector2D WorldPoint = Shape.Points[i].OutVal;
					if (FVector2D::Distance(WorldPoint, CanvasClickPos) < SelectionRadius / ZoomFactor)
					{
						SaveStateForUndo();
						SelectedShapeIndex = ShapeIndex;
						SelectedPointIndex = i;
						bIsShapeSelected = true;
						bIsDraggingPoint = true;

						UE_LOG(LogTemp, Warning, TEXT("Selected point %d in shape %d"), i, ShapeIndex);

						return FReply::Handled()
							.CaptureMouse(SharedThis(this))
							.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
					}
				}
			}

			// Check current in-progress shape
			for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
			{
				FVector2D WorldPoint = CurvePoints.Points[i].OutVal;
				if (FVector2D::Distance(WorldPoint, CanvasClickPos) < SelectionRadius / ZoomFactor)
				{
					SaveStateForUndo();
					SelectedShapeIndex = INDEX_NONE;  // current shape
					SelectedPointIndex = i;
					bIsShapeSelected = true;
					bIsDraggingPoint = true;

					UE_LOG(LogTemp, Warning, TEXT("Selected point %d in current shape"), i);

					return FReply::Handled()
						.CaptureMouse(SharedThis(this))
						.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
				}
			}

			return FReply::Handled();
		}
		
		
		



		if (Points.Num() < 2)
			return FReply::Handled(); // not enough points

			
		// Clicked on empty space; DESELECT
		SelectedPointIndex = INDEX_NONE;
		bIsDraggingPoint = false;
		bIsShapeSelected = false;
		UE_LOG(LogTemp, Warning, TEXT("Deselected all"));
		return FReply::Handled()
			.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
		
	}

	return FReply::Unhandled();
}


void SClothDesignCanvas::FinalizeSeamDefinitionByTargets(
	const FClickTarget& AStart,
	const FClickTarget& AEnd,
	const FClickTarget& BStart,
	const FClickTarget& BEnd)
{
	const int32 NumSeamPoints = 10;
	TArray<FVector2D> PointsA, PointsB;

	auto GetPt = [&](const FClickTarget& T) {
		if (T.ShapeIndex == INDEX_NONE)
			return CurvePoints.Points[T.PointIndex].OutVal;
		else
			return CompletedShapes[T.ShapeIndex].Points[T.PointIndex].OutVal;
	};

	FVector2D A1 = GetPt(AStart), A2 = GetPt(AEnd);
	FVector2D B1 = GetPt(BStart), B2 = GetPt(BEnd);

	for (int32 i = 0; i < NumSeamPoints; ++i)
	{
		float Alpha = float(i) / (NumSeamPoints - 1);
		PointsA.Add(FMath::Lerp(A1, A2, Alpha));
		PointsB.Add(FMath::Lerp(B1, B2, Alpha));
	}

	FPatternSewingConstraint NewSeam;
	NewSeam.ScreenPointsA = PointsA;
	NewSeam.ScreenPointsB = PointsB;
	AllDefinedSeams.Add(NewSeam);

	UE_LOG(LogTemp, Log, TEXT("Seam finalized between [%d,%d] and [%d,%d]"),
		AStart.ShapeIndex, AStart.PointIndex,
		BStart.ShapeIndex, BStart.PointIndex);
}





FVector2D SClothDesignCanvas::GetPointFromShape(int32 ShapeIndex, int32 PointIndex)
{
	if (ShapeIndex == INDEX_NONE)
	{
		return CurvePoints.Points[PointIndex].OutVal;
	}
	else
	{
		return CompletedShapes[ShapeIndex].Points[PointIndex].OutVal;
	}
}

void SClothDesignCanvas::FinalizeSeamDefinitionByIndex(
	FClickTarget AStart, FClickTarget AEnd,
	FClickTarget BStart, FClickTarget BEnd)
{
	const int32 NumSeamPoints = 10;
	TArray<FVector2D> PointsA, PointsB;

	FVector2D A1 = GetPointFromShape(AStart.ShapeIndex, AStart.PointIndex);
	FVector2D A2 = GetPointFromShape(AEnd.ShapeIndex, AEnd.PointIndex);
	FVector2D B1 = GetPointFromShape(BStart.ShapeIndex, BStart.PointIndex);
	FVector2D B2 = GetPointFromShape(BEnd.ShapeIndex, BEnd.PointIndex);

	for (int32 i = 0; i < NumSeamPoints; ++i)
	{
		float Alpha = float(i) / (NumSeamPoints - 1);
		PointsA.Add(FMath::Lerp(A1, A2, Alpha));
		PointsB.Add(FMath::Lerp(B1, B2, Alpha));
	}

	FPatternSewingConstraint NewSeam;
	NewSeam.ScreenPointsA = PointsA;
	NewSeam.ScreenPointsB = PointsB;
	AllDefinedSeams.Add(NewSeam);

	UE_LOG(LogTemp, Log, TEXT("Seam defined with %d points per side."), NumSeamPoints);
	UE_LOG(LogTemp, Log, TEXT("AStart: (%f, %f), AEnd: (%f, %f)"), A1.X, A1.Y, A2.X, A2.Y);
	UE_LOG(LogTemp, Log, TEXT("BStart: (%f, %f), BEnd: (%f, %f)"), B1.X, B1.Y, B2.X, B2.Y);
}


// void SClothDesignCanvas::FinalizeSeamDefinitionByIndex(
// 	int32 AStartIdx, int32 AEndIdx,
// 	int32 BStartIdx, int32 BEndIdx)
// {
// 	// Sample between those two points in your curve:
// 	const int32 NumSeamPoints = 10;
// 	TArray<FVector2D> PointsA, PointsB;
//
// 	//auto& PointsRef = /* same logic to pick CurvePoints or CompletedShapes */;
// 	// 1) Choose the right point list
// 	TArray<FInterpCurvePoint<FVector2D>>& PointArray =
// 		(SelectedShapeIndex == INDEX_NONE)
// 			? CurvePoints.Points
// 			: CompletedShapes[SelectedShapeIndex].Points;
// 	FVector2D A1 = PointArray[AStartIdx].OutVal;
// 	FVector2D A2 = PointArray[AEndIdx].OutVal;
// 	FVector2D B1 = PointArray[BStartIdx].OutVal;
// 	FVector2D B2 = PointArray[BEndIdx].OutVal;
//
// 	for (int32 i = 0; i < NumSeamPoints; ++i)
// 	{
// 		float Alpha = float(i) / (NumSeamPoints - 1);
// 		PointsA.Add(FMath::Lerp(A1, A2, Alpha));
// 		PointsB.Add(FMath::Lerp(B1, B2, Alpha));
// 	}
//
// 	FPatternSewingConstraint NewSeam;
// 	NewSeam.ScreenPointsA = PointsA;
// 	NewSeam.ScreenPointsB = PointsB;
// 	AllDefinedSeams.Add(NewSeam);
//
// 	UE_LOG(LogTemp, Log, TEXT(
// 	   "SeamIndices: A[%d→%d], B[%d→%d]"), 
// 	   AStartIdx, AEndIdx, BStartIdx, BEndIdx
// 	);
// }

FReply SClothDesignCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsPanning)
	{
		FVector2D CurrentMousePos = MouseEvent.GetScreenSpacePosition();
		FVector2D Delta = (CurrentMousePos - LastMousePos) / ZoomFactor; // Normalize for zoom

		float PanSpeedMultiplier = 2.f; // faster panning 
		PanOffset += Delta * PanSpeedMultiplier;
		LastMousePos = CurrentMousePos;

		Invalidate(EInvalidateWidget::Paint); // Trigger repaint
		return FReply::Handled();
	}

	
	if ((CurrentMode == EClothEditorMode::Move || CurrentMode == EClothEditorMode::Select) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D CanvasMousePos = InverseTransformPoint(LocalMousePos);
		UE_LOG(LogTemp, Warning, TEXT("CanvasClick: %s"), *CanvasMousePos.ToString());

		if (bIsDraggingTangent && SelectedPointIndex != INDEX_NONE)
		{
			
			if (SelectedShapeIndex == INDEX_NONE)
			{
				// // Current shape
				// FVector2D PointPos = CurvePoints.Points[SelectedPointIndex].OutVal;
				// FVector2D Delta = CanvasMousePos - PointPos;
				//
				// if (SelectedTangentHandle == ETangentHandle::Arrive)
				// {
				// 	// If separate mode, only move the Arrive handle.
				// 	// Otherwise, move both simultaneously.
				// 	CurvePoints.Points[SelectedPointIndex].ArriveTangent = -Delta;
				// 	if (!bSeparateTangents)
				// 	{
				// 		CurvePoints.Points[SelectedPointIndex].LeaveTangent = -Delta;
				// 	}
				// }
				// else // Leave handle
				// {
				// 	CurvePoints.Points[SelectedPointIndex].LeaveTangent = Delta;
				// 	if (!bSeparateTangents)
				// 	{
				// 		CurvePoints.Points[SelectedPointIndex].ArriveTangent = Delta;
				// 	}
				// }
				// In-progress point (CurvePoints)
				FInterpCurvePoint<FVector2D>& Pt = CurvePoints.Points[SelectedPointIndex];
				FVector2D PointPos = Pt.OutVal;
				FVector2D Delta = CanvasMousePos - PointPos;

				bool bIsBezierPoint = bUseBezierPerPoint[SelectedPointIndex];
				bool bLinkTangents  = bIsBezierPoint && !bSeparateTangents;

				if (SelectedTangentHandle == ETangentHandle::Arrive)
				{
					Pt.ArriveTangent = -Delta;
					if (bLinkTangents)
					{
						// Pt.LeaveTangent = Pt.ArriveTangent;
						float LeaveLen = Pt.LeaveTangent.Size();
						FVector2D OppositeDir = -Delta.GetSafeNormal();
						Pt.LeaveTangent = OppositeDir * LeaveLen;
					}
				}
				else // Leave
				{
					Pt.LeaveTangent = Delta;
					if (bLinkTangents)
					{
						// Pt.ArriveTangent = Pt.LeaveTangent;
						float ArriveLen = Pt.ArriveTangent.Size();  // preserve original length
						FVector2D OppositeDir = Delta.GetSafeNormal();
						Pt.ArriveTangent = OppositeDir * ArriveLen;
					}
				}

			}
			else
			{
				// Completed shape
				FInterpCurvePoint<FVector2D>& Pt = CompletedShapes[SelectedShapeIndex].Points[SelectedPointIndex];
				FVector2D PointPos = Pt.OutVal;
				FVector2D Delta = CanvasMousePos - PointPos;
				
				auto& BezierFlags = CompletedBezierFlags[SelectedShapeIndex];
				if (!BezierFlags[SelectedPointIndex])
				{
					BezierFlags[SelectedPointIndex] = true;
				}
				bool bIsBezierPoint = CompletedBezierFlags[SelectedShapeIndex][SelectedPointIndex];
				bool bLinkTangents   = bIsBezierPoint && !bSeparateTangents;

				if (SelectedTangentHandle == ETangentHandle::Arrive)
				{
					// Always set the arrive tangent
					Pt.ArriveTangent = -Delta;
					// Link it to the leave tangent only if we decided to link
					if (bLinkTangents)
					{
						// Pt.LeaveTangent = Pt.ArriveTangent;
						float LeaveLen = Pt.LeaveTangent.Size();
						FVector2D OppositeDir = -Delta.GetSafeNormal();
						Pt.LeaveTangent = OppositeDir * LeaveLen;
					}
				}
				else // ETangentHandle::Leave
				{
					Pt.LeaveTangent = Delta;
					if (bLinkTangents)
					{
						// Pt.ArriveTangent = Pt.LeaveTangent;
						float ArriveLen = Pt.ArriveTangent.Size();  // preserve original length
						FVector2D OppositeDir = Delta.GetSafeNormal();
						Pt.ArriveTangent = OppositeDir * ArriveLen;
					}
				}

				// if (SelectedTangentHandle == ETangentHandle::Arrive)
				// {
				// 	Pt.ArriveTangent = -Delta;
				// 	
				// 	if (!bSeparateTangents)
				// 	{
				// 		Pt.LeaveTangent = -Delta;
				// 	}
				// }
				// else // Leave
				// {
				// 	Pt.LeaveTangent = Delta;
				// 	
				// 	if (!bSeparateTangents)
				// 	{
				// 		Pt.ArriveTangent = Delta;
				// 	}
				//
				// }
			}

			UE_LOG(LogTemp, Warning, TEXT("Dragging tangent for point %d in shape %d"), SelectedPointIndex, SelectedShapeIndex);
			return FReply::Handled();
		}

		// dragging shape points
		if (bIsDraggingPoint && SelectedPointIndex != INDEX_NONE)
		{
			if (SelectedShapeIndex == INDEX_NONE)
			{
				CurvePoints.Points[SelectedPointIndex].OutVal = CanvasMousePos;
				// In‑progress curve:
				if (!bUseBezierPerPoint[SelectedPointIndex])
					RecalculateNTangents(CurvePoints, bUseBezierPerPoint);
			}
			else
			{
				CompletedShapes[SelectedShapeIndex].Points[SelectedPointIndex].OutVal = CanvasMousePos;
				if (!CompletedBezierFlags[SelectedShapeIndex][SelectedPointIndex])
					RecalculateNTangents(
						CompletedShapes[SelectedShapeIndex],
						CompletedBezierFlags[SelectedShapeIndex]
					);
			}

			UE_LOG(LogTemp, Warning, TEXT("Dragging point %d in shape %d"), SelectedPointIndex, SelectedShapeIndex);
			return FReply::Handled();
		}
		
	}

	return FReply::Unhandled();
}


FReply SClothDesignCanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bIsPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	// if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
	{
		bool WasDragging = bIsDraggingPoint || bIsDraggingTangent;

		bIsDraggingPoint = false;
		bIsDraggingTangent = false;
		// SelectedPointIndex = INDEX_NONE;
		SelectedTangentHandle = ETangentHandle::None;
		// return FReply::Handled().ReleaseMouseCapture();
		
		if (WasDragging)
		{
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Unhandled();
}



FReply SClothDesignCanvas::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("Key pressed: %s"), *InKeyEvent.GetKey().ToString());

	const FKey Key = InKeyEvent.GetKey();


	if (Key == EKeys::Delete || Key == EKeys::BackSpace)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to deelete point: %d"), SelectedPointIndex);
		
		if (SelectedPointIndex != INDEX_NONE)
		{
			SaveStateForUndo();

			if (SelectedShapeIndex == INDEX_NONE)
			{
				// Deleting from current shape
				if (SelectedPointIndex < CurvePoints.Points.Num())
				{
					CurvePoints.Points.RemoveAt(SelectedPointIndex);
					bUseBezierPerPoint.RemoveAt(SelectedPointIndex);
					CurvePoints.AutoSetTangents();
				}
			}
			else if (SelectedShapeIndex >= 0 && SelectedShapeIndex < CompletedShapes.Num())
			{
				// Deleting from completed shape
				if (SelectedPointIndex < CompletedShapes[SelectedShapeIndex].Points.Num())
				{
					CompletedShapes[SelectedShapeIndex].Points.RemoveAt(SelectedPointIndex);
					CompletedBezierFlags[SelectedShapeIndex].RemoveAt(SelectedPointIndex);
					CompletedShapes[SelectedShapeIndex].AutoSetTangents();
				}
			}

			SelectedPointIndex = INDEX_NONE;
			SelectedShapeIndex = INDEX_NONE;

			Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
			return FReply::Handled();
		}


		return FReply::Unhandled();
	}


	
	if (Key == EKeys::One)
	{
		CurrentMode = EClothEditorMode::Draw;
		UE_LOG(LogTemp, Warning, TEXT("Switched to Draw mode"));
		return FReply::Handled();
	}
	else if (Key == EKeys::Two)
	{
		CurrentMode = EClothEditorMode::Select;
		UE_LOG(LogTemp, Warning, TEXT("Switched to Select/Move mode"));

		return FReply::Handled();
	}
	// else if (InKeyEvent.GetKey() == EKeys::Three)
	// {
	// 	CurrentMode = EClothEditorMode::Move;
	// 	UE_LOG(LogTemp, Warning, TEXT("Switched to Move mode"));
	//
	// 	return FReply::Handled();
	// }
	else if (Key == EKeys::Four)
	{
		CurrentMode = EClothEditorMode::Sew;
		UE_LOG(LogTemp, Warning, TEXT("Switched to Sew mode"));

		return FReply::Handled();
	}
	
	// Check for Undo (Ctrl+Z)
	if (Key == EKeys::Z && InKeyEvent.IsControlDown())
	{
		Undo();
		return FReply::Handled();
	}

	// Check for Redo (Ctrl+Y)
	if (Key == EKeys::Y && InKeyEvent.IsControlDown())
	{
		Redo();
		return FReply::Handled();
	}

	if (Key == EKeys::F)
	{
		FocusViewportOnPoints();
		return FReply::Handled();
	}

	

	if (CurrentMode == EClothEditorMode::Select)
	{
		if (Key == EKeys::T)
		{
			bSeparateTangents = true;
			return FReply::Handled();
		}
	}
	
	if (CurrentMode == EClothEditorMode::Draw)
	{
		if (Key == EKeys::B)
		{
			bUseBezierPoints = true;
			return FReply::Handled();
		}
		if (Key == EKeys::N)
		{
			bUseBezierPoints = false;
			return FReply::Handled();
		}
	}
	
	
	if (CurrentMode == EClothEditorMode::Draw)
	{
		if (Key == EKeys::Enter)
		{
			if (CurvePoints.Points.Num() > 0)
			{
				SaveStateForUndo();

				// add bezier tangents to the start and end points
				if (CurvePoints.Points.Num() >= 2)
				{
					int32 LastIdx = CurvePoints.Points.Num() - 1;

					FVector2D Delta0 = CurvePoints.Points[1].OutVal - CurvePoints.Points[0].OutVal;
					CurvePoints.Points[0].ArriveTangent = FVector2D::ZeroVector;
					CurvePoints.Points[0].LeaveTangent  = Delta0 * 0.5f;

					FVector2D Delta1 = CurvePoints.Points[LastIdx].OutVal - CurvePoints.Points[LastIdx - 1].OutVal;
					CurvePoints.Points[LastIdx].ArriveTangent = Delta1 * 0.5f;
					CurvePoints.Points[LastIdx].LeaveTangent  = FVector2D::ZeroVector;
				}
				
				CompletedShapes.Add(CurvePoints);
				CompletedBezierFlags.Add(bUseBezierPerPoint);
				
				CurvePoints.Points.Empty();
				bUseBezierPerPoint.Empty();
				UE_LOG(LogTemp, Warning, TEXT("Shape finalized. Ready to start a new one."));
			}
			return FReply::Handled();
		}
	}

	
	//return FReply::Unhandled();
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);

}

FReply SClothDesignCanvas::OnModeButtonClicked(EClothEditorMode NewMode)
{
	CurrentMode = NewMode;
	Invalidate(EInvalidateWidget::Paint);
	return FReply::Handled();
}

// bool SClothDesignCanvas::IsPointNearLine(const FVector2D& P, const FVector2D& A, const FVector2D& B, float Threshold) const
// {
// 	const FVector2D AP = P - A;
// 	const FVector2D AB = B - A;
//
// 	const float ABLengthSq = AB.SizeSquared();
// 	if (ABLengthSq == 0.f) return false;
//
// 	const float T = FMath::Clamp(FVector2D::DotProduct(AP, AB) / ABLengthSq, 0.f, 1.f);
// 	const FVector2D ClosestPoint = A + T * AB;
// 	return FVector2D::Distance(P, ClosestPoint) <= Threshold;
// }


FReply SClothDesignCanvas::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	
	if (CurrentMode == EClothEditorMode::Select)
	{
		if (Key == EKeys::T)
		{
			bSeparateTangents = false;
			return FReply::Handled();
		}
	}
	//return FReply::Unhandled();
	return SCompoundWidget::OnKeyUp(MyGeometry, InKeyEvent);
}


FReply SClothDesignCanvas::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("Canvas received keyboard focus"));
	return FReply::Handled();
}


void SClothDesignCanvas::FocusViewportOnPoints()
{
	TArray<FVector2D> AllPoints;

	// Collect points from in-progress shape
	for (const auto& Pt : CurvePoints.Points)
	{
		AllPoints.Add(Pt.OutVal);
	}

	// Collect points from completed shapes
	for (const auto& Shape : CompletedShapes)
	{
		for (const auto& Pt : Shape.Points)
		{
			AllPoints.Add(Pt.OutVal);
		}
	}

	if (AllPoints.Num() == 0)
	{
		return; // nothing to focus on
	}

	// Compute bounding box
	FVector2D Min = AllPoints[0];
	FVector2D Max = AllPoints[0];

	for (const FVector2D& Pt : AllPoints)
	{
		Min.X = FMath::Min(Min.X, Pt.X);
		Min.Y = FMath::Min(Min.Y, Pt.Y);
		Max.X = FMath::Max(Max.X, Pt.X);
		Max.Y = FMath::Max(Max.Y, Pt.Y);
	}

	FVector2D Center = (Min + Max) * 0.5f;
	FVector2D BoundsSize = Max - Min;

	// Optional: adjust zoom to fit bounds
	const FVector2D ViewportSize = LastGeometry.GetLocalSize();
	const float Margin = 1.2f; // add some margin around the bounds

	float ZoomX = ViewportSize.X / (BoundsSize.X * Margin);
	float ZoomY = ViewportSize.Y / (BoundsSize.Y * Margin);
	ZoomFactor = FMath::Clamp(FMath::Min(ZoomX, ZoomY), 0.1f, 10.0f);

	//float NewZoom = FMath::Min(ZoomX, ZoomY);

	// Clamp zoom if needed
	//ZoomFactor = FMath::Clamp(NewZoom, 0.05f, 10.0f);

	// Set the offset so the center is at screen center
	//PanOffset = Center - (ViewportSize * 0.5f) / ZoomFactor;
	PanOffset = ViewportSize * 0.5f - Center * ZoomFactor;

	
}

// second but shorter!!
// void SClothDesignCanvas::TriangulateAndBuildMesh()
// {
// 	if (CurvePoints.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}


//void SClothDesignCanvas::TriangulateAndBuildMesh(const FInterpCurve<FVector2D>& Shape)

//
// void SClothDesignCanvas::TriangulateAndBuildMesh(
// 	const FInterpCurve<FVector2D>& Shape,
// 	bool bRecordSeam,
// 	int32 StartPointIdx2D,
// 	int32 EndPointIdx2D
// )
// {
// 	if (Shape.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
//
// 	TArray<FVector2f> PolyVerts;
// 	const int SamplesPerSegment = 10;
// 	// Step 3: Build DynamicMesh
// 	UE::Geometry::FDynamicMesh3 Mesh;
// 	// WORKING but not smooth at all so lots of different sized triangles
// 	// Insert vertices
// 	// TArray<int> VertexIDs;
// 	//
// 	// LastSeamVertexIDs.Empty();
// 	// int TotalSamples = PolyVerts.Num();
//
//
//
//
//
// 	// for (int SegIndex = 0; SegIndex < Shape.Points.Num() - 1; ++SegIndex)
// 	// {
// 	// 	float StartInVal = Shape.Points[SegIndex].InVal;
// 	// 	float EndInVal   = Shape.Points[SegIndex + 1].InVal;
// 	//
// 	// 	for (int i = 0; i < SamplesPerSegment; ++i)
// 	// 	{
// 	// 		float Alpha = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
// 	// 		FVector2D Sample = Shape.Eval(Alpha);
// 	// 		PolyVerts.Add(FVector2f(Sample.X, Sample.Y));
// 	// 	}
// 	// }
//
//
// 	
// 	// Before your loop, compute the integer sample‐range once:
// 	int TotalSamples = (Shape.Points.Num() - 1) * SamplesPerSegment;
// 	int SampleCounter = 0;
//
// 	// Default to an empty range
// 	int MinSample = TotalSamples + 1;
// 	int MaxSample = -1;
//
// 	// Only compute if we really want to record a seam
// 	if (bRecordSeam && StartPointIdx2D >= 0 && EndPointIdx2D >= 0)
// 	{
// 		int S0 = StartPointIdx2D * SamplesPerSegment;
// 		int S1 = EndPointIdx2D   * SamplesPerSegment;
// 		MinSample = FMath::Min(S0, S1);
// 		MaxSample = FMath::Max(S0, S1);
// 	}
//
// 	LastSeamVertexIDs.Empty();
// 	TArray<int32> VertexIDs;
//
// 	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
// 	{
// 		float In0 = Shape.Points[Seg].InVal;
// 		float In1 = Shape.Points[Seg + 1].InVal;
//
// 		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
// 		{
// 			float Alpha = float(i) / SamplesPerSegment;
// 			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
// 			PolyVerts.Add(FVector2f(P2.X, P2.Y));
//
// 			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
// 			VertexIDs.Add(VID);
//
// 			// record seam if this sample falls in the integer [MinSample,MaxSample] range
// 			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
// 			{
// 				LastSeamVertexIDs.Add(VID);
// 			}
// 		}
// 	}
// 	// LastSeamVertexIDs.Empty();
// 	// TArray<int32> VertexIDs;
// 	//
// 	// int TotalSamples = (Shape.Points.Num()-1) * SamplesPerSegment;
// 	// int SampleCounter = 0;
// 	//
// 	// for (int Seg=0; Seg<Shape.Points.Num()-1; ++Seg)
// 	// {
// 	// 	float In0 = Shape.Points[Seg].InVal;
// 	// 	float In1 = Shape.Points[Seg+1].InVal;
// 	//
// 	// 	for (int i=0; i<SamplesPerSegment; ++i, ++SampleCounter)
// 	// 	{
// 	// 		float tSeg = float(i) / SamplesPerSegment;
// 	// 		FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, tSeg));
// 	// 		PolyVerts.Add(FVector2f(P2.X, P2.Y));
// 	//
// 	// 		// store in mesh
// 	// 		int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
// 	// 		VertexIDs.Add(VID);
// 	//
// 	// 		// record seam if in range
// 	// 		if (bRecordSeam)
// 	// 		{
// 	// 			float tCurve = float(SampleCounter) / float(TotalSamples-1);
// 	// 			float tMin = Shape.Points[StartPointIdx2D].InVal / Shape.Points.Last().InVal;
// 	// 			float tMax = Shape.Points[EndPointIdx2D].InVal   / Shape.Points.Last().InVal;
// 	// 			if (tCurve >= tMin && tCurve <= tMax)
// 	// 			{
// 	// 				LastSeamVertexIDs.Add(VID);
// 	// 			}
// 	// 		}
// 	// 	}
// 	// }
// 	
// 	
// 	// Step 2: Triangulate
// 	TArray<UE::Geometry::FIndex3i> Triangles;
// 	PolygonTriangulation::TriangulateSimplePolygon<float>(PolyVerts, Triangles, false);
//
//
//
// 	if (Triangles.Num() == 0)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
// 		return;
// 	}
//
//
// 	for (const FVector2f& V : PolyVerts)
// 	{
// 		int VID = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0)); // z = 0
// 		VertexIDs.Add(VID);
// 	}
// 	// Assume PolyVerts is TArray<FVector2f> of your samples in order
// 	float SignedArea = 0.f;
// 	int N = PolyVerts.Num();
// 	for (int i = 0; i < N; ++i)
// 	{
// 		const FVector2f& A = PolyVerts[i];
// 		const FVector2f& B = PolyVerts[(i+1) % N];
// 		SignedArea += (A.X * B.Y - B.X * A.Y);
// 	}
// 	SignedArea *= 0.5f;
// 	
// 	// for (int idx = 0; idx < TotalSamples; ++idx)
// 	// {
// 	// 	FVector2f V2 = PolyVerts[idx];
// 	// 	int VID = Mesh.AppendVertex(FVector3d(V2.X, V2.Y, 0));
// 	// 	VertexIDs.Add(VID);
// 	//
// 	// 	if (bRecordSeam)
// 	// 	{
// 	// 		// Decide whether idx falls into your seam index range
// 	// 		// (you can map StartPointIdx2D→EndPointIdx2D into sample indices)
// 	// 		if (idx >= SeamSampleStart && idx <= SeamSampleEnd)
// 	// 		{
// 	// 			LastSeamVertexIDs.Add(VID);
// 	// 		}
// 	// 	}
// 	// }
//
//
//
//
//
// 	
// 	// Insert triangles
// 	for (const UE::Geometry::FIndex3i& Tri : Triangles)
// 	{
// 		Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
// 	}
// 	
// 	
// 	// Step 4: Extract to UE arrays
// 	TArray<FVector> Vertices;
// 	TArray<int32> Indices;
//
// 	for (int vid : Mesh.VertexIndicesItr())
// 	{
// 		FVector3d Pos = Mesh.GetVertex(vid);
// 		Vertices.Add(FVector(Pos.X, Pos.Y, Pos.Z));
// 	}
// 	
// 	// for (int tid : Mesh.TriangleIndicesItr())
// 	// {
// 	// 	UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 	// 	Indices.Add(Tri.C);
// 	// 	Indices.Add(Tri.B);
// 	// 	Indices.Add(Tri.A);
// 	// }
//
// 	// fix this later to always wind positive before triangluation but this is a temporary fix for now
// 	bool bReverseWinding = (SignedArea < 0.f);
// 	for (int tid : Mesh.TriangleIndicesItr())
// 	{
// 		UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 		if (bReverseWinding)
// 		{
// 			// flip each triangle
// 			Indices.Add(Tri.A);
// 			Indices.Add(Tri.B);
// 			Indices.Add(Tri.C);
// 		}
// 		else
// 		{
// 			// keep normal winding, here c to a is noremal winding
// 			Indices.Add(Tri.C);
// 			Indices.Add(Tri.B);
// 			Indices.Add(Tri.A);
// 		}
// 	}
// 	
// 	LastBuiltMesh           = MoveTemp(Mesh);
// 	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);
//
// 	// Step 5: Build procedural mesh
// 	CreateProceduralMesh(Vertices, Indices);
// }



// 1) Helper: even–odd rule point-in-polygon test
bool SClothDesignCanvas::IsPointInPolygon(
	const FVector2f& Test, 
	const TArray<FVector2f>& Poly
) {
	bool bInside = false;
	int N = Poly.Num();
	for (int i = 0, j = N - 1; i < N; j = i++) {
		const FVector2f& A = Poly[i];
		const FVector2f& B = Poly[j];
		bool bYCross = ((A.Y > Test.Y) != (B.Y > Test.Y));
		if (bYCross) {
			float XatY = (B.X - A.X) * (Test.Y - A.Y) / (B.Y - A.Y) + A.X;
			if (Test.X < XatY) {
				bInside = !bInside;
			}
		}
	}
	return bInside;
}


// second version but with steiner points, trying delaunay
void SClothDesignCanvas::TriangulateAndBuildMesh(
	const FInterpCurve<FVector2D>& Shape,
	bool bRecordSeam,
	int32 StartPointIdx2D,
	int32 EndPointIdx2D
)
{
	if (Shape.Points.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
		return;
	}

	TArray<FVector2f> PolyVerts;
	const int SamplesPerSegment = 10;
	// Step 3: Build DynamicMesh
	FDynamicMesh3 Mesh;

	
	// Before your loop, compute the integer sample‐range once:
	int TotalSamples = (Shape.Points.Num() - 1) * SamplesPerSegment;
	int SampleCounter = 0;

	// Default to an empty range
	int MinSample = TotalSamples + 1;
	int MaxSample = -1;

	// Only compute if we really want to record a seam
	if (bRecordSeam && StartPointIdx2D >= 0 && EndPointIdx2D >= 0)
	{
		int S0 = StartPointIdx2D * SamplesPerSegment;
		int S1 = EndPointIdx2D   * SamplesPerSegment;
		MinSample = FMath::Min(S0, S1);
		MaxSample = FMath::Max(S0, S1);
	}

	LastSeamVertexIDs.Empty();
	TArray<int32> VertexIDs;

	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
	{
		float In0 = Shape.Points[Seg].InVal;
		float In1 = Shape.Points[Seg + 1].InVal;

		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
		{
			float Alpha = float(i) / SamplesPerSegment;
			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
			PolyVerts.Add(FVector2f(P2.X, P2.Y));

			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
			VertexIDs.Add(VID);

			// record seam if this sample falls in the integer [MinSample,MaxSample] range
			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
			{
				LastSeamVertexIDs.Add(VID);
			}
		}
	}

	// RIGHT HERE: remember how many boundary points you have
	int32 OriginalBoundaryCount = PolyVerts.Num();
	
	// Copy out just the boundary verts for your in‐polygon test:
	TArray<FVector2f> BoundaryOnly;
	BoundaryOnly.Append( PolyVerts.GetData(), OriginalBoundaryCount );

	// --- compute 2D bounding‐box of your sampled polyline
	float MinX = FLT_MAX, MinY = FLT_MAX, MaxX = -FLT_MAX, MaxY = -FLT_MAX;
	for (int32 i = 0; i < OriginalBoundaryCount; ++i)
	{
		const FVector2f& V = PolyVerts[i];
		MinX = FMath::Min(MinX, V.X);
		MinY = FMath::Min(MinY, V.Y);
		MaxX = FMath::Max(MaxX, V.X);
		MaxY = FMath::Max(MaxY, V.Y);
	}

	// --- grid parameters
	const int32 GridRes = 20;    // 10×10 grid → up to 100 interior seeds
	int32 Added = 0;

	// --- sample on a regular grid, keep only centers inside the original polygon
	for (int32 iy = 0; iy < GridRes; ++iy)
	{
		float fy = (iy + 0.5f) / float(GridRes);
		float Y  = FMath::Lerp(MinY, MaxY, fy);

		for (int32 ix = 0; ix < GridRes; ++ix)
		{
			float fx = (ix + 0.5f) / float(GridRes);
			float X  = FMath::Lerp(MinX, MaxX, fx);

			FVector2f Cand(X, Y);
			if ( IsPointInPolygon(Cand, BoundaryOnly) )
			{
				// add to the full list
				PolyVerts.Add(Cand);

				// let your Delaunay/CDT see it:
				int32 VID = Mesh.AppendVertex(FVector3d(Cand.X, Cand.Y, 0));
				VertexIDs.Add(VID);

				++Added;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Placed %d grid‐based interior samples"), Added);

	// Build the list of constrained edges on the original boundary:
	TArray<UE::Geometry::FIndex2i> BoundaryEdges;
	BoundaryEdges.Reserve(OriginalBoundaryCount);
	for (int32 i = 0; i < OriginalBoundaryCount; ++i)
	{
		BoundaryEdges.Add(
			UE::Geometry::FIndex2i(i, (i + 1) % OriginalBoundaryCount)
		);
	}
	

	// --- 2) Set up and run the Constrained Delaunay ---
	UE::Geometry::TConstrainedDelaunay2<float> CDT;
	
	
	CDT.Vertices      = PolyVerts;          // TArray<TVector2<float>>
	CDT.Edges         = BoundaryEdges;      // TArray<FIndex2i>
	CDT.bOrientedEdges = true;              // enforce input edge orientation
	CDT.FillRule = UE::Geometry::TConstrainedDelaunay2<float>::EFillRule::Odd;  

	// CDT.FillRule      = EFillRule::EvenOdd;  
	CDT.bOutputCCW    = true;               // get CCW‐wound triangles

	// If you want to cut out hole‐loops, you can fill CDT.HoleEdges similarly.

	// Run the triangulation:
	bool bOK = CDT.Triangulate();
	if (!bOK || CDT.Triangles.Num() == 0)
	{
	    UE_LOG(LogTemp, Error, TEXT("CDT failed to triangulate shape"));
	    return;
	}

	// --- 3) Move it into an FDynamicMesh3 ---
	UE::Geometry::FDynamicMesh3 MeshOut;
	MeshOut.EnableTriangleGroups();
	// MeshOut.SetAllowBowties(true);  // if you expect split‐bowties

	// Append all vertices:
	for (const UE::Geometry::TVector2<float>& V2 : CDT.Vertices)
	{
	    MeshOut.AppendVertex(FVector3d(V2.X, V2.Y, 0));
	}

	// Append all triangles:
	for (const UE::Geometry::FIndex3i& Tri : CDT.Triangles)
	{
	    // Tri is CCW if bOutputCCW==true
	    MeshOut.AppendTriangle(Tri.A, Tri.B, Tri.C);
	}

	// --- 4) Extract to your ProceduralMeshComponent as before ---
	TArray<FVector> Vertices;
	TArray<int32>   Indices;
	Vertices.Reserve(MeshOut.VertexCount());
	for (int vid : MeshOut.VertexIndicesItr())
	{
	    FVector3d P = MeshOut.GetVertex(vid);
	    Vertices.Add(FVector(P.X, P.Y, P.Z));
	}
	for (int tid : MeshOut.TriangleIndicesItr())
	{
	    auto T = MeshOut.GetTriangle(tid);
	    // already CCW, so push A→B→C
	    Indices.Add(T.C);
	    Indices.Add(T.B);
	    Indices.Add(T.A);
	}

	CreateProceduralMesh(Vertices, Indices);
	
}


// // second version but with steiner points, working random delaunay
// void SClothDesignCanvas::TriangulateAndBuildMesh(
// 	const FInterpCurve<FVector2D>& Shape,
// 	bool bRecordSeam,
// 	int32 StartPointIdx2D,
// 	int32 EndPointIdx2D
// )
// {
// 	if (Shape.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
//
// 	TArray<FVector2f> PolyVerts;
// 	const int SamplesPerSegment = 10;
// 	// Step 3: Build DynamicMesh
// 	FDynamicMesh3 Mesh;
//
// 	
// 	// Before your loop, compute the integer sample‐range once:
// 	int TotalSamples = (Shape.Points.Num() - 1) * SamplesPerSegment;
// 	int SampleCounter = 0;
//
// 	// Default to an empty range
// 	int MinSample = TotalSamples + 1;
// 	int MaxSample = -1;
//
// 	// Only compute if we really want to record a seam
// 	if (bRecordSeam && StartPointIdx2D >= 0 && EndPointIdx2D >= 0)
// 	{
// 		int S0 = StartPointIdx2D * SamplesPerSegment;
// 		int S1 = EndPointIdx2D   * SamplesPerSegment;
// 		MinSample = FMath::Min(S0, S1);
// 		MaxSample = FMath::Max(S0, S1);
// 	}
//
// 	LastSeamVertexIDs.Empty();
// 	TArray<int32> VertexIDs;
//
// 	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
// 	{
// 		float In0 = Shape.Points[Seg].InVal;
// 		float In1 = Shape.Points[Seg + 1].InVal;
//
// 		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
// 		{
// 			float Alpha = float(i) / SamplesPerSegment;
// 			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
// 			PolyVerts.Add(FVector2f(P2.X, P2.Y));
//
// 			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
// 			VertexIDs.Add(VID);
//
// 			// record seam if this sample falls in the integer [MinSample,MaxSample] range
// 			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
// 			{
// 				LastSeamVertexIDs.Add(VID);
// 			}
// 		}
// 	}
//
// 	// RIGHT HERE: remember how many boundary points you have
// 	int32 OriginalBoundaryCount = PolyVerts.Num();
//
// 	// Build the list of constrained edges on the original boundary:
// 	TArray<UE::Geometry::FIndex2i> BoundaryEdges;
// 	BoundaryEdges.Reserve(OriginalBoundaryCount);
// 	for (int32 i = 0; i < OriginalBoundaryCount; ++i)
// 	{
// 		BoundaryEdges.Add(
// 			UE::Geometry::FIndex2i(i, (i + 1) % OriginalBoundaryCount)
// 		);
// 	}
// 	
// 	
// 	// --- compute 2D bounding‐box of your sampled polyline
// 	float MinX = FLT_MAX, MinY = FLT_MAX, MaxX = -FLT_MAX, MaxY = -FLT_MAX;
// 	for (const FVector2f& V : PolyVerts)
// 	{
// 		MinX = FMath::Min(MinX, V.X);
// 		MinY = FMath::Min(MinY, V.Y);
// 		MaxX = FMath::Max(MaxX, V.X);
// 		MaxY = FMath::Max(MaxY, V.Y);
// 	}
//
// 	// --- random stream (optional: seed for reproducible meshes)
// 	FRandomStream RandStream(FPlatformTime::Cycles());
//
// 	const int32 NumInteriorSamples = 200;
// 	int32 Added = 0, Attempts = 0;
// 	while (Added < NumInteriorSamples && Attempts < NumInteriorSamples * 5)
// 	{
// 		++Attempts;
// 		// uniform in [Min,Max]
// 		float RX = RandStream.FRandRange(MinX, MaxX);
// 		float RY = RandStream.FRandRange(MinY, MaxY);
// 		FVector2f Cand(RX, RY);
//
// 		if (IsPointInPolygon(Cand, PolyVerts))
// 		{
// 			// keep it
// 			PolyVerts.Add(Cand);
//
// 			// also append to DynamicMesh so the triangulator can see it
// 			int32 VID = Mesh.AppendVertex(FVector3d(Cand.X, Cand.Y, 0));
// 			VertexIDs.Add(VID);
//
// 			++Added;
// 		}
// 	
// 	UE_LOG(LogTemp, Log, TEXT("Placed %d interior samples after %d tries"), Added, Attempts);
// 	}
//
//
//
// 	// --- 2) Set up and run the Constrained Delaunay ---
// 	UE::Geometry::TConstrainedDelaunay2<float> CDT;
// 	
// 	
// 	CDT.Vertices      = PolyVerts;          // TArray<TVector2<float>>
// 	CDT.Edges         = BoundaryEdges;      // TArray<FIndex2i>
// 	CDT.bOrientedEdges = true;              // enforce input edge orientation
// 	CDT.FillRule = UE::Geometry::TConstrainedDelaunay2<float>::EFillRule::Odd;  
//
// 	// CDT.FillRule      = EFillRule::EvenOdd;  
// 	CDT.bOutputCCW    = true;               // get CCW‐wound triangles
//
// 	// If you want to cut out hole‐loops, you can fill CDT.HoleEdges similarly.
//
// 	// Run the triangulation:
// 	bool bOK = CDT.Triangulate();
// 	if (!bOK || CDT.Triangles.Num() == 0)
// 	{
// 	    UE_LOG(LogTemp, Error, TEXT("CDT failed to triangulate shape"));
// 	    return;
// 	}
//
// 	// --- 3) Move it into an FDynamicMesh3 ---
// 	UE::Geometry::FDynamicMesh3 MeshOut;
// 	MeshOut.EnableTriangleGroups();
// 	// MeshOut.SetAllowBowties(true);  // if you expect split‐bowties
//
// 	// Append all vertices:
// 	for (const UE::Geometry::TVector2<float>& V2 : CDT.Vertices)
// 	{
// 	    MeshOut.AppendVertex(FVector3d(V2.X, V2.Y, 0));
// 	}
//
// 	// Append all triangles:
// 	for (const UE::Geometry::FIndex3i& Tri : CDT.Triangles)
// 	{
// 	    // Tri is CCW if bOutputCCW==true
// 	    MeshOut.AppendTriangle(Tri.A, Tri.B, Tri.C);
// 	}
//
// 	// --- 4) Extract to your ProceduralMeshComponent as before ---
// 	TArray<FVector> Vertices;
// 	TArray<int32>   Indices;
// 	Vertices.Reserve(MeshOut.VertexCount());
// 	for (int vid : MeshOut.VertexIndicesItr())
// 	{
// 	    FVector3d P = MeshOut.GetVertex(vid);
// 	    Vertices.Add(FVector(P.X, P.Y, P.Z));
// 	}
// 	for (int tid : MeshOut.TriangleIndicesItr())
// 	{
// 	    auto T = MeshOut.GetTriangle(tid);
// 	    // already CCW, so push A→B→C
// 	    Indices.Add(T.C);
// 	    Indices.Add(T.B);
// 	    Indices.Add(T.A);
// 	}
//
// 	CreateProceduralMesh(Vertices, Indices);
// 	
// }



// // second version but shorter and with steiner points
// void SClothDesignCanvas::TriangulateAndBuildMesh(
// 	const FInterpCurve<FVector2D>& Shape,
// 	bool bRecordSeam,
// 	int32 StartPointIdx2D,
// 	int32 EndPointIdx2D
// )
// {
// 	if (Shape.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
//
// 	TArray<FVector2f> PolyVerts;
// 	const int SamplesPerSegment = 10;
// 	// Step 3: Build DynamicMesh
// 	UE::Geometry::FDynamicMesh3 Mesh;
//
// 	
// 	// Before your loop, compute the integer sample‐range once:
// 	int TotalSamples = (Shape.Points.Num() - 1) * SamplesPerSegment;
// 	int SampleCounter = 0;
//
// 	// Default to an empty range
// 	int MinSample = TotalSamples + 1;
// 	int MaxSample = -1;
//
// 	// Only compute if we really want to record a seam
// 	if (bRecordSeam && StartPointIdx2D >= 0 && EndPointIdx2D >= 0)
// 	{
// 		int S0 = StartPointIdx2D * SamplesPerSegment;
// 		int S1 = EndPointIdx2D   * SamplesPerSegment;
// 		MinSample = FMath::Min(S0, S1);
// 		MaxSample = FMath::Max(S0, S1);
// 	}
//
// 	LastSeamVertexIDs.Empty();
// 	TArray<int32> VertexIDs;
//
// 	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
// 	{
// 		float In0 = Shape.Points[Seg].InVal;
// 		float In1 = Shape.Points[Seg + 1].InVal;
//
// 		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
// 		{
// 			float Alpha = float(i) / SamplesPerSegment;
// 			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
// 			PolyVerts.Add(FVector2f(P2.X, P2.Y));
//
// 			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
// 			VertexIDs.Add(VID);
//
// 			// record seam if this sample falls in the integer [MinSample,MaxSample] range
// 			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
// 			{
// 				LastSeamVertexIDs.Add(VID);
// 			}
// 		}
// 	}
//
// 	
// 	// --- compute 2D bounding‐box of your sampled polyline
// 	float MinX = FLT_MAX, MinY = FLT_MAX, MaxX = -FLT_MAX, MaxY = -FLT_MAX;
// 	for (const FVector2f& V : PolyVerts)
// 	{
// 		MinX = FMath::Min(MinX, V.X);
// 		MinY = FMath::Min(MinY, V.Y);
// 		MaxX = FMath::Max(MaxX, V.X);
// 		MaxY = FMath::Max(MaxY, V.Y);
// 	}
//
// 	// --- random stream (optional: seed for reproducible meshes)
// 	FRandomStream RandStream(FPlatformTime::Cycles());
//
// 	const int32 NumInteriorSamples = 20;
// 	int32 Added = 0, Attempts = 0;
// 	while (Added < NumInteriorSamples && Attempts < NumInteriorSamples * 5)
// 	{
// 		++Attempts;
// 		// uniform in [Min,Max]
// 		float RX = RandStream.FRandRange(MinX, MaxX);
// 		float RY = RandStream.FRandRange(MinY, MaxY);
// 		FVector2f Cand(RX, RY);
//
// 		if (IsPointInPolygon(Cand, PolyVerts))
// 		{
// 			// keep it
// 			PolyVerts.Add(Cand);
//
// 			// also append to DynamicMesh so the triangulator can see it
// 			int32 VID = Mesh.AppendVertex(FVector3d(Cand.X, Cand.Y, 0));
// 			VertexIDs.Add(VID);
//
// 			++Added;
// 		}
// 	
// 	UE_LOG(LogTemp, Log, TEXT("Placed %d interior samples after %d tries"), Added, Attempts);
// 	}
// 	
// 	
// 	// Step 2: Triangulate
// 	TArray<UE::Geometry::FIndex3i> Triangles;
// 	PolygonTriangulation::TriangulateSimplePolygon<float>(PolyVerts, Triangles, false);
//
//
//
// 	if (Triangles.Num() == 0)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
// 		return;
// 	}
//
//
// 	for (const FVector2f& V : PolyVerts)
// 	{
// 		int VID = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0)); // z = 0
// 		VertexIDs.Add(VID);
// 	}
// 	// Assume PolyVerts is TArray<FVector2f> of your samples in order
// 	float SignedArea = 0.f;
// 	int N = PolyVerts.Num();
// 	for (int i = 0; i < N; ++i)
// 	{
// 		const FVector2f& A = PolyVerts[i];
// 		const FVector2f& B = PolyVerts[(i+1) % N];
// 		SignedArea += (A.X * B.Y - B.X * A.Y);
// 	}
// 	SignedArea *= 0.5f;
//
// 	
// 	// Insert triangles
// 	for (const UE::Geometry::FIndex3i& Tri : Triangles)
// 	{
// 		Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
// 	}
// 	
// 	
// 	// Step 4: Extract to UE arrays
// 	TArray<FVector> Vertices;
// 	TArray<int32> Indices;
//
// 	for (int vid : Mesh.VertexIndicesItr())
// 	{
// 		FVector3d Pos = Mesh.GetVertex(vid);
// 		Vertices.Add(FVector(Pos.X, Pos.Y, Pos.Z));
// 	}
// 	
// 	// for (int tid : Mesh.TriangleIndicesItr())
// 	// {
// 	// 	UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 	// 	Indices.Add(Tri.C);
// 	// 	Indices.Add(Tri.B);
// 	// 	Indices.Add(Tri.A);
// 	// }
//
// 	// fix this later to always wind positive before triangluation but this is a temporary fix for now
// 	bool bReverseWinding = (SignedArea < 0.f);
// 	for (int tid : Mesh.TriangleIndicesItr())
// 	{
// 		UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 		if (bReverseWinding)
// 		{
// 			// flip each triangle
// 			Indices.Add(Tri.A);
// 			Indices.Add(Tri.B);
// 			Indices.Add(Tri.C);
// 		}
// 		else
// 		{
// 			// keep normal winding, here c to a is noremal winding
// 			Indices.Add(Tri.C);
// 			Indices.Add(Tri.B);
// 			Indices.Add(Tri.A);
// 		}
// 	}
// 	
// 	LastBuiltMesh           = MoveTemp(Mesh);
// 	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);
//
// 	// Step 5: Build procedural mesh
// 	CreateProceduralMesh(Vertices, Indices);
// }


// // second version but shorter!!
// void SClothDesignCanvas::TriangulateAndBuildMesh(
// 	const FInterpCurve<FVector2D>& Shape,
// 	bool bRecordSeam,
// 	int32 StartPointIdx2D,
// 	int32 EndPointIdx2D
// )
// {
// 	if (Shape.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
//
// 	TArray<FVector2f> PolyVerts;
// 	const int SamplesPerSegment = 10;
// 	// Step 3: Build DynamicMesh
// 	UE::Geometry::FDynamicMesh3 Mesh;
//
// 	
// 	// Before your loop, compute the integer sample‐range once:
// 	int TotalSamples = (Shape.Points.Num() - 1) * SamplesPerSegment;
// 	int SampleCounter = 0;
//
// 	// Default to an empty range
// 	int MinSample = TotalSamples + 1;
// 	int MaxSample = -1;
//
// 	// Only compute if we really want to record a seam
// 	if (bRecordSeam && StartPointIdx2D >= 0 && EndPointIdx2D >= 0)
// 	{
// 		int S0 = StartPointIdx2D * SamplesPerSegment;
// 		int S1 = EndPointIdx2D   * SamplesPerSegment;
// 		MinSample = FMath::Min(S0, S1);
// 		MaxSample = FMath::Max(S0, S1);
// 	}
//
// 	LastSeamVertexIDs.Empty();
// 	TArray<int32> VertexIDs;
//
// 	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
// 	{
// 		float In0 = Shape.Points[Seg].InVal;
// 		float In1 = Shape.Points[Seg + 1].InVal;
//
// 		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
// 		{
// 			float Alpha = float(i) / SamplesPerSegment;
// 			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
// 			PolyVerts.Add(FVector2f(P2.X, P2.Y));
//
// 			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
// 			VertexIDs.Add(VID);
//
// 			// record seam if this sample falls in the integer [MinSample,MaxSample] range
// 			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
// 			{
// 				LastSeamVertexIDs.Add(VID);
// 			}
// 		}
// 	}
//
// 	
// 	
// 	// Step 2: Triangulate
// 	TArray<UE::Geometry::FIndex3i> Triangles;
// 	PolygonTriangulation::TriangulateSimplePolygon<float>(PolyVerts, Triangles, false);
//
//
//
// 	if (Triangles.Num() == 0)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
// 		return;
// 	}
//
//
// 	for (const FVector2f& V : PolyVerts)
// 	{
// 		int VID = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0)); // z = 0
// 		VertexIDs.Add(VID);
// 	}
// 	// Assume PolyVerts is TArray<FVector2f> of your samples in order
// 	float SignedArea = 0.f;
// 	int N = PolyVerts.Num();
// 	for (int i = 0; i < N; ++i)
// 	{
// 		const FVector2f& A = PolyVerts[i];
// 		const FVector2f& B = PolyVerts[(i+1) % N];
// 		SignedArea += (A.X * B.Y - B.X * A.Y);
// 	}
// 	SignedArea *= 0.5f;
//
// 	
// 	// Insert triangles
// 	for (const UE::Geometry::FIndex3i& Tri : Triangles)
// 	{
// 		Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
// 	}
// 	
// 	
// 	// Step 4: Extract to UE arrays
// 	TArray<FVector> Vertices;
// 	TArray<int32> Indices;
//
// 	for (int vid : Mesh.VertexIndicesItr())
// 	{
// 		FVector3d Pos = Mesh.GetVertex(vid);
// 		Vertices.Add(FVector(Pos.X, Pos.Y, Pos.Z));
// 	}
// 	
// 	// for (int tid : Mesh.TriangleIndicesItr())
// 	// {
// 	// 	UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 	// 	Indices.Add(Tri.C);
// 	// 	Indices.Add(Tri.B);
// 	// 	Indices.Add(Tri.A);
// 	// }
//
// 	// fix this later to always wind positive before triangluation but this is a temporary fix for now
// 	bool bReverseWinding = (SignedArea < 0.f);
// 	for (int tid : Mesh.TriangleIndicesItr())
// 	{
// 		UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 		if (bReverseWinding)
// 		{
// 			// flip each triangle
// 			Indices.Add(Tri.A);
// 			Indices.Add(Tri.B);
// 			Indices.Add(Tri.C);
// 		}
// 		else
// 		{
// 			// keep normal winding, here c to a is noremal winding
// 			Indices.Add(Tri.C);
// 			Indices.Add(Tri.B);
// 			Indices.Add(Tri.A);
// 		}
// 	}
// 	
// 	LastBuiltMesh           = MoveTemp(Mesh);
// 	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);
//
// 	// Step 5: Build procedural mesh
// 	CreateProceduralMesh(Vertices, Indices);
// }
//


//
// void SClothDesignCanvas::TriangulateAndBuildMesh(
// 	const FInterpCurve<FVector2D>& Shape,
// 	bool bRecordSeam,
// 	int32 StartPointIdx2D,
// 	int32 EndPointIdx2D
// )
// {
// 	if (Shape.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
//
//
// 	// // !!
// 	// // At the top of your function
// 	// TArray<bool>    bIsBoundary;    // same length as LastBuiltMesh.MaxVertexID()+1
// 	// TArray<double>  BoundaryT;      // stores the curve parameter for each boundary vertex
// 	//
//
//
//
// 	
// 	TArray<FVector2f> PolyVerts;
// 	const int SamplesPerSegment = 10;
// 	// Step 3: Build DynamicMesh
// 	UE::Geometry::FDynamicMesh3 Mesh;
//
// 	
// 	// Before your loop, compute the integer sample‐range once:
// 	int TotalSamples = (Shape.Points.Num() - 1) * SamplesPerSegment;
// 	int SampleCounter = 0;
//
// 	// Default to an empty range
// 	int MinSample = TotalSamples + 1;
// 	int MaxSample = -1;
//
// 	// Only compute if we really want to record a seam
// 	if (bRecordSeam && StartPointIdx2D >= 0 && EndPointIdx2D >= 0)
// 	{
// 		int S0 = StartPointIdx2D * SamplesPerSegment;
// 		int S1 = EndPointIdx2D   * SamplesPerSegment;
// 		MinSample = FMath::Min(S0, S1);
// 		MaxSample = FMath::Max(S0, S1);
// 	}
//
// 	LastSeamVertexIDs.Empty();
// 	TArray<int32> VertexIDs;
//
// 	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
// 	{
// 		float In0 = Shape.Points[Seg].InVal;
// 		float In1 = Shape.Points[Seg + 1].InVal;
//
// 		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
// 		{
// 			float Alpha = float(i) / SamplesPerSegment;
// 			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
// 			PolyVerts.Add(FVector2f(P2.X, P2.Y));
//
// 			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
// 			VertexIDs.Add(VID);
// 			
// 			// record seam if this sample falls in the integer [MinSample,MaxSample] range
// 			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
// 			{
// 				LastSeamVertexIDs.Add(VID);
// 			}
// 		}
// 	}
//
//
// 	
//
// 	TArray<FVector2d> CDT_Vertices;
// 	TArray<UE::Geometry::FIndex2i> CDT_Edges;
//
// 	int NumVerts = PolyVerts.Num();
// 	for (int32 i = 0; i < NumVerts; ++i)
// 	{
// 		CDT_Vertices.Add(FVector2d(PolyVerts[i]));
// 		int32 Next = (i + 1) % NumVerts;
// 		CDT_Edges.Add(UE::Geometry::FIndex2i(i, Next));
// 	}
//
// 	UE::Geometry::FDelaunay2 CDT;
// 	bool bSuccess = CDT.Triangulate(CDT_Vertices, CDT_Edges);
// 	if (!bSuccess)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("CDT triangulation failed"));
// 		return;
// 	}
// 	
// 	// Step 4: Extract to UE arrays
// 	TArray<FVector> Vertices;
// 	TArray<int32> Indices;
// 	TArray<UE::Geometry::FIndex3i> Triangles;
//
// 	
// 	for (const FVector2d& V : CDT_Vertices)
// 	{
// 		int vid = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0));
// 		VertexIDs.Add(vid);
// 	}
//
// 	for (const UE::Geometry::FIndex3i& Tri : CDT.GetTriangles())
// 	{
// 		Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
// 	}
//
//
//
//
//
// 	
// 	//
// 	// // Step 2: Triangulate
// 	// TArray<UE::Geometry::FIndex3i> Triangles;
// 	// PolygonTriangulation::TriangulateSimplePolygon<float>(PolyVerts, Triangles, false);
// 	//
// 	// FDynamicMesh3 Mesh;
// 	// TArray<int32> VertexIDs; //? correct place?
// 	//
// 	// if (Triangles.Num() == 0)
// 	// {
// 	// 	UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
// 	// 	return;
// 	// }
// 	//
// 	//
// 	// // for (const FVector2f& V : PolyVerts)
// 	// // {
// 	// // 	int VID = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0)); // z = 0
// 	// // 	VertexIDs.Add(VID);
// 	// // }
// 	//
// 	//
// 	// // Assume PolyVerts is TArray<FVector2f> of your samples in order
// 	// float SignedArea = 0.f;
// 	// int N = PolyVerts.Num();
// 	// for (int i = 0; i < N; ++i)
// 	// {
// 	// 	const FVector2f& A = PolyVerts[i];
// 	// 	const FVector2f& B = PolyVerts[(i+1) % N];
// 	// 	SignedArea += (A.X * B.Y - B.X * A.Y);
// 	// }
// 	// SignedArea *= 0.5f;
// 	//
// 	//
// 	// // Insert triangles
// 	// for (const UE::Geometry::FIndex3i& Tri : Triangles)
// 	// {
// 	// 	Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
// 	// }
//
//
//
//
// 	
// 	// // 2. Remesher setup
// 	// UE::Geometry::FRemesher Remesher(&Mesh);
// 	// Remesher.SetTargetEdgeLength(1.50); // Set this to desired density
// 	//
// 	// Remesher.SmoothSpeedT = 0.0f; // Optional tweak
// 	//
// 	// for (int i = 0; i < 2; ++i)  // 1 not enough, 3-5 too much in terms of edge vertices changing
// 	// {
// 	// 	Remesher.BasicRemeshPass();
// 	// }
// 	//
// 	// // // (Optional) Subdivide for more detail
// 	// // // UE::Geometry::FDynamicMeshEditor Editor(&Mesh);
// 	//
// 	// int32 NumSubdivisions = 2;
// 	// double MaxEdgeLength = 1.50;
// 	//
// 	// for (int32 Pass = 0; Pass < NumSubdivisions; ++Pass)
// 	// {
// 	// 	TArray<int32> EdgesToSplit;
// 	// 	for (int32 eid : Mesh.EdgeIndicesItr())
// 	// 	{
// 	// 		UE::Geometry::FIndex2i EdgeVerts = Mesh.GetEdgeV(eid);
// 	// 		FVector3d A = Mesh.GetVertex(EdgeVerts.A);
// 	// 		FVector3d B = Mesh.GetVertex(EdgeVerts.B);
// 	//
// 	// 		if (FVector3d::Distance(A, B) > MaxEdgeLength)
// 	// 		{
// 	// 			EdgesToSplit.Add(eid);
// 	// 		}
// 	// 	}
// 	//
// 	// 	for (int32 eid : EdgesToSplit)
// 	// 	{
// 	// 		UE::Geometry::FDynamicMesh3::FEdgeSplitInfo SplitInfo;
// 	// 		Mesh.SplitEdge(eid, SplitInfo);
// 	// 	}
// 	// }
// 	
//
//
// 	
// 	// // Step 4: Extract to UE arrays
// 	// TArray<FVector> Vertices;
// 	// TArray<int32> Indices;
//
// 	for (int vid : Mesh.VertexIndicesItr())
// 	{
// 		FVector3d Pos = Mesh.GetVertex(vid);
// 		Vertices.Add(FVector(Pos.X, Pos.Y, Pos.Z));
// 	}
//
//
//
// 	for (int tid : Mesh.TriangleIndicesItr())
// 	{
// 		UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 		Indices.Add(Tri.C);
// 		Indices.Add(Tri.B);
// 		Indices.Add(Tri.A);
// 	}
//
// 	// // fix this later to always wind positive before triangluation but this is a temporary fix for now
// 	// bool bReverseWinding = (SignedArea < 0.f);
// 	// for (int tid : Mesh.TriangleIndicesItr())
// 	// {
// 	// 	UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 	// 	if (bReverseWinding)
// 	// 	{
// 	// 		// flip each triangle
// 	// 		Indices.Add(Tri.A);
// 	// 		Indices.Add(Tri.B);
// 	// 		Indices.Add(Tri.C);
// 	// 	}
// 	// 	else
// 	// 	{
// 	// 		// keep normal winding, here c to a is noremal winding
// 	// 		Indices.Add(Tri.C);
// 	// 		Indices.Add(Tri.B);
// 	// 		Indices.Add(Tri.A);
// 	// 	}
// 	// }
// 	
// 	LastBuiltMesh           = MoveTemp(Mesh);
// 	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);
//
//
// 	//
// 	// double DesiredEdgeLength = 10.0;  
// 	// double TargetLen = DesiredEdgeLength;  // your max‐edge threshold
// 	// int  MaxPasses  = 3;                   // loop a few times in case splits introduce new long edges
// 	//
// 	//
// 	// // desired max edge length in the same units as your FVector2D X/Y
// 	//
// 	// // do multiple passes in case splitting creates new long edges
// 	// for (int pass = 0; pass < 3; ++pass)
// 	// {
// 	// 	TArray<int> ToSplit;
// 	// 	// collect edge IDs
// 	// 	for (int eid : LastBuiltMesh.EdgeIndicesItr())
// 	// 	{
// 	// 		auto ev = LastBuiltMesh.GetEdgeV(eid);
// 	// 		FVector3d A = LastBuiltMesh.GetVertex(ev.A);
// 	// 		FVector3d B = LastBuiltMesh.GetVertex(ev.B);
// 	// 		if ((A - B).Length() > TargetLen)
// 	// 		{
// 	// 			ToSplit.Add(eid);
// 	// 		}
// 	// 	}
// 	// 	if (ToSplit.Num() == 0)
// 	// 	{
// 	// 		break;
// 	// 	}
// 	// 	// split them
// 	// 	DynamicMeshInfo::FEdgeSplitInfo splitInfo;
// 	// 	for (int eid : ToSplit)
// 	// 	{
// 	// 		auto ev = LastBuiltMesh.GetEdgeV(eid);
// 	// 		LastBuiltMesh.SplitEdge(ev.A, ev.B, splitInfo);
// 	// 	}
// 	// }
// 	// // 3a) Flip any edge where swapping improves the worst angle
// 	// DynamicMeshInfo::FEdgeFlipInfo flipInfo;
// 	// for (int eid : LastBuiltMesh.EdgeIndicesItr())
// 	// {
// 	// 	auto ev = LastBuiltMesh.GetEdgeV(eid);
// 	// 	// you could check an “improvement” metric, but let’s just attempt flips
// 	// 	LastBuiltMesh.FlipEdge(ev.A, ev.B, flipInfo);
// 	// }
// 	//
// 	// // 3b) Quick Laplacian smooth (2 iterations)
// 	// for (int iter = 0; iter < 2; ++iter)
// 	// {
// 	// 	TArray<FVector3d> NewPos; 
// 	// 	NewPos.SetNum(LastBuiltMesh.MaxVertexID()+1);
// 	// 	for (int vid : LastBuiltMesh.VertexIndicesItr())
// 	// 	{
// 	// 		FVector3d Sum(0,0,0);
// 	// 		int Count = 0;
// 	// 		for (int nbr : LastBuiltMesh.VtxVerticesItr(vid))
// 	// 		{
// 	// 			Sum += LastBuiltMesh.GetVertex(nbr);
// 	// 			++Count;
// 	// 		}
// 	// 		if (Count > 0)
// 	// 		{
// 	// 			// blend 50% toward neighborhood centroid
// 	// 			NewPos[vid] = LastBuiltMesh.GetVertex(vid) * 0.5 + (Sum / Count) * 0.5;
// 	// 		}
// 	// 		else
// 	// 		{
// 	// 			NewPos[vid] = LastBuiltMesh.GetVertex(vid);
// 	// 		}
// 	// 	}
// 	// 	for (int vid : LastBuiltMesh.VertexIndicesItr())
// 	// 	{
// 	// 		LastBuiltMesh.SetVertex(vid, NewPos[vid]);
// 	// 	}
// 	// }
// 	//
// 	// Vertices.SetNum(LastBuiltMesh.VertexCount());
// 	// for (int vid : LastBuiltMesh.VertexIndicesItr())
// 	// {
// 	// 	FVector3d P = LastBuiltMesh.GetVertex(vid);
// 	// 	Vertices[vid] = FVector(P.X, P.Y, P.Z);
// 	// }
// 	//
// 	// Indices.SetNum(LastBuiltMesh.TriangleCount() * 3);
// 	// int idx = 0;
// 	// for (int tid : LastBuiltMesh.TriangleIndicesItr())
// 	// {
// 	// 	UE::Geometry::FIndex3i T = LastBuiltMesh.GetTriangle(tid);
// 	// 	Indices[idx++] = T.A;
// 	// 	Indices[idx++] = T.B;
// 	// 	Indices[idx++] = T.C;
// 	// }
// 	//
//
//
//
//
// 	
// 	// Step 5: Build procedural mesh
// 	CreateProceduralMesh(Vertices, Indices);
// }





//
// // third version with interior sampling
// void SClothDesignCanvas::TriangulateAndBuildMesh(
// 	const FInterpCurve<FVector2D>& Shape,
// 	bool bRecordSeam,
// 	int32 StartPointIdx2D,
// 	int32 EndPointIdx2D
// )
// {
// 	if (Shape.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
//
// 	TArray<FVector2f> PolyVerts; // boundarypts
// 	const int SamplesPerSegment = 10;
// 	// Step 3: Build DynamicMesh
// 	UE::Geometry::FDynamicMesh3 Mesh;
//
// 	
// 	// Before your loop, compute the integer sample‐range once:
// 	int TotalSamples = (Shape.Points.Num() - 1) * SamplesPerSegment;
// 	int SampleCounter = 0;
//
// 	// Default to an empty range
// 	int MinSample = TotalSamples + 1;
// 	int MaxSample = -1;
//
// 	// Only compute if we really want to record a seam
// 	if (bRecordSeam && StartPointIdx2D >= 0 && EndPointIdx2D >= 0)
// 	{
// 		int S0 = StartPointIdx2D * SamplesPerSegment;
// 		int S1 = EndPointIdx2D   * SamplesPerSegment;
// 		MinSample = FMath::Min(S0, S1);
// 		MaxSample = FMath::Max(S0, S1);
// 	}
//
// 	LastSeamVertexIDs.Empty();
// 	TArray<int32> VertexIDs;
//
// 	for (int Seg = 0; Seg < Shape.Points.Num() - 1; ++Seg)
// 	{
// 		float In0 = Shape.Points[Seg].InVal;
// 		float In1 = Shape.Points[Seg + 1].InVal;
//
// 		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter)
// 		{
// 			float Alpha = float(i) / SamplesPerSegment;
// 			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, Alpha));
// 			PolyVerts.Add(FVector2f(P2.X, P2.Y));
//
// 			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
// 			VertexIDs.Add(VID);
//
// 			// record seam if this sample falls in the integer [MinSample,MaxSample] range
// 			if (bRecordSeam && SampleCounter >= MinSample && SampleCounter <= MaxSample)
// 			{
// 				LastSeamVertexIDs.Add(VID);
// 			}
// 		}
// 	}
// 	
// 	auto IsInsideFloatPoly = [&](const FVector2f& Pf){
// 		FVector2D Pd(Pf.X, Pf.Y);
// 		// convert your array once:
// 		static TArray<FVector2D> PolyVertsD;
// 		if (PolyVertsD.Num() != PolyVerts.Num()) {
// 			PolyVertsD.Reset(PolyVerts.Num());
// 			for (auto& Vf : PolyVerts)
// 				PolyVertsD.Add(FVector2D(Vf.X, Vf.Y));
// 		}
// 		return FGeomTools2D::IsPointInPolygon(Pd, PolyVertsD);
// 	};
// 	
// 	TArray<FVector2D> PolyVerts2D;
// 	PolyVerts2D.Reserve(PolyVerts.Num());
// 	for (const FVector2f& Vf : PolyVerts)
// 	{
// 		PolyVerts2D.Add( FVector2D(Vf.X, Vf.Y) );
// 	}
// 	
// 	// 2) Generate interior jittered grid points
// 	double TargetLen = 1.5; 
// 	// compute boundary bbox
// 	FBox2D BB(EForceInit::ForceInit);
// 	for (auto& P : PolyVerts) BB += FVector2D(P);
// 	float Spacing = TargetLen * 0.9f;
// 	int NX = FMath::CeilToInt(BB.GetSize().X / Spacing);
// 	int NY = FMath::CeilToInt(BB.GetSize().Y / Spacing);
// 	
// 	TArray<FVector2f> InteriorPts;
// 	std::mt19937_64 RNG(FPlatformTime::Cycles());
// 	std::uniform_real_distribution<float> J(-0.5f,0.5f);
// 	
// 	// for (int ix = 0; ix < NX; ++ix)
// 	// {
// 	// 	for (int iy = 0; iy < NY; ++iy)
// 	// 	{
// 	// 		FVector2D Cent2D((float)Cent.X, (float)Cent.Y);  
// 	//
// 	// 		if (FGeomTools2D::IsPointInPolygon(FVector2d(P), PolyVerts))
// 	// 		{
// 	// 			InteriorPts.Add(FVector2f(P));
// 	// 		}
// 	// 	}
// 	// }
// 	
// 	// 3) Combine boundary + interior
// 	TArray<FVector2f> AllPts = PolyVerts;
// 	AllPts.Append(InteriorPts);
//
// 	
// 	// Step 2: Triangulate
// 	TArray<UE::Geometry::FIndex3i> Triangles;
// 	PolygonTriangulation::TriangulateSimplePolygon<float>(AllPts, Triangles, false);
//
//
//
// 	if (Triangles.Num() == 0)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
// 		return;
// 	}
//
//
// 	for (const FVector2f& V : PolyVerts)
// 	{
// 		int VID = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0)); // z = 0
// 		VertexIDs.Add(VID);
// 	}
// 	// Assume PolyVerts is TArray<FVector2f> of your samples in order
// 	float SignedArea = 0.f;
// 	int N = PolyVerts.Num();
// 	for (int i = 0; i < N; ++i)
// 	{
// 		const FVector2f& A = PolyVerts[i];
// 		const FVector2f& B = PolyVerts[(i+1) % N];
// 		SignedArea += (A.X * B.Y - B.X * A.Y);
// 	}
// 	SignedArea *= 0.5f;
//
// 	
// 	// Insert triangles
// 	for (const UE::Geometry::FIndex3i& Tri : Triangles)
// 	{
// 		Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
// 	}
//
//
// 	
// 	// Step 4: Extract to UE arrays
// 	TArray<FVector> Vertices;
// 	TArray<int32> Indices;
//
// 	for (int vid : Mesh.VertexIndicesItr())
// 	{
// 		FVector3d Pos = Mesh.GetVertex(vid);
// 		Vertices.Add(FVector(Pos.X, Pos.Y, Pos.Z));
// 	}
// 	
// 	// for (int tid : Mesh.TriangleIndicesItr())
// 	// {
// 	// 	UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 	// 	Indices.Add(Tri.C);
// 	// 	Indices.Add(Tri.B);
// 	// 	Indices.Add(Tri.A);
// 	// }
//
// 	// fix this later to always wind positive before triangluation but this is a temporary fix for now
// 	bool bReverseWinding = (SignedArea < 0.f);
// 	for (int tid : Mesh.TriangleIndicesItr())
// 	{
// 		UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 		if (bReverseWinding)
// 		{
// 			// flip each triangle
// 			Indices.Add(Tri.A);
// 			Indices.Add(Tri.B);
// 			Indices.Add(Tri.C);
// 		}
// 		else
// 		{
// 			// keep normal winding, here c to a is noremal winding
// 			Indices.Add(Tri.C);
// 			Indices.Add(Tri.B);
// 			Indices.Add(Tri.A);
// 		}
// 	}
// 	
// 	LastBuiltMesh           = MoveTemp(Mesh);
// 	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);
//
// 	// Step 5: Build procedural mesh
// 	CreateProceduralMesh(Vertices, Indices);
// }



// original version
// void SClothDesignCanvas::TriangulateAndBuildMesh(
// 	const FInterpCurve<FVector2D>& Shape,
// 	bool bRecordSeam,
// 	int32 StartPointIdx2D,
// 	int32 EndPointIdx2D
// )
// {
// 	if (Shape.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
//
// 	TArray<FVector2f> PolyVerts;
// 	const int SamplesPerSegment = 10;
//
// 	for (int SegIndex = 0; SegIndex < Shape.Points.Num() - 1; ++SegIndex)
// 	{
// 		float StartInVal = Shape.Points[SegIndex].InVal;
// 		float EndInVal   = Shape.Points[SegIndex + 1].InVal;
//
// 		for (int i = 0; i < SamplesPerSegment; ++i)
// 		{
// 			float Alpha = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
// 			FVector2D Sample = Shape.Eval(Alpha);
// 			PolyVerts.Add(FVector2f(Sample.X, Sample.Y));
// 		}
// 	}
// 	
// 	// Step 2: Triangulate
// 	TArray<UE::Geometry::FIndex3i> Triangles;
// 	PolygonTriangulation::TriangulateSimplePolygon<float>(PolyVerts, Triangles, false);
//
// 	if (Triangles.Num() == 0)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
// 		return;
// 	}
//
// 	// Step 3: Build DynamicMesh
// 	UE::Geometry::FDynamicMesh3 Mesh;
// 	// WORKING but not smooth at all so lots of different sized triangles
// 	// Insert vertices
// 	TArray<int> VertexIDs;
// 	for (const FVector2f& V : PolyVerts)
// 	{
// 		int VID = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0)); // z = 0
// 		VertexIDs.Add(VID);
// 	}
// 	
// 	// Insert triangles
// 	for (const UE::Geometry::FIndex3i& Tri : Triangles)
// 	{
// 		Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
// 	}
// 	
//
// 	
// 	LastSeamVertexIDs.Empty();
// 	int   SampleCounter = 0;
// 	int   TotalSamples  = (Shape.Points.Num() - 1) * SamplesPerSegment;
//
// 	// Insert vertices
// 	for (int SegIndex = 0; SegIndex < Shape.Points.Num() - 1; ++SegIndex) {
// 		float StartInVal = Shape.Points[SegIndex].InVal;
// 		float EndInVal   = Shape.Points[SegIndex+1].InVal;
//
// 		for (int i = 0; i < SamplesPerSegment; ++i, ++SampleCounter) {
// 			float Alpha = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
// 			FVector2D Sample2D = Shape.Eval(Alpha);
// 			int VID = Mesh.AppendVertex(FVector3d(Sample2D.X, Sample2D.Y, 0));
//
// 			// If this segment covers your 2D seam range, record it
// 			if (bRecordSeam) {
// 				// Map SampleCounter to an 2D-curve segment index:
// 				float t2d = (float)SampleCounter / (TotalSamples-1);
// 				// If t2d lies between your start/end 2D-curve InVals:
// 				float startT = Shape.Points[StartPointIdx2D].InVal;
// 				float endT   = Shape.Points[EndPointIdx2D].InVal;
// 				if (t2d >= startT && t2d <= endT) {
// 					LastSeamVertexIDs.Add(VID);
// 				}
// 			}
//
// 			VertexIDs.Add(VID);
// 		}
// 	}
// 	
//
//
//
//
// 	// // 2. Remesher setup
// 	// UE::Geometry::FRemesher Remesher(&Mesh);
// 	// Remesher.SetTargetEdgeLength(1.50); // Set this to desired density
// 	//
// 	// Remesher.SmoothSpeedT = 0.0f; // Optional tweak
// 	//
// 	// for (int i = 0; i < 2; ++i)  // 1 not enough, 3-5 too much in terms of edge vertices changing
// 	// {
// 	// 	// Remesher.BasicRemeshPass();
// 	// }
//
//
// 	// // // (Optional) Subdivide for more detail
// 	// // // UE::Geometry::FDynamicMeshEditor Editor(&Mesh);
// 	//
// 	// int32 NumSubdivisions = 3;
// 	// double MaxEdgeLength = 1.50;
// 	//
// 	// for (int32 Pass = 0; Pass < NumSubdivisions; ++Pass)
// 	// {
// 	// 	TArray<int32> EdgesToSplit;
// 	// 	for (int32 eid : Mesh.EdgeIndicesItr())
// 	// 	{
// 	// 		UE::Geometry::FIndex2i EdgeVerts = Mesh.GetEdgeV(eid);
// 	// 		FVector3d A = Mesh.GetVertex(EdgeVerts.A);
// 	// 		FVector3d B = Mesh.GetVertex(EdgeVerts.B);
// 	//
// 	// 		if (FVector3d::Distance(A, B) > MaxEdgeLength)
// 	// 		{
// 	// 			EdgesToSplit.Add(eid);
// 	// 		}
// 	// 	}
// 	//
// 	// 	for (int32 eid : EdgesToSplit)
// 	// 	{
// 	// 		UE::Geometry::FDynamicMesh3::FEdgeSplitInfo SplitInfo;
// 	// 		Mesh.SplitEdge(eid, SplitInfo);
// 	// 	}
// 	// }
// 	
// 	
// 	// Step 4: Extract to UE arrays
// 	TArray<FVector> Vertices;
// 	TArray<int32> Indices;
//
// 	for (int vid : Mesh.VertexIndicesItr())
// 	{
// 		FVector3d Pos = Mesh.GetVertex(vid);
// 		Vertices.Add(FVector(Pos.X, Pos.Y, Pos.Z));
// 	}
//
// 	for (int tid : Mesh.TriangleIndicesItr())
// 	{
// 		UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
// 		Indices.Add(Tri.C);
// 		Indices.Add(Tri.B);
// 		Indices.Add(Tri.A);
// 	}
//
// 	
// 	LastBuiltMesh           = MoveTemp(Mesh);
// 	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);
//
// 	// Step 5: Build procedural mesh
// 	CreateProceduralMesh(Vertices, Indices);
// }

void SClothDesignCanvas::CreateProceduralMesh(const TArray<FVector>& Vertices, const TArray<int32>& Indices)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	
	// Unique counter for naming
	static int32 MeshCounter = 0;
	FString UniqueLabel = FString::Printf(TEXT("ClothMeshActor_%d"), MeshCounter++);

	FActorSpawnParameters SpawnParams;
	AClothPatternMeshActor* MeshActor = World->SpawnActor<AClothPatternMeshActor>(SpawnParams);
	if (!MeshActor) return;
	
	// Record for later
	SpawnedPatternActors.Add(MeshActor);

	// (1) Move the mesh data into the actor
	MeshActor->DynamicMesh        = MoveTemp(LastBuiltMesh);

	// (2) Move the seam IDs in as well
	MeshActor->LastSeamVertexIDs = MoveTemp(LastBuiltSeamVertexIDs);

	
	FString FolderName = TEXT("GeneratedClothActors");
	MeshActor->SetFolderPath(FName(*FolderName));
	
	// Set a visible name in World Outliner
#if WITH_EDITOR
	MeshActor->SetActorLabel(UniqueLabel);
#endif

	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Normals.Add(FVector::UpVector);
		UV0.Add(FVector2D(Vertices[i].X * 0.01f, Vertices[i].Y * 0.01f));
		VertexColors.Add(FLinearColor::White);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
	}

	MeshActor->MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UV0, VertexColors, Tangents, true);

}





void SClothDesignCanvas::TriangulateAndBuildAllMeshes()
{

	// Loop over all completed shapes
	for (const FInterpCurve<FVector2D>& Shape : CompletedShapes)
	{
		TriangulateAndBuildMesh(Shape);
	}

	// Optionally, include current shape (if it's closed/finalized)
	if (CurvePoints.Points.Num() >= 3)
	{
		TriangulateAndBuildMesh(CurvePoints);
	}
}



void SClothDesignCanvas::AlignSeamMeshes(
	AClothPatternMeshActor* MeshActorA,
	AClothPatternMeshActor* MeshActorB)
{
	// // 2) Fetch world positions
	// TArray<FVector> WorldA, WorldB;
	//
	// for (int VID : MeshActorA->LastSeamVertexIDs)
	// {
	// 	FVector Loc = MeshActorA->DynamicMesh.GetVertex(VID);
	// 	WorldA.Add(MeshActorA->GetActorTransform().TransformPosition(Loc));
	// }
	// for (int VID : MeshActorB->LastSeamVertexIDs)
	// {
	// 	FVector Loc = MeshActorB->DynamicMesh.GetVertex(VID);
	// 	WorldB.Add(MeshActorB->GetActorTransform().TransformPosition(Loc));
	// }

	// Access the stored vertex IDs and dynamic meshes:
	const TArray<int32>& IDsA = MeshActorA->LastSeamVertexIDs;
	const TArray<int32>& IDsB = MeshActorB->LastSeamVertexIDs;

	// // Query world-space positions from each actor’s DynamicMesh:
	// TArray<FVector> WorldA, WorldB;
	//
	// for (int VID : IDsA)
	// {
	// 	FVector Local = (FVector)MeshActorA->DynamicMesh.GetVertex(VID);
	// 	WorldA.Add(MeshActorA->GetActorTransform().TransformPosition(Local));
	// }
	// for (int VID : IDsB)
	// {
	// 	FVector Local = (FVector)MeshActorB->DynamicMesh.GetVertex(VID);
	// 	WorldB.Add(MeshActorB->GetActorTransform().TransformPosition(Local));
	// }
	//
	// // Compute average offset, apply it...
	//
	//
	// // 3) Compute & apply offset
	// FVector TotalOff = FVector::ZeroVector;
	// const int Count = WorldA.Num();
	// for (int i = 0; i < Count; ++i)
	// {
	// 	TotalOff += (WorldA[i] - WorldB[i]);
	// }
	// FVector AvgOff = TotalOff / Count;
	//
	// FTransform T = MeshActorB->GetActorTransform();
	// T.AddToTranslation(AvgOff);
	// MeshActorB->SetActorTransform(T);
	//
	// UE_LOG(LogTemp, Log, TEXT("Aligned MeshB by %s"), *AvgOff.ToString());



	
	// 1) Guard against empty or mismatched arrays
	int32 NumA = IDsA.Num();
	int32 NumB = IDsB.Num();
	if (NumA == 0 || NumB == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot align: one of the seam lists is empty (A=%d, B=%d)"), NumA, NumB);
		return;
	}
	if (NumA != NumB)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot align: seam lists length mismatch (A=%d, B=%d)"), NumA, NumB);
		return;
	}

	// 2) Fetch world positions
	TArray<FVector> WorldA, WorldB;
	WorldA.Reserve(NumA);
	WorldB.Reserve(NumA);

	for (int i = 0; i < NumA; ++i)
	{
		FVector LocalA = FVector(MeshActorA->DynamicMesh.GetVertex(IDsA[i]));
		FVector LocalB = FVector(MeshActorB->DynamicMesh.GetVertex(IDsB[i]));
		WorldA.Add( MeshActorA->GetActorTransform().TransformPosition(LocalA) );
		WorldB.Add( MeshActorB->GetActorTransform().TransformPosition(LocalB) );
	}

	// 3) Compute average offset safely
	FVector TotalOffset = FVector::ZeroVector;
	for (int i = 0; i < NumA; ++i)
	{
		TotalOffset += (WorldA[i] - WorldB[i]);
	}

	// Now we know NumA > 0, so this is safe:
	FVector AverageOffset = TotalOffset / float(NumA);

	// 4) Apply the translation to MeshActorB
	FTransform NewTransform = MeshActorB->GetActorTransform();
	NewTransform.AddToTranslation(AverageOffset);
	MeshActorB->SetActorTransform(NewTransform);

	UE_LOG(LogTemp, Log, TEXT("Aligned MeshB by %s"), *AverageOffset.ToString());
}

// void SClothDesignCanvas::BuildAndAlignClickedSeam()
// {
// 	// 1) Rebuild all meshes so SpawnedPatternActors matches CompletedShapes order
// 	//TriangulateAndBuildAllMeshes();
//
// 	// 2) Determine which shapes the seam came from
// 	//    (Assuming AStartTarget.ShapeIndex and BStartTarget.ShapeIndex were filled)
// 	int32 ShapeAIdx = AStartTarget.ShapeIndex; // INDEX_NONE means the “current” shape
// 	int32 ShapeBIdx = BStartTarget.ShapeIndex;
//
// 	// 3) Translate shape indices to actor indices
// 	//    CompletedShapes are built first, then the current shape
// 	int32 ActorAIdx = ShapeAIdx == INDEX_NONE ? CompletedShapes.Num() : ShapeAIdx;
// 	int32 ActorBIdx = ShapeBIdx == INDEX_NONE ? CompletedShapes.Num() : ShapeBIdx;
//
// 	// 4) Grab the actors
// 	if ( SpawnedPatternActors.IsValidIndex(ActorAIdx) &&
// 		 SpawnedPatternActors.IsValidIndex(ActorBIdx) )
// 	{
// 		AClothPatternMeshActor* ActorA = SpawnedPatternActors[ActorAIdx].Get();
// 		AClothPatternMeshActor* ActorB = SpawnedPatternActors[ActorBIdx].Get();
// 		if (ActorA && ActorB)
// 		{
// 			AlignSeamMeshes(ActorA, ActorB);
// 		}
// 	}
// }

void SClothDesignCanvas::BuildAndAlignClickedSeam()
{
	// 1) Clear out previous actors
	for (auto& Weak : SpawnedPatternActors)
		if (auto* A = Weak.Get()) A->Destroy();
	SpawnedPatternActors.Empty();

	// 2) Build the first mesh (Shape A) and record its seam
	{
		int32 ShapeIdx = AStartTarget.ShapeIndex;
		const FInterpCurve<FVector2D>& Shape = 
			(ShapeIdx == INDEX_NONE) ? CurvePoints : CompletedShapes[ShapeIdx];
		TriangulateAndBuildMesh( Shape, /*bRecordSeam=*/true,
								 AStartTarget.PointIndex, AEndTarget.PointIndex );
	}

	// 3) Build the second mesh (Shape B) and record its seam
	{
		int32 ShapeIdx = BStartTarget.ShapeIndex;
		const FInterpCurve<FVector2D>& Shape = 
			(ShapeIdx == INDEX_NONE) ? CurvePoints : CompletedShapes[ShapeIdx];
		TriangulateAndBuildMesh( Shape, /*bRecordSeam=*/true,
								 BStartTarget.PointIndex, BEndTarget.PointIndex );
	}

	// 4) Now you have exactly two spawned actors with seams recorded
	if (SpawnedPatternActors.Num() == 2)
	{
		AlignSeamMeshes( SpawnedPatternActors[0].Get(),
						 SpawnedPatternActors[1].Get() );
	}
}


void SClothDesignCanvas::SaveStateForUndo()
{
	UndoStack.Add(GetCurrentCanvasState());
	RedoStack.Empty(); // Clear redo on new action
}

// FCanvasState SClothDesignCanvas::GetCurrentCanvasState() const
// {
// 	FCanvasState State;
// 	State.Points = Points;
// 	State.SelectedPointIndex = SelectedPointIndex;
// 	State.PanOffset = PanOffset;
// 	State.ZoomFactor = ZoomFactor;
// 	return State;
// }
FCanvasState SClothDesignCanvas::GetCurrentCanvasState() const
{
	FCanvasState State;
	State.CurvePoints = CurvePoints; // Full copy, includes tangents and interp mode
	State.CompletedShapes = CompletedShapes;
	
	State.bUseBezierPerPoint = bUseBezierPerPoint;
	State.CompletedBezierFlags = CompletedBezierFlags;
	
	State.SelectedPointIndex = SelectedPointIndex;
	State.PanOffset = PanOffset;
	State.ZoomFactor = ZoomFactor;
	return State;
}


void SClothDesignCanvas::RestoreCanvasState(const FCanvasState& State)
{
	// to avoid the redo/undo crashes
	ensure(State.CurvePoints.Points.Num() == State.bUseBezierPerPoint.Num());
	ensure(State.CompletedShapes.Num() == State.CompletedBezierFlags.Num());
	
	CurvePoints = State.CurvePoints;
	CompletedShapes = State.CompletedShapes;
	
	bUseBezierPerPoint = State.bUseBezierPerPoint;
	CompletedBezierFlags = State.CompletedBezierFlags;
	
	SelectedPointIndex = State.SelectedPointIndex;
	PanOffset = State.PanOffset;
	ZoomFactor = State.ZoomFactor;
	
	SelectedPointIndex = INDEX_NONE;
	SelectedShapeIndex = INDEX_NONE;
	
	Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
}


void SClothDesignCanvas::Undo()
{
	if (UndoStack.Num() > 0)
	{
		RedoStack.Add(GetCurrentCanvasState());
		const FCanvasState PreviousState = UndoStack.Pop();
		RestoreCanvasState(PreviousState);
	}
}

void SClothDesignCanvas::Redo()
{
	if (RedoStack.Num() > 0)
	{
		UndoStack.Add(GetCurrentCanvasState());
		const FCanvasState NextState = RedoStack.Pop();
		RestoreCanvasState(NextState);
	}
}

void SClothDesignCanvas::AddSewingConstraints(
	AActor* PatternPiece1,
	const TArray<int32>& Vertices1,
	AActor* PatternPiece2,
	const TArray<int32>& Vertices2,
	float Stiffness,
	TArray<FPatternSewingConstraint>& OutConstraints
)
{
	if (!PatternPiece1 || !PatternPiece2) return;
	if (Vertices1.Num() != Vertices2.Num()) return;

	USkeletalMeshComponent* Mesh1 = PatternPiece1->FindComponentByClass<USkeletalMeshComponent>();
	USkeletalMeshComponent* Mesh2 = PatternPiece2->FindComponentByClass<USkeletalMeshComponent>();


	if (!Mesh1 || !Mesh2) return;

	for (int32 i = 0; i < Vertices1.Num(); ++i)
	{
		FPatternSewingConstraint Constraint;
		Constraint.MeshA = Mesh1;
		Constraint.VertexIndexA = Vertices1[i];
		Constraint.MeshB = Mesh2;
		Constraint.VertexIndexB = Vertices2[i];
		Constraint.Stiffness = Stiffness;

		OutConstraints.Add(Constraint);
	}
}


void SClothDesignCanvas::SewingTest()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;
	
	AActor* PatternPiece1 = nullptr;
	AActor* PatternPiece2 = nullptr;
	
	// USkeletalMeshComponent* MeshComp1 = PatternPiece1->FindComponentByClass<USkeletalMeshComponent>();
	// USkeletalMeshComponent* MeshComp2 = PatternPiece2->FindComponentByClass<USkeletalMeshComponent>();

	// Option 1: Lookup by name (label in the outliner, not class)
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->GetActorLabel() == TEXT("SKM_pattern_piece_1"))
		{
			PatternPiece1 = Actor;
		}
		else if (Actor->GetActorLabel() == TEXT("SKM_pattern_piece_2"))
		{
			PatternPiece2 = Actor;
		}
	}
	
	if (!PatternPiece1 || !PatternPiece2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pattern pieces not found in scene."));
		return;
	}

	
	// Define vertex indices
	TArray<int32> VerticesA = {2, 5, 8};
	TArray<int32> VerticesB = {6, 7, 8};
	
	// Store constraints
	// TArray<FPatternSewingConstraint> SewingConstraints;
	SewingConstraints.Reset();

	AddSewingConstraints(PatternPiece1, VerticesA, PatternPiece2, VerticesB, 1.0f, SewingConstraints);

	UE_LOG(LogTemp, Warning, TEXT("Added %d uskel constraints."), SewingConstraints.Num());
}



void SClothDesignCanvas::SewingStart()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	AActor* PatternPiece1 = nullptr;
	AActor* PatternPiece2 = nullptr;

	// Find actors
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->GetActorLabel() == TEXT("SKM_pattern_piece_1")) PatternPiece1 = Actor;
		if (Actor->GetActorLabel() == TEXT("SKM_pattern_piece_2")) PatternPiece2 = Actor;
	}

	if (!PatternPiece1 || !PatternPiece2) return;

	USkeletalMeshComponent* Mesh1 = PatternPiece1->FindComponentByClass<USkeletalMeshComponent>();
	USkeletalMeshComponent* Mesh2 = PatternPiece2->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh1 || !Mesh2) return;

	TArray<int32> VerticesA = {8, 7, 5}; 
	TArray<int32> VerticesB = {1, 2, 5};
	// the vertex order from houdini of 0 to 8 in rows of three translates to this order in UE: 4, 6, 8, then 0, 3, 7, then 1, 2, 5

	if (VerticesA.Num() != VerticesB.Num()) return;

	// Get rest pose positions
	FSkeletalMeshRenderData* RenderData1 = Mesh1->GetSkeletalMeshRenderData();
	FSkeletalMeshRenderData* RenderData2 = Mesh2->GetSkeletalMeshRenderData();

	if (!RenderData1 || !RenderData2) return;

	const FPositionVertexBuffer& VertexBuffer1 = RenderData1->LODRenderData[0].StaticVertexBuffers.PositionVertexBuffer;
	const FPositionVertexBuffer& VertexBuffer2 = RenderData2->LODRenderData[0].StaticVertexBuffers.PositionVertexBuffer;

	// Compute average offset
	FVector TotalOffset = FVector::ZeroVector;
	int32 PairCount = VerticesA.Num();

	for (int32 i = 0; i < PairCount; ++i)
	{
		int32 IndexA = VerticesA[i];
		int32 IndexB = VerticesB[i];

		if (IndexA >= VertexBuffer1.GetNumVertices() || IndexB >= VertexBuffer2.GetNumVertices()) continue;

		FVector PosA = Mesh1->GetComponentTransform().TransformPosition(FVector(VertexBuffer1.VertexPosition(IndexA)));
		FVector PosB = Mesh2->GetComponentTransform().TransformPosition(FVector(VertexBuffer2.VertexPosition(IndexB)));

		FVector Delta = PosA - PosB;
		TotalOffset += Delta;
	}

	
	// for (int32 i = 0; i < VertexBuffer1.GetNumVertices(); ++i)
	// {
	// 	FVector LocalPos = FVector(VertexBuffer1.VertexPosition(i)); // FVector3f → FVector
	// 	FVector WorldPos = Mesh1->GetComponentTransform().TransformPosition(LocalPos);
	//
	// 	UE_LOG(LogTemp, Warning, TEXT("Vertex %d: %s"), i, *WorldPos.ToString());
	// }
	//
	//
	//
	
	FVector AverageOffset = TotalOffset / PairCount;

	// Apply offset to pattern_piece_2
	FTransform CurrentTransform = PatternPiece2->GetActorTransform();
	CurrentTransform.AddToTranslation(AverageOffset);
	PatternPiece2->SetActorTransform(CurrentTransform);

	UE_LOG(LogTemp, Warning, TEXT("Moved mesh by %s to preview sewing."), *AverageOffset.ToString());
}



UClothingSimulationInteractor* GetClothInteractor(USkeletalMeshComponent* Mesh)
{
	if (!Mesh) return nullptr;

	return Mesh->GetClothingSimulationInteractor();
}

void SClothDesignCanvas::ApplySpringSeamForces(
	const TArray<FPatternSewingConstraint>& Constraints,
	float DeltaTime)
{
	
	if (Constraints.Num() == 0) return;

	// —————————————————————————————————————————
	// 1) Grab the Chaos Clothing Interactor
	// —————————————————————————————————————————
	USkeletalMeshComponent* Mesh = Constraints[0].MeshA;
	if (!Mesh) return;

	// UClothingSimulationInteractor is the base, cast to Chaos version:
	UChaosClothingSimulationInteractor* ChaosInteractor =
		Cast<UChaosClothingSimulationInteractor>(
			Mesh->GetClothingSimulationInteractor()
		);
	if (!ChaosInteractor) return;
	
	
}




void SClothDesignCanvas::FinalizeSeamDefinition(
	const FVector2D& AStart,
	const FVector2D& AEnd,
	const FVector2D& BStart,
	const FVector2D& BEnd)
{
	// Step 1: Find the nearest mesh + edge for each click range (optional or future logic)

	// Step 2: Sample N points along each line
	const int32 NumSeamPoints = 10;
	TArray<FVector2D> PointsA, PointsB;

	for (int32 i = 0; i < NumSeamPoints; ++i)
	{
		float Alpha = (float)i / (NumSeamPoints - 1);
		PointsA.Add(FMath::Lerp(AStart, AEnd, Alpha));
		PointsB.Add(FMath::Lerp(BStart, BEnd, Alpha));
	}

	// Step 3: Store seam definition
	FPatternSewingConstraint NewSeam;
	NewSeam.ScreenPointsA = PointsA;
	NewSeam.ScreenPointsB = PointsB;

	AllDefinedSeams.Add(NewSeam);

	
	UE_LOG(LogTemp, Log, TEXT("Seam defined with %d points per side."), NumSeamPoints);
	UE_LOG(LogTemp, Log, TEXT("AStart: (%.2f, %.2f), AEnd: (%.2f, %.2f)"), AStart.X, AStart.Y, AEnd.X, AEnd.Y);
	UE_LOG(LogTemp, Log, TEXT("BStart: (%.2f, %.2f), BEnd: (%.2f, %.2f)"), BStart.X, BStart.Y, BEnd.X, BEnd.Y);
}



void SClothDesignCanvas::MergeLastTwoMeshes()
{
    // 1) Make sure we have at least two actors
    if (SpawnedPatternActors.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("Need at least two meshes to merge"));
        return;
    }
    // Grab the last two
    AClothPatternMeshActor* A = SpawnedPatternActors[SpawnedPatternActors.Num() - 2].Get();
    AClothPatternMeshActor* B = SpawnedPatternActors[SpawnedPatternActors.Num() - 1].Get();
    if (!A || !B)
    {
        UE_LOG(LogTemp, Warning, TEXT("One of the two actors is invalid"));
        return;
    }

    // 2) Copy their dynamic meshes
    UE::Geometry::FDynamicMesh3 MergedMesh = A->DynamicMesh;
    int32 BaseVID = MergedMesh.VertexCount();

    // // Append B’s vertices
    // for (int32 VID : ActorB->DynamicMesh.VertexIndicesItr())
    // {
    //     FVector3d P = ActorB->DynamicMesh.GetVertex(VID);
    //     MergedMesh.AppendVertex(P);
    // }
    //
    // // Append B’s triangles (offset indices by BaseVID)
    // for (int32 TID : ActorB->DynamicMesh.TriangleIndicesItr())
    // {
    //     UE::Geometry::FIndex3i Tri = ActorB->DynamicMesh.GetTriangle(TID);
    //     MergedMesh.AppendTriangle(
    //         Tri.A + BaseVID,
    //         Tri.B + BaseVID,
    //         Tri.C + BaseVID
    //     );
    // }
	const FTransform& TA = A->GetActorTransform();
	TArray<int> MapA;
	MapA.SetNum(A->DynamicMesh.VertexCount());

	// 1) Add all A’s vertices in world space
	for (int vid : A->DynamicMesh.VertexIndicesItr())
	{
		FVector3d LocalP = A->DynamicMesh.GetVertex(vid);
		FVector    WorldP = TA.TransformPosition((FVector)LocalP);
		MapA[vid] = MergedMesh.AppendVertex(FVector3d(WorldP));
	}
	// 2) Add A’s triangles with remapped indices
	for (int tid : A->DynamicMesh.TriangleIndicesItr())
	{
		auto T = A->DynamicMesh.GetTriangle(tid);
		MergedMesh.AppendTriangle(
			MapA[T.A], MapA[T.B], MapA[T.C]
		);
	}
	
	const FTransform& TB = B->GetActorTransform();
	TArray<int> MapB;
	MapB.SetNum(B->DynamicMesh.VertexCount());

	for (int vid : B->DynamicMesh.VertexIndicesItr())
	{
		FVector3d LocalP = B->DynamicMesh.GetVertex(vid);
		FVector    WorldP = TB.TransformPosition((FVector)LocalP);
		MapB[vid] = MergedMesh.AppendVertex(FVector3d(WorldP));
	}
	for (int tid : B->DynamicMesh.TriangleIndicesItr())
	{
		auto T = B->DynamicMesh.GetTriangle(tid);
		MergedMesh.AppendTriangle(
			MapB[T.A], MapB[T.B], MapB[T.C]
		);
	}
	
	
	if (!MergedMesh.HasAttributes())
	{
		MergedMesh.EnableAttributes();
	}
	
	
    // 3) Extract to raw arrays for ProceduralMeshComponent
    TArray<FVector> Vertices;
    TArray<int32>   Indices;
    Vertices.Reserve(MergedMesh.VertexCount());
    Indices.Reserve(MergedMesh.TriangleCount() * 3);

    for (int32 VID : MergedMesh.VertexIndicesItr())
    {
        FVector3d P = MergedMesh.GetVertex(VID);
        Vertices.Add(FVector(P.X, P.Y, P.Z));
    }
    for (int32 TID : MergedMesh.TriangleIndicesItr())
    {
        UE::Geometry::FIndex3i Tri = MergedMesh.GetTriangle(TID);
        Indices.Add(Tri.A);
        Indices.Add(Tri.B);
        Indices.Add(Tri.C);
    }

    // 4) Spawn a new actor for the merged mesh
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return;

    FActorSpawnParameters Params;
    AClothPatternMeshActor* MergedActor =
        World->SpawnActor<AClothPatternMeshActor>(Params);
    if (!MergedActor) return;

#if WITH_EDITOR
    MergedActor->SetActorLabel(TEXT("MergedPatternMesh"));
#endif

    // Store the merged dynamic mesh
    MergedActor->DynamicMesh = MoveTemp(MergedMesh);

    // Create the visible section
    TArray<FVector> Normals; Normals.Init(FVector::UpVector, Vertices.Num());
    TArray<FVector2D> UV0;  UV0.Init(FVector2D::ZeroVector, Vertices.Num());
    TArray<FLinearColor> VertexColors; VertexColors.Init(FLinearColor::White, Vertices.Num());
    TArray<FProcMeshTangent> Tangents;   Tangents.Init(FProcMeshTangent(1,0,0), Vertices.Num());

    MergedActor->MeshComponent->CreateMeshSection_LinearColor(
        0, Vertices, Indices,
        Normals, UV0, VertexColors, Tangents,
        /*bCreateCollision=*/true
    );

    UE_LOG(LogTemp, Log, TEXT("Merged two meshes into '%s' (%d verts, %d tris)"),
        *MergedActor->GetActorLabel(),
        Vertices.Num(), Indices.Num() / 3
    );
}





void SClothDesignCanvas::MergeAndWeldLastTwoMeshes()
{
    // 1) Grab the last two actors
    if (SpawnedPatternActors.Num() < 2) { return; }
    AClothPatternMeshActor* A = SpawnedPatternActors.Last(1).Get();
    AClothPatternMeshActor* B = SpawnedPatternActors.Last().Get();
    if (!A || !B) { return; }

    // 2) Merge their DynamicMesh3’s
    UE::Geometry::FDynamicMesh3 Merged = A->DynamicMesh;
    int32 BaseVID = Merged.VertexCount();
	
    // for (int vid : B->DynamicMesh.VertexIndicesItr())
    // {
    //     Merged.AppendVertex(B->DynamicMesh.GetVertex(vid));
    // }
    // for (int tid : B->DynamicMesh.TriangleIndicesItr())
    // {
    //     auto T = B->DynamicMesh.GetTriangle(tid);
    //     Merged.AppendTriangle(T.A + BaseVID, T.B + BaseVID, T.C + BaseVID);
    // }
	// -- (A) Append A’s world‑transformed vertices & triangles --
	
    const FTransform& TA = A->GetActorTransform();
    TArray<int> MapA;
    MapA.SetNum(A->DynamicMesh.VertexCount());

    // 1) Add all A’s vertices in world space
    for (int vid : A->DynamicMesh.VertexIndicesItr())
    {
    	FVector3d LocalP = A->DynamicMesh.GetVertex(vid);
    	FVector    WorldP = TA.TransformPosition((FVector)LocalP);
    	MapA[vid] = Merged.AppendVertex(FVector3d(WorldP));
    }
    // 2) Add A’s triangles with remapped indices
    for (int tid : A->DynamicMesh.TriangleIndicesItr())
    {
    	auto T = A->DynamicMesh.GetTriangle(tid);
    	Merged.AppendTriangle(
			MapA[T.A], MapA[T.B], MapA[T.C]
		);
    }
	

	// -- (B) Append B’s world‑transformed vertices & triangles --
	// int32 BaseVID = Merged.VertexCount();
	
    const FTransform& TB = B->GetActorTransform();
    TArray<int> MapB;
    MapB.SetNum(B->DynamicMesh.VertexCount());

    for (int vid : B->DynamicMesh.VertexIndicesItr())
    {
    	FVector3d LocalP = B->DynamicMesh.GetVertex(vid);
    	FVector    WorldP = TB.TransformPosition((FVector)LocalP);
    	MapB[vid] = Merged.AppendVertex(FVector3d(WorldP));
    }
    for (int tid : B->DynamicMesh.TriangleIndicesItr())
    {
    	auto T = B->DynamicMesh.GetTriangle(tid);
    	Merged.AppendTriangle(
			MapB[T.A], MapB[T.B], MapB[T.C]
		);
    }
	




	
	
	// 2) Build loops of exactly two vertices each, for each seam pair
	//TArray<TArray<int>> WeldLoops;
	// int32 Count = FMath::Min(A->LastSeamVertexIDs.Num(), B->LastSeamVertexIDs.Num());
	// WeldLoops.Reserve(Count);
	// for (int32 i = 0; i < Count; ++i)
	// {
	// 	int VA = A->LastSeamVertexIDs[i];
	// 	int VB = B->LastSeamVertexIDs[i] + BaseVID;
	// 	WeldLoops.Add( TArray<int>({ VA, VB }) );
	// }

	
	if (!Merged.HasAttributes())
	{
		Merged.EnableAttributes();
	}
	
	
    // 3) Weld each seam‐vertex pair
    UE::Geometry::FDynamicMeshEditor Editor(&Merged);
	TArray<int32> LoopA;
	TArray<int32> LoopB;
	int32 Count = FMath::Min(A->LastSeamVertexIDs.Num(), B->LastSeamVertexIDs.Num());

	// for (int32 i = 0; i < Count; ++i)
	// {
	// 	int VA = A->LastSeamVertexIDs[i];
	// 	int VB = B->LastSeamVertexIDs[i] + BaseVID;
	// 	LoopA.Add(VA);
	// 	LoopB.Add(VB);
	// 	
	// 	// TArray<int32> Pair = { VA, VB };
	// 	// TArray<int32> Removed;
	// 	// Editor.WeldVertexLoops(Pair, Removed);
	// }
	
	for (int32 i = 0; i < Count; ++i)
	{
		// origVA is the index in A->DynamicMesh
		int32 origVA = A->LastSeamVertexIDs[i];
		// The new ID in Merged is:
		int32 mergedVA = MapA[origVA];
		LoopA.Add( mergedVA );

		// origVB is the index in B->DynamicMesh
		int32 origVB = B->LastSeamVertexIDs[i];
		// The new ID in Merged is:
		int32 mergedVB = MapB[origVB];
		LoopB.Add( mergedVB );
	}
	
	UE_LOG(LogTemp, Warning, TEXT("LoopA has %d vertices"), LoopA.Num());
	UE_LOG(LogTemp, Warning, TEXT("LoopB has %d vertices"), LoopB.Num());






	
	if (LoopA.Num() != LoopB.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Loop sizes don't match: A=%d, B=%d"), LoopA.Num(), LoopB.Num());
		return;
	}
	
	for (int32 i = 0; i < LoopA.Num(); ++i)
	{
		int32 VA = LoopA[i];
		int32 VB = LoopB[i];

		// Move VB to the position of VA
		Merged.SetVertex(VB, Merged.GetVertex(VA));

		// Replace all uses of VB in triangles with VA
		TArray<int32> TrianglesToUpdate;
		Merged.GetVtxTriangles(VB, TrianglesToUpdate);

		for (int32 TID : TrianglesToUpdate)
		{
			auto Tri = Merged.GetTriangle(TID);
			for (int j = 0; j < 3; ++j)
			{
				if (Tri[j] == VB)
				{
					Tri[j] = VA;
				}
			}
			Merged.SetTriangle(TID, Tri);
		}

		// Optionally delete VB if it's no longer referenced
		if (Merged.IsVertex(VB) && Merged.GetVtxTriangleCount(VB) == 0)
		{
			Merged.RemoveVertex(VB, false);
		}
	}

	
	// 4) Export welded mesh to a new actor
	//    (same code as before to flatten vertices+indices + spawn)
	TArray<FVector>    Verts;    Verts.Reserve(Merged.VertexCount());
	TArray<int32>      Inds;     Inds.Reserve(Merged.TriangleCount()*3);
	for (int vid : Merged.VertexIndicesItr())
	{
		auto P = Merged.GetVertex(vid);
		Verts.Add(FVector(P.X,P.Y,P.Z));
	}
	for (int tid : Merged.TriangleIndicesItr())
	{
		auto T = Merged.GetTriangle(tid);
		Inds.Add(T.A); Inds.Add(T.B); Inds.Add(T.C);
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;
	AClothPatternMeshActor* MergedActor =
		World->SpawnActor<AClothPatternMeshActor>();
	
#if WITH_EDITOR
	MergedActor->SetActorLabel(TEXT("WeldedPatternMesh"));
#endif
	MergedActor->DynamicMesh = MoveTemp(Merged);

	TArray<FVector> Normals;     Normals.Init(FVector::UpVector, Verts.Num());
	TArray<FVector2D> UV0;       UV0.Init(FVector2D::ZeroVector, Verts.Num());
	TArray<FLinearColor> Colors; Colors.Init(FLinearColor::White,  Verts.Num());
	TArray<FProcMeshTangent> Tangs; Tangs.Init(FProcMeshTangent(1,0,0), Verts.Num());

	MergedActor->MeshComponent->CreateMeshSection_LinearColor(
		0, Verts, Inds,
		Normals, UV0, Colors, Tangs,
		/*bCreateCollision=*/true
	);

	UE_LOG(LogTemp, Log, TEXT("Merged & welded: %d verts, %d tris"),
		   Verts.Num(), Inds.Num()/3);
}





bool SClothDesignCanvas::SaveShapeAsset(const FString& AssetPath, const FString& AssetName)
{
	// Create package path - e.g. /Game/YourFolder/AssetName
	FString PackageName = FString::Printf(TEXT("/Game/%s/%s"), *AssetPath, *AssetName);
	FString SanitizedPackageName = PackageTools::SanitizePackageName(PackageName);

	UPackage* Package = LoadPackage(nullptr, *SanitizedPackageName, LOAD_None);
	if (!Package)
	{
		// If the package doesn't exist, create it
		Package = CreatePackage(*SanitizedPackageName);
	}
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for saving asset"));
		return false;
	}

	// Force full load of the package (if not already)
	Package->FullyLoad();

	
	// Try to find existing asset
	UClothShapeAsset* ExistingAsset = FindObject<UClothShapeAsset>(Package, *AssetName);
	UClothShapeAsset* TargetAsset = ExistingAsset;

	if (!TargetAsset)
	{
		// If it doesn't exist, create a new one
		TargetAsset = NewObject<UClothShapeAsset>(Package, *AssetName, RF_Public | RF_Standalone);
		if (!TargetAsset)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create new UClothShapeAsset"));
			return false;
		}
	}

	// Clear and copy your canvas data to the asset
	TargetAsset->ClothShapes.Empty();
	TargetAsset->ClothCurvePoints.Empty();



	
	// Copy all completed shapes into asset
	// for (const auto& ShapeCurve : CompletedShapes)
	// {
	for (int32 ShapeIdx = 0; ShapeIdx < CompletedShapes.Num(); ++ShapeIdx)
	{
		const auto& ShapeCurve = CompletedShapes[ShapeIdx];
		const auto& ShapeFlags = CompletedBezierFlags[ShapeIdx];
		
		FShapeData SavedShape;
		

		// int32 i = 0;
		// for (const auto& Point : ShapeCurve.Points)
		// {
		for (int32 i = 0; i < ShapeCurve.Points.Num(); ++i)
		{
			const auto& Point = ShapeCurve.Points[i];

			FCurvePointData NewPoint;
			NewPoint.InputKey = Point.InVal;
			NewPoint.Position = Point.OutVal;
			NewPoint.ArriveTangent = Point.ArriveTangent;
			NewPoint.LeaveTangent = Point.LeaveTangent;
			NewPoint.bUseBezier   = ShapeFlags.IsValidIndex(i) 
									  ? ShapeFlags[i] 
									  : true;
			SavedShape.CompletedClothShape.Add(NewPoint);
			// ++i;
		}

		TargetAsset->ClothShapes.Add(SavedShape);
	}



	
	// Iterate over your FInterpCurve keys (points)
	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
	{
		const FInterpCurvePoint<FVector2D>& Point = CurvePoints.Points[i];

		FCurvePointData NewPoint;
		NewPoint.InputKey = Point.InVal;
		NewPoint.Position = Point.OutVal;
		NewPoint.ArriveTangent = Point.ArriveTangent;
		NewPoint.LeaveTangent = Point.LeaveTangent;
		NewPoint.bUseBezier = bUseBezierPerPoint.IsValidIndex(i) ? bUseBezierPerPoint[i] : true;


		// Add to the array
		TargetAsset->ClothCurvePoints.Add(NewPoint);
	}

	
	// Mark dirty and notify asset registry only once
	TargetAsset->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(TargetAsset);
	

	// Save package to disk
	FString PackageFileName = FPackageName::LongPackageNameToFilename(SanitizedPackageName, FPackageName::GetAssetPackageExtension());
	bool bSaved = UPackage::SavePackage(Package, TargetAsset, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName);

	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("Asset saved successfully: %s"), *PackageFileName);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save asset: %s"), *PackageFileName);
		return false;
	}
}

void SClothDesignCanvas::LoadShapeAssetData()
{
	if (!ClothAsset.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid shape asset to load."));
		return;
	}

	// Clear existing data first
	// ClearCurrentShapeData();
	CompletedShapes.Empty();
	CompletedBezierFlags.Empty();

	CurvePoints.Points.Empty();
	bUseBezierPerPoint.Empty();
	
	// Reset selection and indices
	SelectedPointIndex = INDEX_NONE;
	SelectedShapeIndex = INDEX_NONE;
	
	Invalidate(EInvalidateWidgetReason::Paint);
	
	for (const auto& SavedShape : ClothAsset->ClothShapes)
	{
		//FInterpCurve<FVector2D> LoadedCurve;
		//
		CurvePoints.Points.Empty();
		bUseBezierPerPoint.Empty();
		UE_LOG(LogTemp, Log, TEXT("Trying to load shapes!"));

		for (const auto& SavedPoint : SavedShape.CompletedClothShape)
		{
			AddPointToCanvas(SavedPoint);
		}
		
		
		if (CurvePoints.Points.Num() != bUseBezierPerPoint.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("Mismatch between points and Bezier flags!"));
			// Handle error, maybe fix array sizes here
		}
		
		if (bUseBezierPerPoint.Num() == 0)
		{
			// Avoid accessing Last()
			return;
		}
		
		FInterpCurve<FVector2D> CopiedCurve = CurvePoints;
		
		// 3. Only add if something was loaded
		if (CurvePoints.Points.Num() > 0)
		{
			CompletedShapes.Add(CopiedCurve);
			CompletedBezierFlags.Add(bUseBezierPerPoint);

		}

		// UE_LOG(LogTemp, Log, TEXT("Loaded shape %d has %d points, flags: [%s]"),
		// 	CompletedShapes.Num(),
		// 	CompletedBezierFlags.Last().Num(),
		// 	*FString::JoinBy(CompletedBezierFlags.Last(), TEXT(","), [](bool b){ return b ? TEXT("B") : TEXT("N"); })
		// );
		

	}
	
	// Now load active (incomplete) working shape
	// CurvePoints.Points.Empty();
	// bUseBezierPerPoint.Empty();
	// ClearCurrentShapeData();
	//ClearCurvePointArrays();
	SelectedPointIndex = INDEX_NONE;
	SelectedShapeIndex = INDEX_NONE;
	

	CurvePoints.Points.Empty();
	bUseBezierPerPoint.Empty();

	
	// Example: Suppose your UMyShapeAsset has a TArray<FCurvePointData> called SavedPoints
	for (const FCurvePointData& Point : ClothAsset->ClothCurvePoints)
	{
		UE_LOG(LogTemp, Log, TEXT("Loaded Point: %s"), *Point.Position.ToString());
		AddPointToCanvas(Point);
	}


	// Refresh the canvas/UI to display loaded data
	Invalidate(EInvalidateWidgetReason::Paint);

}


FString SClothDesignCanvas::GetSelectedShapeAssetPath() const
{
	return ClothAsset.IsValid() ? ClothAsset->GetPathName() : FString();
}

void SClothDesignCanvas::OnShapeAssetSelected(const FAssetData& AssetData)
{
	ClothAsset = Cast<UClothShapeAsset>(AssetData.GetAsset());

	if (ClothAsset.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Selected shape: %s"), *ClothAsset->GetName());

		LoadShapeAssetData();
	}
}




// void SClothDesignCanvas::AddPointToCanvas(const FCurvePointData& Point)
// {
// 	// Assuming you have an array or structure holding your points:
// 	FInterpCurvePoint<FVector2D> NewPoint;
// 	CurvePoints.Points.Add(NewPoint);
//
// 	
// }

void SClothDesignCanvas::AddPointToCanvas(const FCurvePointData& Point)
{
	FInterpCurvePoint<FVector2D> NewPoint;

	NewPoint.InVal = Point.InputKey;
	NewPoint.OutVal = Point.Position;
	NewPoint.ArriveTangent = Point.ArriveTangent;
	NewPoint.LeaveTangent = Point.LeaveTangent;
	NewPoint.InterpMode = CIM_CurveAuto; // or use Point.InterpMode if you stored that

	CurvePoints.Points.Add(NewPoint);
	//bUseBezierPerPoint.Add(true); // or Point.bUseBezier if you have it
	bUseBezierPerPoint.Add(Point.bUseBezier);

	if (bUseBezierPerPoint.Num() > 0 && !bUseBezierPerPoint.Last())
	{
		UE_LOG(LogTemp, Log, TEXT("N point loading!"));
		RecalculateNTangents(CurvePoints, bUseBezierPerPoint);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("B point loading!"));
		CurvePoints.AutoSetTangents();
	}

	// Manually fix first/last tangents if needed (like you do on click)
	const int32 NumPts = CurvePoints.Points.Num();
	if (NumPts >= 2)
	{
		// First point
		CurvePoints.Points[0].ArriveTangent = FVector2D::ZeroVector;
		CurvePoints.Points[0].LeaveTangent = (CurvePoints.Points[1].OutVal - CurvePoints.Points[0].OutVal) * 0.5f;

		// Last point
		const int32 LastIdx = NumPts - 1;
		CurvePoints.Points[LastIdx].ArriveTangent = (CurvePoints.Points[LastIdx].OutVal - CurvePoints.Points[LastIdx - 1].OutVal) * 0.5f;
		CurvePoints.Points[LastIdx].LeaveTangent = FVector2D::ZeroVector;
	}

	// Invalidate canvas for redraw
	Invalidate(EInvalidateWidgetReason::Paint);
}



void SClothDesignCanvas::ClearCurrentShapeData()
{
	CompletedShapes.Empty();
	CompletedBezierFlags.Empty();

	CurvePoints.Points.Empty();
	bUseBezierPerPoint.Empty();
	
	// Reset selection and indices
	SelectedPointIndex = INDEX_NONE;
	SelectedShapeIndex = INDEX_NONE;
	
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SClothDesignCanvas::ClearCurvePointArrays()
{
	// // Use temp arrays to safely invalidate memory & avoid crash
	// TArray<FInterpCurvePoint<FVector2D>> DummyPoints;
	// TArray<bool> DummyFlags;
	//
	// CurvePoints.Points = MoveTemp(DummyPoints);
	// bUseBezierPerPoint = MoveTemp(DummyFlags);
	//CompletedShapes.Empty();
	// Reset selection and indices
	SelectedPointIndex = INDEX_NONE;
	SelectedShapeIndex = INDEX_NONE;
	

	CurvePoints.Points.Empty();
	bUseBezierPerPoint.Empty();

	
	// Invalidate(EInvalidateWidgetReason::Paint);
}









//     // 4) Export welded mesh to new actor
//     //   (similar to your previous spawn logic)
//     UWorld* World = GEditor->GetEditorWorldContext().World();
//     if (!World) return;
//     FActorSpawnParameters Params;
//     AClothPatternMeshActor* MergedActor =
//         World->SpawnActor<AClothPatternMeshActor>(Params);
// #if WITH_EDITOR
//     MergedActor->SetActorLabel(TEXT("WeldedPatternMesh"));
// #endif
//
//     // Store welded mesh
//     MergedActor->DynamicMesh = MoveTemp(Merged);
//
//     // Build ProceduralMeshComponent section
//     TArray<FVector>    Verts;    Verts.Reserve(MergedActor->DynamicMesh.VertexCount());
//     TArray<int32>      Inds;     Inds.Reserve(MergedActor->DynamicMesh.TriangleCount()*3);
//     for (int vid : MergedActor->DynamicMesh.VertexIndicesItr())
//     {
//         auto P = MergedActor->DynamicMesh.GetVertex(vid);
//         Verts.Add(FVector(P.X, P.Y, P.Z));
//     }
//     for (int tid : MergedActor->DynamicMesh.TriangleIndicesItr())
//     {
//         auto T = MergedActor->DynamicMesh.GetTriangle(tid);
//         Inds.Add(T.A); Inds.Add(T.B); Inds.Add(T.C);
//     }
//
//     TArray<FVector>       Normals;     Normals.Init(FVector::UpVector,    Verts.Num());
//     TArray<FVector2D>     UV0;         UV0.Init(FVector2D::ZeroVector,  Verts.Num());
//     TArray<FLinearColor>  Colors;      Colors.Init(FLinearColor::White,   Verts.Num());
//     TArray<FProcMeshTangent> Tangs;    Tangs.Init(FProcMeshTangent(1,0,0),Verts.Num());
//
//     MergedActor->MeshComponent->CreateMeshSection_LinearColor(
//         0, Verts, Inds, Normals, UV0, Colors, Tangs, true
//     );
//
//     UE_LOG(LogTemp, Log, TEXT("Merged & welded meshes: %d verts, %d tris"),
//            Verts.Num(), Inds.Num()/3);
// }













// void SClothDesignCanvas::ApplySewingConstraints()
// {
// 	for (const FPatternSewingConstraint& Constraint : SewingConstraints)
// 	{
// 		if (!Constraint.MeshA || !Constraint.MeshB) continue;
//
// 		// Get world positions for each vertex
// 		FVector PosA = GetSkinnedVertexPosition(Constraint.MeshA, Constraint.VertexIndexA);
// 		FVector PosB = GetSkinnedVertexPosition(Constraint.MeshB, Constraint.VertexIndexB);
//
// 		FVector Midpoint = (PosA + PosB) * 0.5f;
//
// 		FVector NewPosA = FMath::Lerp(PosA, Midpoint, Constraint.Stiffness);
// 		FVector NewPosB = FMath::Lerp(PosB, Midpoint, Constraint.Stiffness);
//
// 		// You can't directly set vertex positions, so visualize the result instead:
// 		DrawDebugSphere(Constraint.MeshA->GetWorld(), NewPosA, 2.5f, 8, FColor::Green, false, 10.0f);
// 		DrawDebugSphere(Constraint.MeshB->GetWorld(), NewPosB, 2.5f, 8, FColor::Blue, false, 10.0f);
// 	}
// }



// FVector SClothDesignCanvas::GetSkinnedVertexPosition(USkeletalMeshComponent* MeshComp, int32 VertexIndex)
// {
// 	// if (!MeshComp || !MeshComp->SkeletalMesh) return FVector::ZeroVector;
// 	//
// 	// FSkeletalMeshRenderData* RenderData = MeshComp->GetSkeletalMeshRenderData();
// 	// if (!RenderData || RenderData->LODRenderData.Num() == 0) return FVector::ZeroVector;
//
// 	// // Use LOD 0
// 	// const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
// 	// const FPositionVertexBuffer& PositionBuffer = LODData.StaticVertexBuffers.PositionVertexBuffer;
// 	//
// 	// if (VertexIndex < 0 || VertexIndex >= PositionBuffer.GetNumVertices()) return FVector::ZeroVector;
// 	//
// 	// // Get local position, convert to world
// 	// FVector LocalPosition = PositionBuffer.VertexPosition(VertexIndex);
// 	// return MeshComp->GetComponentTransform().TransformPosition(LocalPosition);
// }









// third version
// void SClothDesignCanvas::TriangulateAndBuildMesh()
// {
//     if (Points.Num() < 3)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
//         return;
//     }
//
//     // 1) Build a flat-outline triangulation
//     TArray<PolygonTriangulation::TVector2<float>> Poly2d;
//     Poly2d.Reserve(Points.Num());
//     for (const FVector2D& P : Points)
//     {
//         Poly2d.Add(PolygonTriangulation::TVector2<float>(P.X, P.Y));
//     }
//
//     TArray<UE::Geometry::FIndex3i> InitialTris;
//     PolygonTriangulation::TriangulateSimplePolygon<float>(Poly2d, InitialTris, false);
//     if (InitialTris.Num() == 0)
//     {
//         UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
//         return;
//     }
//
//     // 2) Populate a DynamicMesh3 by appending verts & tris
//     UE::Geometry::FDynamicMesh3 Mesh;
//
// 	
// 	for (const auto& V2 : Poly2d)
// 	{
// 		Mesh.AppendVertex((double)V2.X, (double)V2.Y, 0.0);
// 	}
// 	for (const UE::Geometry::FIndex3i& T : InitialTris)
// 	{
// 		Mesh.AppendTriangle(T.A, T.B, T.C);
// 	}
//     // 3) (Optional) Subdivide for higher density
//     UE::Geometry::FDynamicMeshEditor Subdivider(&Mesh);
//     for (int Pass = 0; Pass < 2; ++Pass)
//     {
//         Subdivider.SubdivideEdges(
//             [](const UE::Geometry::FVector3d& A, const UE::Geometry::FVector3d& B)
//             {
//                 return A.Distance(B) > 0.1; // split edges longer than 0.1
//             }
//         );
//     }
//
//     // 4) Extract into Unreal arrays for procedural mesh
//     TArray<FVector> FinalVerts;  
//     FinalVerts.SetNum(Mesh.VertexCount());
//     for (int VID : Mesh.VertexIndicesItr())
//     {
//         auto P = Mesh.GetVertex(VID); // P is FVector3d
//         FinalVerts[VID] = FVector(P.X, P.Y, 0.0f);
//     }
//
//     TArray<int32> FinalTris;
//     FinalTris.Reserve(Mesh.TriangleCount() * 3);
//     for (int TID : Mesh.TriangleIndicesItr())
//     {
//         auto Tri = Mesh.GetTriangle(TID);
//         FinalTris.Add(Tri.A);
//         FinalTris.Add(Tri.B);
//         FinalTris.Add(Tri.C);
//     }
//
//     // 5) Spawn the procedural mesh
//     CreateProceduralMesh(FinalVerts, FinalTris);
// }


// first version working !!
// void SClothDesignCanvas::TriangulateAndBuildMesh()
// {
// 	if (Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
//
// 	// Step 1: Convert FVector2D 
// 	TArray<PolygonTriangulation::TVector2<float>> PolygonVerts;
// 	
// 	// Convert from your Points (FVector2D) to PolygonVerts
// 	for (const FVector2D& P : Points)
// 	{
// 		PolygonVerts.Add(PolygonTriangulation::TVector2<float>(P.X, P.Y));
// 	}
//
// 	// Step 2: Triangulate using GeometryProcessing
// 	TArray<UE::Geometry::FIndex3i> Triangles;
//
//
// 	PolygonTriangulation::TriangulateSimplePolygon<float>(PolygonVerts, Triangles, false);
//
// 	if (Triangles.Num() == 0)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
// 		return;
// 	}
// 	// Step 3: Convert results to Unreal-friendly format
// 	TArray<FVector> Vertices;
// 	TArray<int32> Indices;
//
// 	for (const PolygonTriangulation::TVector2<float>& V : PolygonVerts)
// 	{
// 		Vertices.Add(FVector(V.X, V.Y, 0.f));  // Z=0 since it’s flat
// 	}
//
// 	
// 	for (const UE::Geometry::FIndex3i& Tri : Triangles)
// 	{
// 		Indices.Add(Tri.C);
// 		Indices.Add(Tri.B);
// 		Indices.Add(Tri.A);
// 	}
//
// 	// Step 4: Spawn procedural mesh (next step)
// 	CreateProceduralMesh(Vertices, Indices);
// }

// i think second verion but not sure if its working
// void SClothDesignCanvas::TriangulateAndBuildMesh()
// {
// 	// UE::Geometry::TGeneralPolygon2<float> Polygon;
// 	if (Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
// 	
//     // 1) Convert FVector2D points into the float‐precision polygon
// 	// TArray<FVector2d> Poly;
// 	// for (auto& P : Points) Poly.Add(FVector2d(P.X, P.Y));
// 	// Step 1: Convert FVector2D 
// 	TArray<PolygonTriangulation::TVector2<float>> Poly2d;
// 	
// 	// Convert from Points (FVector2D) to PolygonVerts
// 	for (const FVector2D& P : Points)
// 	{
// 		Poly2d.Add(PolygonTriangulation::TVector2<float>(P.X, P.Y));
// 	}
//
// 	// 2) Coarse triangulation of that outline
//
// 	TArray<UE::Geometry::FIndex3i> InitialTris;
// 	
// 	PolygonTriangulation::TriangulateSimplePolygon<float>(Poly2d, InitialTris, false);
// 	if (InitialTris.Num() == 0)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
// 		return;
// 	}
// 	
// }
