#include "ClothDesignCanvas.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "CoreMinimal.h"
#include "Math/MathFwd.h"
#include "UObject/Class.h"
#include "Rendering/DrawElements.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ClothingSimulationFactory.h"
#include "ClothingSimulationFactory.h"
#include "Widgets/SCompoundWidget.h"
#include "Math/InterpCurve.h"
#include "Misc/ScopeLock.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"

#include "Canvas/CanvasPaint.h"
#include "Canvas/CanvasUtils.h"
#include "Canvas/CanvasInputHandler.h"
#include "Canvas/CanvasMesh.h"


void SClothDesignCanvas::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNullWidget::NullWidget
		]
	];
	
	this->SetEnabled(true);
	this->SetCanTick(true);
	

	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
}

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
	const FSlateRect& CullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const_cast<SClothDesignCanvas*>(this)->LastGeometry = AllottedGeometry;
	
	// Respect parent clipping
	// const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	// ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;


	FSlateClippingZone ClippingZone(AllottedGeometry);
	OutDrawElements.PushClip(ClippingZone);
	// OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));
	
	int32 Layer = LayerId + 1;  // Instead of 0
	FCanvasPaint Drawer(const_cast<SClothDesignCanvas*>(this));
	
	Layer = Drawer.DrawBackground(AllottedGeometry, OutDrawElements, Layer);
	Layer = Drawer.DrawGrid(AllottedGeometry, OutDrawElements, Layer);
	Layer = Drawer.DrawFinalisedSeamLines(AllottedGeometry, OutDrawElements, Layer);
	Layer = Drawer.DrawCompletedShapes(AllottedGeometry, OutDrawElements, Layer);
	Layer = Drawer.DrawCurrentShape(AllottedGeometry, OutDrawElements, Layer);

	
	OutDrawElements.PopClip(); // end clipping
	return Layer;
}



FReply SClothDesignCanvas::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	const float ScrollDelta = MouseEvent.GetWheelDelta();
	const FVector2D MousePos = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	// Compute the world position under the mouse before zoom
	const FVector2D WorldBeforeZoom = (MousePos - PanOffset) / ZoomFactor;

	// constexpr float ZoomDelta = 0.1f; // How fast to zoom
	// Adjust zoom factor multiplicatively
	float ZoomScale = 1.1f; // How much each wheel tick zooms (10% per tick)
	if (ScrollDelta < 0)
		ZoomFactor /= FMath::Pow(ZoomScale, -ScrollDelta);
	else
		ZoomFactor *= FMath::Pow(ZoomScale, ScrollDelta);
	
	// Adjust zoom factor
	ZoomFactor = FMath::Clamp(ZoomFactor, 1.f, 20.0f);

	// Recalculate pan offset to keep zoom centered under mouse
	PanOffset = MousePos - WorldBeforeZoom * ZoomFactor;

	// Repaint
	Invalidate(EInvalidateWidget::LayoutAndVolatility);

	return FReply::Handled();
}



FReply SClothDesignCanvas::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMouseButtonDown fired. Mode: %d"), (int32)CurrentMode);

	FCanvasInputHandler Handler(this);

	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		return Handler.HandlePan(MouseEvent).CaptureMouse(SharedThis(this));
	}
	
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();
	
	
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D LocalClickPos = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D CanvasClickPos = InverseTransformPoint(LocalClickPos);
		
		if (CurrentMode == EClothEditorMode::Draw)
		{
			return Handler.HandleDraw(Geometry, MouseEvent);
		}
		
		else if (CurrentMode == EClothEditorMode::Sew)
		{
			return Handler.HandleSew(CanvasClickPos);
		}

		else if (CurrentMode == EClothEditorMode::Select)
		{
			return Handler.HandleSelect(CanvasClickPos);
		}
		
		if (CurvePoints.Points.Num() < 2)
			return FReply::Handled(); // not enough points
		
		// Clicked on empty space; DESELECT
		
		SelectedSeamIndex = INDEX_NONE;
		SelectedShapeIndex = INDEX_NONE;
		SelectedPointIndex = INDEX_NONE;
		bIsDraggingPoint = false;
		bIsShapeSelected = false;
		UE_LOG(LogTemp, Warning, TEXT("Deselected all"));
		// Invalidate(EInvalidateWidget::Paint);  // trigger repaint so highlight updates
		return FReply::Handled()
			.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
	}
	return FReply::Unhandled();
}





