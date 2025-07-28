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


//void SClothDesignCanvas::TriangulateAndBuildMesh(const FInterpCurve<FVector2D>& Shape)


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
	UE::Geometry::FDynamicMesh3 Mesh;
	// WORKING but not smooth at all so lots of different sized triangles
	// Insert vertices
	// TArray<int> VertexIDs;
	//
	// LastSeamVertexIDs.Empty();
	// int TotalSamples = PolyVerts.Num();





	// for (int SegIndex = 0; SegIndex < Shape.Points.Num() - 1; ++SegIndex)
	// {
	// 	float StartInVal = Shape.Points[SegIndex].InVal;
	// 	float EndInVal   = Shape.Points[SegIndex + 1].InVal;
	//
	// 	for (int i = 0; i < SamplesPerSegment; ++i)
	// 	{
	// 		float Alpha = FMath::Lerp(StartInVal, EndInVal, float(i) / SamplesPerSegment);
	// 		FVector2D Sample = Shape.Eval(Alpha);
	// 		PolyVerts.Add(FVector2f(Sample.X, Sample.Y));
	// 	}
	// }
	

	
	LastSeamVertexIDs.Empty();
	TArray<int32> VertexIDs;

	int TotalSamples = (Shape.Points.Num()-1) * SamplesPerSegment;
	int SampleCounter = 0;

	for (int Seg=0; Seg<Shape.Points.Num()-1; ++Seg)
	{
		float In0 = Shape.Points[Seg].InVal;
		float In1 = Shape.Points[Seg+1].InVal;

		for (int i=0; i<SamplesPerSegment; ++i, ++SampleCounter)
		{
			float tSeg = float(i) / SamplesPerSegment;
			FVector2D P2 = Shape.Eval(FMath::Lerp(In0, In1, tSeg));
			PolyVerts.Add(FVector2f(P2.X, P2.Y));

			// store in mesh
			int VID = Mesh.AppendVertex(FVector3d(P2.X, P2.Y, 0));
			VertexIDs.Add(VID);

			// record seam if in range
			if (bRecordSeam)
			{
				float tCurve = float(SampleCounter) / float(TotalSamples-1);
				float tMin = Shape.Points[StartPointIdx2D].InVal / Shape.Points.Last().InVal;
				float tMax = Shape.Points[EndPointIdx2D].InVal   / Shape.Points.Last().InVal;
				if (tCurve >= tMin && tCurve <= tMax)
				{
					LastSeamVertexIDs.Add(VID);
				}
			}
		}
	}
	
	
	// Step 2: Triangulate
	TArray<UE::Geometry::FIndex3i> Triangles;
	PolygonTriangulation::TriangulateSimplePolygon<float>(PolyVerts, Triangles, false);



	if (Triangles.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
		return;
	}


	for (const FVector2f& V : PolyVerts)
	{
		int VID = Mesh.AppendVertex(FVector3d(V.X, V.Y, 0)); // z = 0
		VertexIDs.Add(VID);
	}
	// Assume PolyVerts is TArray<FVector2f> of your samples in order
	float SignedArea = 0.f;
	int N = PolyVerts.Num();
	for (int i = 0; i < N; ++i)
	{
		const FVector2f& A = PolyVerts[i];
		const FVector2f& B = PolyVerts[(i+1) % N];
		SignedArea += (A.X * B.Y - B.X * A.Y);
	}
	SignedArea *= 0.5f;
	
	// for (int idx = 0; idx < TotalSamples; ++idx)
	// {
	// 	FVector2f V2 = PolyVerts[idx];
	// 	int VID = Mesh.AppendVertex(FVector3d(V2.X, V2.Y, 0));
	// 	VertexIDs.Add(VID);
	//
	// 	if (bRecordSeam)
	// 	{
	// 		// Decide whether idx falls into your seam index range
	// 		// (you can map StartPointIdx2D→EndPointIdx2D into sample indices)
	// 		if (idx >= SeamSampleStart && idx <= SeamSampleEnd)
	// 		{
	// 			LastSeamVertexIDs.Add(VID);
	// 		}
	// 	}
	// }





	
	// Insert triangles
	for (const UE::Geometry::FIndex3i& Tri : Triangles)
	{
		Mesh.AppendTriangle(VertexIDs[Tri.A], VertexIDs[Tri.B], VertexIDs[Tri.C]);
	}
	
	
	// Step 4: Extract to UE arrays
	TArray<FVector> Vertices;
	TArray<int32> Indices;

	for (int vid : Mesh.VertexIndicesItr())
	{
		FVector3d Pos = Mesh.GetVertex(vid);
		Vertices.Add(FVector(Pos.X, Pos.Y, Pos.Z));
	}
	
	bool bReverseWinding = (SignedArea < 0.f);
	// for (int tid : Mesh.TriangleIndicesItr())
	// {
	// 	UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
	// 	Indices.Add(Tri.A);
	// 	Indices.Add(Tri.B);
	// 	Indices.Add(Tri.C);
	// }

	for (int tid : Mesh.TriangleIndicesItr())
	{
		UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(tid);
		if (bReverseWinding)
		{
			// flip each triangle
			Indices.Add(Tri.A);
			Indices.Add(Tri.B);
			Indices.Add(Tri.C);
		}
		else
		{
			// keep normal winding, here c to a is noremal winding
			Indices.Add(Tri.C);
			Indices.Add(Tri.B);
			Indices.Add(Tri.A);
		}
	}
	
	LastBuiltMesh           = MoveTemp(Mesh);
	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);

	// Step 5: Build procedural mesh
	CreateProceduralMesh(Vertices, Indices);
}


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
    AClothPatternMeshActor* ActorA = SpawnedPatternActors[SpawnedPatternActors.Num() - 2].Get();
    AClothPatternMeshActor* ActorB = SpawnedPatternActors[SpawnedPatternActors.Num() - 1].Get();
    if (!ActorA || !ActorB)
    {
        UE_LOG(LogTemp, Warning, TEXT("One of the two actors is invalid"));
        return;
    }

    // 2) Copy their dynamic meshes
    UE::Geometry::FDynamicMesh3 MergedMesh = ActorA->DynamicMesh;
    int32 BaseVID = MergedMesh.VertexCount();

    // Append B’s vertices
    for (int32 VID : ActorB->DynamicMesh.VertexIndicesItr())
    {
        FVector3d P = ActorB->DynamicMesh.GetVertex(VID);
        MergedMesh.AppendVertex(P);
    }

    // Append B’s triangles (offset indices by BaseVID)
    for (int32 TID : ActorB->DynamicMesh.TriangleIndicesItr())
    {
        UE::Geometry::FIndex3i Tri = ActorB->DynamicMesh.GetTriangle(TID);
        MergedMesh.AppendTriangle(
            Tri.A + BaseVID,
            Tri.B + BaseVID,
            Tri.C + BaseVID
        );
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
    for (int vid : B->DynamicMesh.VertexIndicesItr())
    {
        Merged.AppendVertex(B->DynamicMesh.GetVertex(vid));
    }
    for (int tid : B->DynamicMesh.TriangleIndicesItr())
    {
        auto T = B->DynamicMesh.GetTriangle(tid);
        Merged.AppendTriangle(T.A + BaseVID, T.B + BaseVID, T.C + BaseVID);
    }
	
	// 2) Build loops of exactly two vertices each, for each seam pair
	TArray<TArray<int>> WeldLoops;
	int32 Count = FMath::Min(A->LastSeamVertexIDs.Num(), B->LastSeamVertexIDs.Num());
	WeldLoops.Reserve(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		int VA = A->LastSeamVertexIDs[i];
		int VB = B->LastSeamVertexIDs[i] + BaseVID;
		WeldLoops.Add( TArray<int>({ VA, VB }) );
	}

	
	if (!Merged.HasAttributes())
	{
		Merged.EnableAttributes();
	}


	
    // 3) Weld each seam‐vertex pair
    UE::Geometry::FDynamicMeshEditor Editor(&Merged);
	TArray<int32> LoopA;
	TArray<int32> LoopB;
	for (int32 i = 0; i < Count; ++i)
	{
		int VA = A->LastSeamVertexIDs[i];
		int VB = B->LastSeamVertexIDs[i] + BaseVID;
		LoopA.Add(VA);
		LoopB.Add(VB);
		
		// TArray<int32> Pair = { VA, VB };
		// TArray<int32> Removed;
		// Editor.WeldVertexLoops(Pair, Removed);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("LoopA has %d vertices:"), LoopA.Num());
	// for (int i = 0; i < LoopA.Num(); ++i)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("A[%d] = %d"), i, LoopA[i]);
	// }

	UE_LOG(LogTemp, Warning, TEXT("LoopB has %d vertices:"), LoopB.Num());
	// for (int i = 0; i < LoopB.Num(); ++i)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("B[%d] = %d"), i, LoopB[i]);
	// }

	// TArray<int32> Removed;
	// Editor.WeldVertexLoops(LoopA, LoopB);
	//
	// if (!Editor.WeldVertexLoops(LoopA, LoopB))
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("WeldVertexLoops failed! Loops may be mismatched or invalid."));
	// 	return;
	// }
	//


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
