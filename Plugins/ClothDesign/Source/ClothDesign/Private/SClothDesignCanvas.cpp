#include "SClothDesignCanvas.h"
#include "ClothPatternMeshActor.h"
#include "CompGeom/Delaunay2.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshEditor.h"
#include "CoreMinimal.h"
#include "Math/MathFwd.h"
#include "UObject/Class.h"
#include "Rendering/DrawElements.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Engine/SkeletalMesh.h"
#include "ClothingSimulationInteractor.h"
#include "ClothingSimulationFactory.h"
#include "ChaosCloth/ChaosClothingSimulationInteractor.h"
#include "ClothingSimulationFactory.h"
#include "Widgets/SCompoundWidget.h"
#include "ProceduralMeshComponent.h"
#include "Math/InterpCurve.h"
#include "PatternSewingConstraint.h"
#include "Curve/DynamicGraph.h"
#include "ConstrainedDelaunay2.h"
#include "Misc/ScopeLock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "ClothShapeAsset.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Canvas/CanvasPaint.h"
#include "Canvas/CanvasUtils.h"
#include "Canvas/CanvasInputHandler.h"




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
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	// ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;


	FSlateClippingZone ClippingZone(AllottedGeometry);
	OutDrawElements.PushClip(ClippingZone);
	// OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));
	
	int32 Layer = LayerId + 1;  // Instead of 0
	FCanvasPaint Drawer(const_cast<SClothDesignCanvas*>(this));
	
	Layer = Drawer.DrawBackground(AllottedGeometry, OutDrawElements, Layer);
	Layer = Drawer.DrawGrid(AllottedGeometry, OutDrawElements, Layer);
	Layer = Drawer.DrawCompletedShapes(AllottedGeometry, OutDrawElements, Layer);
	Layer = Drawer.DrawCurrentCurve(AllottedGeometry, OutDrawElements, Layer);

	
	OutDrawElements.PopClip(); // end clipping
	return Layer;
}



FReply SClothDesignCanvas::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const float ScrollDelta = MouseEvent.GetWheelDelta();
	const float ZoomDelta = 0.1f; // How fast to zoom

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

	FCanvasInputHandler Handler(this);

	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		return Handler.HandlePan(MouseEvent).CaptureMouse(SharedThis(this));
	}
	
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();
	
	
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D LocalClickPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D CanvasClickPos = InverseTransformPoint(LocalClickPos);
		
		if (CurrentMode == EClothEditorMode::Draw)
		{
			return Handler.HandleDraw(MyGeometry, MouseEvent);
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
		SelectedPointIndex = INDEX_NONE;
		bIsDraggingPoint = false;
		bIsShapeSelected = false;
		UE_LOG(LogTemp, Warning, TEXT("Deselected all"));
		return FReply::Handled()
			.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
	}
	return FReply::Unhandled();
}





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

	
	if (CurrentMode == EClothEditorMode::Select && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D CanvasMousePos = InverseTransformPoint(LocalMousePos);
		UE_LOG(LogTemp, Warning, TEXT("CanvasClick: %s"), *CanvasMousePos.ToString());

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
				FCanvasUtils::SaveStateForUndo(UndoStack, RedoStack, GetCurrentCanvasState());


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


// second version but with steiner points, grid spaced constrained delaunay
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

	LastBuiltMesh           = MoveTemp(Mesh);
	LastBuiltSeamVertexIDs  = MoveTemp(LastSeamVertexIDs);

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


void SClothDesignCanvas::AlignSeamMeshes(
	AClothPatternMeshActor* MeshActorA,
	AClothPatternMeshActor* MeshActorB)
{
	// Access the stored vertex IDs and dynamic meshes:
	const TArray<int32>& IDsA = MeshActorA->LastSeamVertexIDs;
	const TArray<int32>& IDsB = MeshActorB->LastSeamVertexIDs;
	
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



// void SClothDesignCanvas::AddSewingConstraints(
// 	AActor* PatternPiece1,
// 	const TArray<int32>& Vertices1,
// 	AActor* PatternPiece2,
// 	const TArray<int32>& Vertices2,
// 	float Stiffness,
// 	TArray<FPatternSewingConstraint>& OutConstraints
// )
// {
// 	if (!PatternPiece1 || !PatternPiece2) return;
// 	if (Vertices1.Num() != Vertices2.Num()) return;
//
// 	USkeletalMeshComponent* Mesh1 = PatternPiece1->FindComponentByClass<USkeletalMeshComponent>();
// 	USkeletalMeshComponent* Mesh2 = PatternPiece2->FindComponentByClass<USkeletalMeshComponent>();
//
//
// 	if (!Mesh1 || !Mesh2) return;
//
// 	for (int32 i = 0; i < Vertices1.Num(); ++i)
// 	{
// 		FPatternSewingConstraint Constraint;
// 		Constraint.MeshA = Mesh1;
// 		Constraint.VertexIndexA = Vertices1[i];
// 		Constraint.MeshB = Mesh2;
// 		Constraint.VertexIndexB = Vertices2[i];
// 		Constraint.Stiffness = Stiffness;
//
// 		OutConstraints.Add(Constraint);
// 	}
// }














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

void SClothDesignCanvas::LoadShapeAssetData()
{
	if (!ClothAsset.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid shape asset to load."));
		return;
	}

	FCanvasState LoadedState;
	if (FCanvasAssets::LoadCanvasState(ClothAsset.Get(), LoadedState))
	{
		// Clear any current UI data & then restore in one go
		ClearCurrentShapeData();
		RestoreCanvasState(LoadedState);
	}

	// Force a repaint
	Invalidate(EInvalidateWidgetReason::Paint);
}


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