FReply SClothDesignCanvas::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
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

	
	if (CurrentMode == EClothEditorMode::Select && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		FVector2D LocalMousePos = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D CanvasMousePos = InverseTransformPoint(LocalMousePos);
		//UE_LOG(LogTemp, Warning, TEXT("CanvasClick: %s"), *CanvasMousePos.ToString());

		if (bIsDraggingTangent && SelectedPointIndex != INDEX_NONE)
		{
			
			if (SelectedShapeIndex == INDEX_NONE)
			{
	
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
					FCanvasUtils::RecalculateNTangents(CurvePoints, bUseBezierPerPoint);
			}
			else
			{
				CompletedShapes[SelectedShapeIndex].Points[SelectedPointIndex].OutVal = CanvasMousePos;
				if (!CompletedBezierFlags[SelectedShapeIndex][SelectedPointIndex])
					FCanvasUtils::RecalculateNTangents(
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


FReply SClothDesignCanvas::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
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



FReply SClothDesignCanvas::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& InKeyEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("Key pressed: %s"), *InKeyEvent.GetKey().ToString());

	const FKey Key = InKeyEvent.GetKey();


	if (Key == EKeys::Delete || Key == EKeys::BackSpace)
	{
		// seam deletion
		// If a seam is selected -> delete the seam (remove its bookkeeping + runtime constraint)
		if (SelectedSeamIndex != INDEX_NONE)
		{
			FCanvasUtils::SaveStateForUndo(UndoStack, RedoStack, GetCurrentCanvasState());

			int32 idx = SelectedSeamIndex;

			// 1) Remove FSeamDefinition
			if (GetSewingManager().SeamDefinitions.IsValidIndex(idx))
			{
				GetSewingManager().SeamDefinitions.RemoveAt(idx);
			}

			// 2) Remove runtime cached constraint if you kept one-per-seam in AllDefinedSeams (best-effort)
			if (GetSewingManager().AllDefinedSeams.IsValidIndex(idx))
			{
				GetSewingManager().AllDefinedSeams.RemoveAt(idx);
			}
			// If your runtime sewing constraints are stored separately, try to find and  remove matching entries:
			// e.g. if you have FPatternSewingConstraint list, compare ScreenPointsA/B or Mesh pointers to find the matching one and remove it.

			// 3) Update the canvas sewn-point cache and redraw
			UpdateSewnPointSets(); // recompute sewn-point map from SeamDefinitions
			SelectedSeamIndex = INDEX_NONE;
			Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
			return FReply::Handled();
		}


		// point deletion
		UE_LOG(LogTemp, Warning, TEXT("Trying to deelete point: %d"), SelectedPointIndex);
		
		if (SelectedPointIndex != INDEX_NONE)
		{
			FCanvasUtils::SaveStateForUndo(UndoStack, RedoStack, GetCurrentCanvasState());


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

	else if (Key == EKeys::Three)
	{
		CurrentMode = EClothEditorMode::Sew;
		UE_LOG(LogTemp, Warning, TEXT("Switched to Sew mode"));

		return FReply::Handled();
	}
	
	// Check for Undo (Ctrl+Z)
	if (Key == EKeys::Z && InKeyEvent.IsControlDown())
	{
		FCanvasState CurrentState = GetCurrentCanvasState();
		if (FCanvasUtils::Undo(UndoStack, RedoStack, CurrentState))
		{
			RestoreCanvasState(CurrentState);
			return FReply::Handled();
		}
	}

	if (Key == EKeys::Y && InKeyEvent.IsControlDown())
	{
		FCanvasState CurrentState = GetCurrentCanvasState();
		if (FCanvasUtils::Redo(UndoStack, RedoStack, CurrentState))
		{
			RestoreCanvasState(CurrentState);
			return FReply::Handled();
		}
	}
	
	if (Key == EKeys::F)
	{
		FocusViewportOnPoints();
		return FReply::Handled();
	}
	
	if (CurrentMode == EClothEditorMode::Select)
	{
		if (Key == EKeys::T) // so hold down while editing the tangent
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
			FinaliseCurrentShape();
			return FReply::Handled();
		}
	}
	
	//return FReply::Unhandled();
	return SCompoundWidget::OnKeyDown(Geometry, InKeyEvent);

}


int32 SClothDesignCanvas::FinaliseCurrentShape(bool bGenerateNow, TArray<TWeakObjectPtr<APatternMesh>>* OutSpawnedActors)
{
	 // nothing to finalize
    if (CurvePoints.Points.Num() == 0)
    {
        UE_LOG(LogTemp, Verbose, TEXT("FinalizeCurrentShape: no points to finalize"));
        return INDEX_NONE;
    }
	
	FCanvasUtils::SaveStateForUndo(UndoStack, RedoStack, GetCurrentCanvasState());
	

    // Add bezier tangents to the start and end points (same as your original)
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

    // Move current sketch into CompletedShapes
    CompletedShapes.Add(CurvePoints);
    CompletedBezierFlags.Add(bUseBezierPerPoint); // keep your existing boolean flags array in sync

	int32 NewIndex = CompletedShapes.Num() - 1;

    // Clear the current sketch so user can start a new one
    CurvePoints.Points.Empty();
    bUseBezierPerPoint.Empty();

    UE_LOG(LogTemp, Warning, TEXT("Shape finalised. Ready to start a new one."));
	
	return NewIndex;
}


FReply SClothDesignCanvas::OnKeyUp(const FGeometry& Geometry, const FKeyEvent& InKeyEvent)
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
	return SCompoundWidget::OnKeyUp(Geometry, InKeyEvent);
}


FReply SClothDesignCanvas::OnFocusReceived(const FGeometry& Geometry, const FFocusEvent& InFocusEvent)
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
	constexpr float Margin = 1.2f; // add some margin around the bounds

	float ZoomX = ViewportSize.X / (BoundsSize.X * Margin);
	float ZoomY = ViewportSize.Y / (BoundsSize.Y * Margin);
	ZoomFactor = FMath::Clamp(FMath::Min(ZoomX, ZoomY), 0.1f, 20.0f);
	
	PanOffset = ViewportSize * 0.5f - Center * ZoomFactor;
}

FReply SClothDesignCanvas::OnModeButtonClicked(EClothEditorMode NewMode)
{
	CurrentMode = NewMode;
	Invalidate(EInvalidateWidget::Paint);
	return FReply::Handled();
}



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

	// --- Sewing manager data ---
	const auto& SewingMgr = GetSewingManager(); // your accessor

	// copy seam definitions (deep copy)
	State.SeamDefinitions = SewingMgr.SeamDefinitions;

	// copy the transient preview points drawn during sewing
	State.SeamPreviewPoints = SewingMgr.CurrentSeamPreviewPoints;

	// store click-state as int
	State.SeamClickState = static_cast<int32>(SewingMgr.SeamClickState);

	// copy the 4 targets (shape,index) into FIntPoint pairs
	State.AStartTarget = FIntPoint(SewingMgr.AStartTarget.ShapeIndex, SewingMgr.AStartTarget.PointIndex);
	State.AEndTarget   = FIntPoint(SewingMgr.AEndTarget.ShapeIndex,   SewingMgr.AEndTarget.PointIndex);
	State.BStartTarget = FIntPoint(SewingMgr.BStartTarget.ShapeIndex, SewingMgr.BStartTarget.PointIndex);
	State.BEndTarget   = FIntPoint(SewingMgr.BEndTarget.ShapeIndex,   SewingMgr.BEndTarget.PointIndex);

	State.SelectedSeamIndex = SelectedSeamIndex;
	
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

	// restore sewing manager
	auto& SewingMgr = GetSewingManager();
	SewingMgr.SeamDefinitions = State.SeamDefinitions;
	SewingMgr.CurrentSeamPreviewPoints = State.SeamPreviewPoints;
	SewingMgr.SeamClickState = static_cast<ESeamClickState>(State.SeamClickState);
	SewingMgr.AStartTarget = { State.AStartTarget.X, State.AStartTarget.Y };
	SewingMgr.AEndTarget   = { State.AEndTarget.X,   State.AEndTarget.Y };
	SewingMgr.BStartTarget = { State.BStartTarget.X, State.BStartTarget.Y };
	SewingMgr.BEndTarget   = { State.BEndTarget.X,   State.BEndTarget.Y };

	SelectedSeamIndex = State.SelectedSeamIndex;
	
	UpdateSewnPointSets();

	Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
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







// manager is a data only layer, canvas widget is the ui layer so it owns the visual state


FString SClothDesignCanvas::GetSelectedShapeAssetPath() const
{
    return AssetManager.GetSelectedShapeAssetPath();
}

void SClothDesignCanvas::OnShapeAssetSelected(const FAssetData& AssetData)
{
	FCanvasState LoadedState;
	if (AssetManager.OnShapeAssetSelected(AssetData, LoadedState))
	{
		// UI-specific steps:
		// ClearAllShapeData();
		RestoreCanvasState(LoadedState);
		FocusViewportOnPoints();
		Invalidate(EInvalidateWidgetReason::Paint);
	}
	
}

void SClothDesignCanvas::ClearAllShapeData()
{
	CompletedShapes.Empty();
	CompletedBezierFlags.Empty();

	CurvePoints.Points.Empty();
	bUseBezierPerPoint.Empty();
	
	// Reset selection and indices
	SelectedPointIndex = INDEX_NONE;
	SelectedShapeIndex = INDEX_NONE;
	
	ClearAllSewing();
	
	Invalidate(EInvalidateWidgetReason::Paint);
}


void SClothDesignCanvas::SewingClick()
{
	SewingManager.BuildAndAlignAllSeams();
}


void SClothDesignCanvas::MergeClick()
{
	SewingManager.MergeSewnPatternPieces();
}

void SClothDesignCanvas::ClearAllSewing()
{
	SewnPointIndicesPerShape.Empty();
	GetSewingManager().CurrentSeamPreviewPoints.Empty();
	SewingManager.ClearAllSeams();
}


// move here from the uimodule class so that canvas owns all the how to save logic using the canvasassets
FReply SClothDesignCanvas::SaveClick(const FString& SaveName)
{
	if (SaveName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Please enter a name before saving."));
		return FReply::Handled();
	}
	
	// Save
	bool bOK = FCanvasAssets::SaveShapeAsset(
		TEXT("SavedClothMeshes"),
		SaveName,
		CompletedShapes,
		CompletedBezierFlags,
		CurvePoints,
		bUseBezierPerPoint
	);

	UE_LOG(LogTemp, Log, TEXT("Save %s: %s"), *SaveName, bOK ? TEXT("Success") : TEXT("FAILED"));

	return FReply::Handled();
}




void SClothDesignCanvas::GenerateMeshesClick()
{
	SewingManager.SpawnedPatternActors.Empty();
	
	TArray<FDynamicMesh3> AllMeshes;
	CanvasMesh CanvasMesh;

	CanvasMesh.TriangulateAndBuildAllMeshes(
		CompletedShapes,
		CurvePoints,
		AllMeshes,
		SewingManager.SpawnedPatternActors);

	for (int i = 0; i < SewingManager.SpawnedPatternActors.Num(); ++i)
	{
		if (APatternMesh* A = SewingManager.SpawnedPatternActors[i].Get())
		{
			UE_LOG(LogTemp, Warning, TEXT("overall SpawnedActors[%d] = %s"), i, *A->GetName());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Built %d meshes"), AllMeshes.Num());
	// You can inspect AllMeshes[0].TriangleCount(), VertexCount(), etc.
}


// Canvas.cpp
void SClothDesignCanvas::UpdateSewnPointSets()
{
	//SewnPointIndicesPerShape.Empty();

	SewingManager.BuildSewnPointSets(SewnPointIndicesPerShape);
	
	// request redraw
	Invalidate(EInvalidateWidget::Paint);
}


