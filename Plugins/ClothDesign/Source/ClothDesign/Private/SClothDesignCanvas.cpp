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

// FReply SClothDesignCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
// {
// 	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
// 	{
// 		const FVector2D LocalClickPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
// 		Points.Add(LocalClickPos);
// 		return FReply::Handled();
// 	}
// 	return FReply::Unhandled();
// }



FVector2D SClothDesignCanvas::TransformPoint(const FVector2D& Point) const
{
	return (Point * ZoomFactor) + PanOffset;
}

FVector2D SClothDesignCanvas::InverseTransformPoint(const FVector2D& ScreenPoint) const
{
	return (ScreenPoint - PanOffset) / ZoomFactor;
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
	const FLinearColor GridColor(0.1f, 0.1f, 0.1f, 0.4f);
	
	// --- Draw Grid ---
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	// const float NonScaledGridSpacing = 100.f; // spacing in pixels

	// const float GridSpacing = NonScaledGridSpacing* ZoomFactor; // spacing in pixels

	const FVector2D TopLeftScreen     = FVector2D(0, 0);
	const FVector2D BottomRightScreen = Size;

	// Convert those to “world” (canvas) coordinates
	FVector2D WorldTopLeft     = InverseTransformPoint(TopLeftScreen);
	FVector2D WorldBottomRight = InverseTransformPoint(BottomRightScreen);
	const float WorldGridSpacing = 100.f;  // e.g. every 100 “canvas” units

	
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
			ESlateDrawEffect::None, GridColor, true, 1.0f
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
			ESlateDrawEffect::None, GridColor, true, 1.0f
		);
	}

	// --- Advance Layer for shapes ---
	LayerId++;

	
	 // Compute starting points for vertical lines
	// float StartX = FMath::Fmod(-PanOffset.X * ZoomFactor, GridSpacing);
	// if (StartX < 0) StartX += GridSpacing;
	// for (float x = -StartX; x < Size.X; x += GridSpacing)
	// for (float x = FMath::Fmod(-PanOffset.X, GridSpacing); x < Size.X; x += GridSpacing)

	// // Anchor to world origin, transformed
	// FVector2D AnchorScreen = TransformPoint(FVector2D(0, 0));
	//
	// // Compute offset from anchor to screen
	// float StartX = FMath::Fmod(AnchorScreen.X, GridSpacing);
	// if (StartX < 0) StartX += GridSpacing;
	
	// for (float x = -StartX; x < Size.X; x += GridSpacing)
	// {	
	// 	FSlateDrawElement::MakeLines(
	// 		OutDrawElements,
	// 		LayerId,
	// 		AllottedGeometry.ToPaintGeometry(),
	// 		{ FVector2D(x, 0), FVector2D(x, Size.Y) },
	// 		ESlateDrawEffect::None,
	// 		GridColor,
	// 		true,
	// 		1.0f
	// 	);
	// }

	
	// // Same for horizontal lines:
	// float StartY = FMath::Fmod(-PanOffset.Y * ZoomFactor, GridSpacing);
	// if (StartY < 0) StartY += GridSpacing;
	// for (float y = -StartY; y < Size.Y; y += GridSpacing)
	// for (float y = FMath::Fmod(-PanOffset.Y, GridSpacing); y < Size.Y; y += GridSpacing)
	
	// float StartY = FMath::Fmod(AnchorScreen.Y, GridSpacing);
	// if (StartY < 0) StartY += GridSpacing;

	// for (float y = -StartY; y < Size.Y; y += GridSpacing)
	// {
	// 	FSlateDrawElement::MakeLines(
	// 		OutDrawElements,
	// 		LayerId,
	// 		AllottedGeometry.ToPaintGeometry(),
	// 		{ FVector2D(0, y), FVector2D(Size.X, y) },
	// 		ESlateDrawEffect::None,
	// 		GridColor,
	// 		true,
	// 		1.0f
	// 	);
	// }

	
	
	// // Draw lines between points
	// for (int32 i = 0; i < Points.Num() - 1; ++i)
	// {
	// 	FSlateDrawElement::MakeLines(
	// 		OutDrawElements,
	// 		LayerId,
	// 		AllottedGeometry.ToPaintGeometry(),
	// 	{ TransformPoint(Points[i]), TransformPoint(Points[i + 1]) },
	// 		ESlateDrawEffect::None,
	// 		FLinearColor::Gray,
	// 		true,
	// 		2.0f
	// 	);
	// }
	//
	// // Draw closing line if shape is closed
	// if (Points.Num() > 2)
	// {
	// 	FSlateDrawElement::MakeLines(
	// 		OutDrawElements,
	// 		LayerId,
	// 		AllottedGeometry.ToPaintGeometry(),
	// 		{ TransformPoint(Points.Last()), TransformPoint(Points[0]) },
	// 		ESlateDrawEffect::None,
	// 		FLinearColor::Gray,
	// 		true,
	// 		2.0f
	// 	);
	// }



	//
	// // 3. Advance layer so points draw on top of lines
	// LayerId++;

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
	//

	// if (CurvePoints.Points.Num() >= 2)
	// {
	// 	TArray<FVector2D> CurveSamples;
	//
	// 	// Sample curve between 0 and max key (e.g., 0 to last point's input)
	// 	// float StartKey = CurvePoints.Points[0].InVal;
	// 	// float EndKey = CurvePoints.Points.Last().InVal;
	// 	float StartKey = 0.f;
	// 	float EndKey = CurvePoints.Points.Num() - 1;
	// 	
	// 	const int NumSamples = 50; // How smooth the curve is
	// 	for (int i = 0; i <= NumSamples; i++)
	// 	{
	// 		float Alpha = FMath::Lerp(StartKey, EndKey, float(i) / NumSamples);
	// 		FVector2D PointOnCurve = CurvePoints.Eval(Alpha);
 //        
	// 		// TransformPoint is your function that converts from canvas to screen coords
	// 		FVector2D ScreenPoint = TransformPoint(PointOnCurve);
	// 		CurveSamples.Add(ScreenPoint);
	// 	}
	// 	
	// 	// Draw lines between the sampled points to render the curve
	// 	for (int i = 0; i < CurveSamples.Num() - 1; i++)
	// 	{
	// 		FSlateDrawElement::MakeLines(
	// 			OutDrawElements, LayerId,
	// 			AllottedGeometry.ToPaintGeometry(),
	// 			{ CurveSamples[i], CurveSamples[i + 1] },
	// 			ESlateDrawEffect::None,
	// 			FLinearColor::Green, // or your color
	// 			true, 2.0f // anti-aliased, thickness
	// 		);
	// 	}
	//
	// 	LayerId++;
	// }

	// Draw completed shapes first
	for (const FInterpCurve<FVector2D>& Shape : CompletedShapes)
	{
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
						FLinearColor::Blue,
						true, 2.0f
					);
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
	}
	
	// Draw interactive points and bezier handles for all completed shapes
	for (const FInterpCurve<FVector2D>& Shape : CompletedShapes)
	{
		for (int32 i = 0; i < Shape.Points.Num(); ++i)
		{
			const auto& Pt = Shape.Points[i];
			FVector2D DrawPos = TransformPoint(Pt.OutVal);
			FVector2D ArriveHandle = TransformPoint(Pt.OutVal - Pt.ArriveTangent);
			FVector2D LeaveHandle  = TransformPoint(Pt.OutVal + Pt.LeaveTangent);

			FLinearColor BoxColor = FLinearColor::Gray; // Or color by selection if implemented

			// Draw point box
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(DrawPos - FVector2D(3, 3), FVector2D(6, 6)),
				FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None,
				BoxColor
			);

			// Draw handle lines
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				{ DrawPos, ArriveHandle }, ESlateDrawEffect::None, FLinearColor::Gray, true, 1.0f);

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				{ DrawPos, LeaveHandle }, ESlateDrawEffect::None, FLinearColor::Gray, true, 1.0f);

			// Draw handle boxes
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(ArriveHandle - FVector2D(3, 3), FVector2D(6, 6)),
				FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, FLinearColor::Yellow);

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(LeaveHandle - FVector2D(3, 3), FVector2D(6, 6)),
				FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, FLinearColor::Yellow);
		}
	}

	
	// curve points
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
					FLinearColor::Blue,
					true, 2.0f
				);
			}
		}

		++LayerId;
	}

	// close shape with straight line
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
	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
	{
		FVector2D DrawPos = TransformPoint(CurvePoints.Points[i].OutVal);
		FLinearColor Color = (i == SelectedPointIndex) ? FLinearColor::Yellow : FLinearColor::White;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(DrawPos - FVector2D(3, 3), FVector2D(6, 6)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			Color
		);
	}

	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
	{
		const auto& Pt = CurvePoints.Points[i];
		const FVector2D Pos = TransformPoint(Pt.OutVal);
		const FVector2D ArriveHandle = TransformPoint(Pt.OutVal - Pt.ArriveTangent);
		const FVector2D LeaveHandle  = TransformPoint(Pt.OutVal + Pt.LeaveTangent);

		// Draw lines from point to each handle
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			{ Pos, ArriveHandle }, ESlateDrawEffect::None, FLinearColor::Blue, true, 1.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			{ Pos, LeaveHandle }, ESlateDrawEffect::None, FLinearColor::Blue, true, 1.0f);

		// Draw the handles as small draggable boxes
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(ArriveHandle - FVector2D(3, 3), FVector2D(6, 6)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None, FLinearColor::Yellow);

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(LeaveHandle - FVector2D(3, 3), FVector2D(6, 6)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None, FLinearColor::Yellow);
	}
	
	return LayerId;
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

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D LocalClickPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D CanvasClickPos = InverseTransformPoint(LocalClickPos);

		// // Draw
		// if (CurrentMode == EClothEditorMode::Draw)
		// {
		// 	SaveStateForUndo();
		// 	// Points.Add(CanvasClickPos);
		// 	// float Key = CanvasClickPos.X; // or use an incrementing ID if you want to decouple from X
		// 	// CurvePoints.Points.Add(FInterpCurvePoint<FVector2D>(Key, CanvasClickPos));
		// 	// CurvePoints.Points.Sort([](const FInterpCurvePoint<FVector2D>& A, const FInterpCurvePoint<FVector2D>& B) {
		// 	// 	return A.InVal < B.InVal;
		// 	// });
		// 	FInterpCurvePoint<FVector2D> NewPoint;
		// 	NewPoint.InVal = CurvePoints.Points.Num(); // Or use a running float like TotalDistance or cumulative X
		// 	NewPoint.OutVal = CanvasClickPos; // Clicked canvas-space position
		// 	NewPoint.InterpMode = CIM_CurveAuto; // or auto like before,  Or CIM_Linear if you want sharp lines
		// 	// for (auto& Point : CurvePoints.Points)
		// 	// {
		// 	// 	Point.InterpMode = CIM_CurveAuto;
		// 	// }
		//
		//
		// 	CurvePoints.Points.Add(NewPoint);
		// 	CurvePoints.AutoSetTangents();
		//
		// 	
		// 	UE_LOG(LogTemp, Warning, TEXT("Draw mode: Added point at (%f, %f)"), CanvasClickPos.X, CanvasClickPos.Y);
		// 	return FReply::Handled();
		// }




		
		// /*
		// When the user hits Enter, push CurvePoints to CompletedShapes and clear it to start a new one.
		// In rendering and triangulation, loop over both CompletedShapes and CurvePoints.
		//  */
		
		if (CurrentMode == EClothEditorMode::Draw)
		{
			SaveStateForUndo();

			FInterpCurvePoint<FVector2D> NewPoint;
			NewPoint.InVal = CurvePoints.Points.Num();
			NewPoint.OutVal = CanvasClickPos;
			NewPoint.InterpMode = CIM_CurveAuto;

			CurvePoints.Points.Add(NewPoint);
			CurvePoints.AutoSetTangents();

			UE_LOG(LogTemp, Warning, TEXT("Draw mode: Added point at (%f, %f)"), CanvasClickPos.X, CanvasClickPos.Y);
			return FReply::Handled();
		}



		// SINGLE SHAPE
		// // Select and Move
		// else if (CurrentMode == EClothEditorMode::Select) // || CurrentMode == EClothEditorMode::Move)
		// {
		// 	FVector2D LocalClick = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		// 	FVector2D CanvasClick = InverseTransformPoint(LocalClick);
		//
		// 	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
		// 	{
		// 		const auto& Pt = CurvePoints.Points[i];
		// 		const FVector2D PointPos = Pt.OutVal;
		//
		// 		FVector2D Arrive = PointPos - Pt.ArriveTangent;
		// 		FVector2D Leave  = PointPos + Pt.LeaveTangent;
		//
		// 		if (FVector2D::Distance(CanvasClick, Arrive) < TangentHandleRadius / ZoomFactor)
		// 		{
		// 			SaveStateForUndo();
		// 			SelectedPointIndex = i;
		// 			SelectedTangentHandle = ETangentHandle::Arrive;
		// 			bIsDraggingTangent = true;
		// 			UE_LOG(LogTemp, Warning, TEXT("Selected arrive handle for point %d"), i);
		//
		// 			return FReply::Handled().CaptureMouse(SharedThis(this));
		// 		}
		// 		else if (FVector2D::Distance(CanvasClick, Leave) < TangentHandleRadius / ZoomFactor)
		// 		{
		// 			SaveStateForUndo();
		// 			SelectedPointIndex = i;
		// 			SelectedTangentHandle = ETangentHandle::Leave;
		// 			bIsDraggingTangent = true;
		// 			UE_LOG(LogTemp, Warning, TEXT("Selected leave handle for point %d"), i);
		//
		// 			return FReply::Handled().CaptureMouse(SharedThis(this));
		// 		}
		// 	}
		//
		//
		// 	
		// 	const float SelectionRadius = 10.0f;
		// 	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
		// 	{
		// 		FVector2D WorldPoint = CurvePoints.Points[i].OutVal;
		// 		if (FVector2D::Distance(WorldPoint, CanvasClickPos) < SelectionRadius / ZoomFactor)
		// 		{
		// 			SaveStateForUndo();
		// 			SelectedPointIndex = i;
		// 			bIsShapeSelected = false;
		// 			bIsDraggingPoint = true;
		//
		// 			UE_LOG(LogTemp, Warning, TEXT("Selected point %d"), i);
		//
		// 			return FReply::Handled()
		// 				.CaptureMouse(SharedThis(this))
		// 				.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
		// 		}
		// 	}


		// Check completed shapes first
		for (int32 ShapeIndex = 0; ShapeIndex < CompletedShapes.Num(); ++ShapeIndex)
		{
			const auto& Shape = CompletedShapes[ShapeIndex];

			for (int32 i = 0; i < Shape.Points.Num(); ++i)
			{
				const auto& Pt = Shape.Points[i];
				const FVector2D PointPos = Pt.OutVal;
				FVector2D Arrive = PointPos - Pt.ArriveTangent;
				FVector2D Leave = PointPos + Pt.LeaveTangent;

				if (FVector2D::Distance(CanvasClickPos, Arrive) < TangentHandleRadius / ZoomFactor)
				{
					SaveStateForUndo();
					SelectedShapeIndex = ShapeIndex;
					SelectedPointIndex = i;
					SelectedTangentHandle = ETangentHandle::Arrive;
					bIsDraggingTangent = true;

					return FReply::Handled().CaptureMouse(SharedThis(this));
				}
				else if (FVector2D::Distance(CanvasClickPos, Leave) < TangentHandleRadius / ZoomFactor)
				{
					SaveStateForUndo();
					SelectedShapeIndex = ShapeIndex;
					SelectedPointIndex = i;
					SelectedTangentHandle = ETangentHandle::Leave;
					bIsDraggingTangent = true;

					return FReply::Handled().CaptureMouse(SharedThis(this));
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

			if (FVector2D::Distance(CanvasClickPos, Arrive) < TangentHandleRadius / ZoomFactor)
			{
				SaveStateForUndo();
				SelectedShapeIndex = INDEX_NONE; // current shape
				SelectedPointIndex = i;
				SelectedTangentHandle = ETangentHandle::Arrive;
				bIsDraggingTangent = true;

				return FReply::Handled().CaptureMouse(SharedThis(this));
			}
			else if (FVector2D::Distance(CanvasClickPos, Leave) < TangentHandleRadius / ZoomFactor)
			{
				SaveStateForUndo();
				SelectedShapeIndex = INDEX_NONE;
				SelectedPointIndex = i;
				SelectedTangentHandle = ETangentHandle::Leave;
				bIsDraggingTangent = true;

				return FReply::Handled().CaptureMouse(SharedThis(this));
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

		
		if (Points.Num() < 2)
			return FReply::Handled(); // not enough points

			// // No point selected, check if clicked near a line
			// const float LineSelectThreshold = 10.f / ZoomFactor;
			// for (int32 i = 0; i < Points.Num() - 1; ++i)
			// {
			// 	const FVector2D A = Points[i];
			// 	const FVector2D B = Points[(i + 1) % Points.Num()]; // Loop around if closed
			//
			// 	if (IsPointNearLine(CanvasClickPos, A, B, LineSelectThreshold))
			// 	{
			// 		UE_LOG(LogTemp, Warning, TEXT("ZoomFactor: %f"), ZoomFactor);
			//
			// 		SaveStateForUndo();
			// 		bIsShapeSelected = true;
			// 		SelectedPointIndex = INDEX_NONE;
			// 		bIsDraggingShape = true;
			//
			// 		UE_LOG(LogTemp, Warning, TEXT("Selected line near segment %d"), i);
			// 		return FReply::Handled().CaptureMouse(SharedThis(this));
			// 	}
			// }



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
//
// FReply SClothDesignCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
// {
// 	if ((CurrentMode == EClothEditorMode::Move || CurrentMode == EClothEditorMode::Select) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
// 	{
// 		FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
// 		const FVector2D CanvasMousePos = InverseTransformPoint(LocalMousePos);
// 		UE_LOG(LogTemp, Warning, TEXT("CanvasClick: %s"), *CanvasMousePos.ToString());
//
// 		// for curve handles
// 		if (bIsDraggingTangent && SelectedPointIndex != INDEX_NONE)
// 		{
//
// 			FVector2D PointPos = CurvePoints.Points[SelectedPointIndex].OutVal;
// 			FVector2D Delta = CanvasMousePos - PointPos;
//
// 			if (SelectedTangentHandle == ETangentHandle::Arrive)
// 			{
// 				UE_LOG(LogTemp, Warning, TEXT("Click on Arrive handle of point"));
// 				CurvePoints.Points[SelectedPointIndex].ArriveTangent = -Delta;
// 				//CurvePoints.Points[SelectedPointIndex].LeaveTangent = Delta;
//
// 			}
// 			else if (SelectedTangentHandle == ETangentHandle::Leave)
// 			{
// 				UE_LOG(LogTemp, Warning, TEXT("Click on leave handle of point"));
// 				CurvePoints.Points[SelectedPointIndex].LeaveTangent = Delta;
// 				//CurvePoints.Points[SelectedPointIndex].ArriveTangent = -Delta;
// 			}
// 			UE_LOG(LogTemp, Warning, TEXT("Dragging tangent for point %d"), SelectedPointIndex);
//
// 			return FReply::Handled();
// 		}
// 		
// 		if (bIsDraggingPoint && SelectedPointIndex != INDEX_NONE)
// 		{
// 			
// 			// Points[SelectedPointIndex] = InverseTransformPoint(LocalPos);
// 			CurvePoints.Points[SelectedPointIndex].OutVal = CanvasMousePos;
// 			UE_LOG(LogTemp, Warning, TEXT("Dragging for point %d"), SelectedPointIndex);
//
// 			return FReply::Handled();
// 		}
// 	}
// 	
//
// 	return FReply::Unhandled();
// }

FReply SClothDesignCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ((CurrentMode == EClothEditorMode::Move || CurrentMode == EClothEditorMode::Select) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D CanvasMousePos = InverseTransformPoint(LocalMousePos);
		UE_LOG(LogTemp, Warning, TEXT("CanvasClick: %s"), *CanvasMousePos.ToString());

		if (bIsDraggingTangent && SelectedPointIndex != INDEX_NONE)
		{
			// Decide which shape's points to update
			if (SelectedShapeIndex == INDEX_NONE)
			{
				// Current shape
				FVector2D PointPos = CurvePoints.Points[SelectedPointIndex].OutVal;
				FVector2D Delta = CanvasMousePos - PointPos;

				if (SelectedTangentHandle == ETangentHandle::Arrive)
				{
					CurvePoints.Points[SelectedPointIndex].ArriveTangent = -Delta;
				}
				else if (SelectedTangentHandle == ETangentHandle::Leave)
				{
					CurvePoints.Points[SelectedPointIndex].LeaveTangent = Delta;
				}
			}
			else
			{
				// Completed shape
				FInterpCurvePoint<FVector2D>& Pt = CompletedShapes[SelectedShapeIndex].Points[SelectedPointIndex];
				FVector2D PointPos = Pt.OutVal;
				FVector2D Delta = CanvasMousePos - PointPos;

				if (SelectedTangentHandle == ETangentHandle::Arrive)
				{
					Pt.ArriveTangent = -Delta;
				}
				else if (SelectedTangentHandle == ETangentHandle::Leave)
				{
					Pt.LeaveTangent = Delta;
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("Dragging tangent for point %d in shape %d"), SelectedPointIndex, SelectedShapeIndex);
			return FReply::Handled();
		}

		if (bIsDraggingPoint && SelectedPointIndex != INDEX_NONE)
		{
			if (SelectedShapeIndex == INDEX_NONE)
			{
				CurvePoints.Points[SelectedPointIndex].OutVal = CanvasMousePos;
			}
			else
			{
				CompletedShapes[SelectedShapeIndex].Points[SelectedPointIndex].OutVal = CanvasMousePos;
			}

			UE_LOG(LogTemp, Warning, TEXT("Dragging point %d in shape %d"), SelectedPointIndex, SelectedShapeIndex);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}


FReply SClothDesignCanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
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

		// if (SelectedPointIndex != INDEX_NONE && SelectedPointIndex < CurvePoints.Points.Num())
		// {
		// 	UE_LOG(LogTemp, Warning, TEXT("Trying to delete point: %d"), SelectedPointIndex);
		// 	SaveStateForUndo();
		// 	CurvePoints.Points.RemoveAt(SelectedPointIndex);
		// 	CurvePoints.AutoSetTangents(); // Rebuild curve after deleting
		//
		// 	SelectedPointIndex = INDEX_NONE;
		// 	Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
		//
		// 	return FReply::Handled();
		// }
		if (SelectedPointIndex != INDEX_NONE)
		{
			SaveStateForUndo();

			if (SelectedShapeIndex == INDEX_NONE)
			{
				// Deleting from current shape
				if (SelectedPointIndex < CurvePoints.Points.Num())
				{
					CurvePoints.Points.RemoveAt(SelectedPointIndex);
					CurvePoints.AutoSetTangents();
				}
			}
			else if (SelectedShapeIndex >= 0 && SelectedShapeIndex < CompletedShapes.Num())
			{
				// Deleting from completed shape
				if (SelectedPointIndex < CompletedShapes[SelectedShapeIndex].Points.Num())
				{
					CompletedShapes[SelectedShapeIndex].Points.RemoveAt(SelectedPointIndex);
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
	
	if (CurrentMode == EClothEditorMode::Draw)
	{
		if (Key == EKeys::Enter)
		{
			if (CurvePoints.Points.Num() > 0)
			{
				SaveStateForUndo();
				CompletedShapes.Add(CurvePoints);
				CurvePoints.Points.Empty();
				UE_LOG(LogTemp, Warning, TEXT("Shape finalized. Ready to start a new one."));
			}
			return FReply::Handled();
		}
	}

	
	//return FReply::Unhandled();
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);

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





FReply SClothDesignCanvas::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("Canvas received keyboard focus"));
	return FReply::Handled();
}


// void SClothDesignCanvas::TriangulateAndBuildMesh()
// {
// 	if (CurvePoints.Points.Num() < 3)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
// 		return;
// 	}
void SClothDesignCanvas::TriangulateAndBuildMesh(const FInterpCurve<FVector2D>& Shape)
{
	if (Shape.Points.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
		return;
	}

	TArray<FVector2f> PolyVerts;
	const int SamplesPerSegment = 10;

	for (int SegIndex = 0; SegIndex < Shape.Points.Num() - 1; ++SegIndex)
	{
		float StartInVal = Shape.Points[SegIndex].InVal;
		float EndInVal   = Shape.Points[SegIndex + 1].InVal;

		for (int i = 0; i < SamplesPerSegment; ++i)
		{
			float Alpha = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
			FVector2D Sample = Shape.Eval(Alpha);
			PolyVerts.Add(FVector2f(Sample.X, Sample.Y));
		}
	}
	// ...rest of triangulation and mesh creation code...

	// // Step 1: Sample the curve into straight segments
	// TArray<FVector2f> PolyVerts;
	//
	// const int SamplesPerSegment = 10;
	//
	// for (int SegIndex = 0; SegIndex < CurvePoints.Points.Num() - 1; ++SegIndex)
	// {
	// 	float StartInVal = CurvePoints.Points[SegIndex].InVal;
	// 	float EndInVal   = CurvePoints.Points[SegIndex + 1].InVal;
	//
	// 	for (int i = 0; i < SamplesPerSegment; ++i)
	// 	{
	// 		float Alpha = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
	// 		FVector2D Sample = CurvePoints.Eval(Alpha);
	// 		PolyVerts.Add(FVector2f(Sample.X, Sample.Y));
	// 	}
	// }
	//
	//
	//
	// if (PolyVerts.Num() < 3)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Not enough curve samples to triangulate"));
	// 	return;
	// }
	
	// Step 2: Triangulate
	TArray<UE::Geometry::FIndex3i> Triangles;
	PolygonTriangulation::TriangulateSimplePolygon<float>(PolyVerts, Triangles, false);

	if (Triangles.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
		return;
	}

	// Step 3: Build DynamicMesh
	UE::Geometry::FDynamicMesh3 Mesh;

	// WORKING but not smooth at all so lots of different sized triangles
	// Insert vertices
	TArray<int> VertexIDs;
	for (const FVector2f& V : PolyVerts)
	{
		int VID = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0)); // z = 0
		VertexIDs.Add(VID);
	}
	
	// Insert triangles
	for (const UE::Geometry::FIndex3i& Tri : Triangles)
	{
		Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
	}
	
	// 2. Remesher setup
	UE::Geometry::FRemesher Remesher(&Mesh);
	Remesher.SetTargetEdgeLength(1.50); // Set this to desired density
	
	Remesher.SmoothSpeedT = 0.0f; // Optional tweak
	
	for (int i = 0; i < 2; ++i)  // 1 not enough, 3-5 too much in terms of edge vertices changing
	{
		// Remesher.BasicRemeshPass();
	}


	// // (Optional) Subdivide for more detail
	// // UE::Geometry::FDynamicMeshEditor Editor(&Mesh);
	
	int32 NumSubdivisions = 3;
	double MaxEdgeLength = 1.50;
	
	for (int32 Pass = 0; Pass < NumSubdivisions; ++Pass)
	{
		TArray<int32> EdgesToSplit;
		for (int32 eid : Mesh.EdgeIndicesItr())
		{
			UE::Geometry::FIndex2i EdgeVerts = Mesh.GetEdgeV(eid);
			FVector3d A = Mesh.GetVertex(EdgeVerts.A);
			FVector3d B = Mesh.GetVertex(EdgeVerts.B);
	
			if (FVector3d::Distance(A, B) > MaxEdgeLength)
			{
				EdgesToSplit.Add(eid);
			}
		}
	
		for (int32 eid : EdgesToSplit)
		{
			UE::Geometry::FDynamicMesh3::FEdgeSplitInfo SplitInfo;
			Mesh.SplitEdge(eid, SplitInfo);
		}
	}
	
	
	// Step 4: Extract to UE arrays
	TArray<FVector> Vertices;
	TArray<int32> Indices;

	for (int vid : Mesh.VertexIndicesItr())
	{
		FVector3d Pos = Mesh.GetVertex(vid);
		Vertices.Add(FVector(Pos.X, Pos.Y, Pos.Z));
	}

	for (int tid : Mesh.TriangleIndicesItr())
	{
		UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
		Indices.Add(Tri.C);
		Indices.Add(Tri.B);
		Indices.Add(Tri.A);
	}

	// Step 5: Build procedural mesh
	CreateProceduralMesh(Vertices, Indices);
}



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


	FString FolderName = TEXT("GeneratedClothActors");
	MeshActor->SetFolderPath(FName(*FolderName));
	
	// ✅ Set a visible name in World Outliner
#if WITH_EDITOR
	MeshActor->SetActorLabel(UniqueLabel);
#endif
	
	// AClothPatternMeshActor* MeshActor = World->SpawnActor<AClothPatternMeshActor>();
	// if (!MeshActor) return;

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

	State.SelectedPointIndex = SelectedPointIndex;
	State.PanOffset = PanOffset;
	State.ZoomFactor = ZoomFactor;
	return State;
}

// void SClothDesignCanvas::RestoreCanvasState(const FCanvasState& State)
// {
// 	Points = State.Points;
// 	SelectedPointIndex = State.SelectedPointIndex;
// 	PanOffset = State.PanOffset;
// 	ZoomFactor = State.ZoomFactor;
//
// 	Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
// }
void SClothDesignCanvas::RestoreCanvasState(const FCanvasState& State)
{
	CurvePoints = State.CurvePoints;
	CompletedShapes = State.CompletedShapes;

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
