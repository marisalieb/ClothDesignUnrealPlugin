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



	
	// 3. Advance layer so points draw on top of lines
	LayerId++;

	// 4. Draw interactive points as boxes (highlight selected)
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		FVector2D DrawPos = TransformPoint(Points[i]);
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
					FLinearColor::Green,
					true, 2.0f
				);
			}
		}

		++LayerId;
	}
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

		// Draw
		if (CurrentMode == EClothEditorMode::Draw)
		{
			SaveStateForUndo();
			// Points.Add(CanvasClickPos);
			// float Key = CanvasClickPos.X; // or use an incrementing ID if you want to decouple from X
			// CurvePoints.Points.Add(FInterpCurvePoint<FVector2D>(Key, CanvasClickPos));
			// CurvePoints.Points.Sort([](const FInterpCurvePoint<FVector2D>& A, const FInterpCurvePoint<FVector2D>& B) {
			// 	return A.InVal < B.InVal;
			// });
			FInterpCurvePoint<FVector2D> NewPoint;
			NewPoint.InVal = CurvePoints.Points.Num(); // Or use a running float like TotalDistance or cumulative X
			NewPoint.OutVal = CanvasClickPos; // Clicked canvas-space position
			NewPoint.InterpMode = CIM_CurveAuto; // Or CIM_Linear if you want sharp lines
			// for (auto& Point : CurvePoints.Points)
			// {
			// 	Point.InterpMode = CIM_CurveAuto;
			// }


			CurvePoints.Points.Add(NewPoint);
			CurvePoints.AutoSetTangents();

			
			UE_LOG(LogTemp, Warning, TEXT("Draw mode: Added point at (%f, %f)"), CanvasClickPos.X, CanvasClickPos.Y);
			return FReply::Handled();
		}

		// Select and Move
		else if (CurrentMode == EClothEditorMode::Select) // || CurrentMode == EClothEditorMode::Move)
		{
			const float SelectionRadius = 10.0f;
			for (int32 i = 0; i < Points.Num(); ++i)
			{
				if (FVector2D::Distance(Points[i], CanvasClickPos) < SelectionRadius / ZoomFactor)
				{
					SaveStateForUndo();
					SelectedPointIndex = i;
					bIsShapeSelected = false;
					bIsDraggingPoint = true;

					UE_LOG(LogTemp, Warning, TEXT("Selected point %d"), i);

					return FReply::Handled()
						.CaptureMouse(SharedThis(this))
						.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
				}
			}

			
			if (Points.Num() < 2)
				return FReply::Handled(); // not enough points

			// No point selected, check if clicked near a line
			const float LineSelectThreshold = 10.f / ZoomFactor;
			for (int32 i = 0; i < Points.Num() - 1; ++i)
			{
				const FVector2D A = Points[i];
				const FVector2D B = Points[(i + 1) % Points.Num()]; // Loop around if closed

				if (IsPointNearLine(CanvasClickPos, A, B, LineSelectThreshold))
				{
					UE_LOG(LogTemp, Warning, TEXT("ZoomFactor: %f"), ZoomFactor);

					SaveStateForUndo();
					bIsShapeSelected = true;
					SelectedPointIndex = INDEX_NONE;
					bIsDraggingShape = true;

					UE_LOG(LogTemp, Warning, TEXT("Selected line near segment %d"), i);
					return FReply::Handled().CaptureMouse(SharedThis(this));
				}
			}

			
			// Clicked on empty space; DESELECT
			SelectedPointIndex = INDEX_NONE;
			bIsDraggingPoint = false;
			bIsShapeSelected = false;
			UE_LOG(LogTemp, Warning, TEXT("Deselected all"));
			return FReply::Handled()
				.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
		}
	}

	return FReply::Unhandled();
}

FReply SClothDesignCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ((CurrentMode == EClothEditorMode::Move || CurrentMode == EClothEditorMode::Select) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (bIsDraggingPoint && SelectedPointIndex != INDEX_NONE)
		{
			FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const FVector2D CanvasMousePos = InverseTransformPoint(LocalMousePos);

			// Points[SelectedPointIndex] = InverseTransformPoint(LocalPos);
			Points[SelectedPointIndex] = CanvasMousePos;

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SClothDesignCanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDraggingPoint && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	// if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
	{
		bIsDraggingPoint = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SClothDesignCanvas::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("Key pressed: %s"), *InKeyEvent.GetKey().ToString());

	const FKey Key = InKeyEvent.GetKey();


	if (Key == EKeys::Delete || Key == EKeys::BackSpace)
	{
		if (SelectedPointIndex != INDEX_NONE && SelectedPointIndex < Points.Num())
		{
			Points.RemoveAt(SelectedPointIndex);
			SelectedPointIndex = INDEX_NONE;
			// You may also want to refresh/redraw the canvas here if needed
		}
		else if (bIsShapeSelected)
		{
			Points.Empty();
			bIsShapeSelected = false;
		}
		return FReply::Handled();
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
	
	//return FReply::Unhandled();
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);

}


bool SClothDesignCanvas::IsPointNearLine(const FVector2D& P, const FVector2D& A, const FVector2D& B, float Threshold) const
{
	const FVector2D AP = P - A;
	const FVector2D AB = B - A;

	const float ABLengthSq = AB.SizeSquared();
	if (ABLengthSq == 0.f) return false;

	const float T = FMath::Clamp(FVector2D::DotProduct(AP, AB) / ABLengthSq, 0.f, 1.f);
	const FVector2D ClosestPoint = A + T * AB;
	return FVector2D::Distance(P, ClosestPoint) <= Threshold;
}


FReply SClothDesignCanvas::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("Canvas received keyboard focus"));
	return FReply::Handled();
}
void SClothDesignCanvas::TriangulateAndBuildMesh()
{
	if (Points.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
		return;
	}

	// Step 1: Convert to UE::Geometry::FVector2f
	TArray<FVector2f> PolyVerts;
	for (const FVector2D& P : Points)
	{
		PolyVerts.Add(FVector2f(P.X, P.Y));
	}

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
	Remesher.SetTargetEdgeLength(50.50); // Set this to desired density
	
	Remesher.SmoothSpeedT = 0.0f; // Optional tweak

	for (int i = 0; i < 2; ++i)  // 1 not enough, 3-5 too much in terms of edge vertices changing
	{
		Remesher.BasicRemeshPass();
	}


	// (Optional) Subdivide for more detail
	// UE::Geometry::FDynamicMeshEditor Editor(&Mesh);
	
	int32 NumSubdivisions = 3;
	double MaxEdgeLength = 1.0;
	
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

	AClothPatternMeshActor* MeshActor = World->SpawnActor<AClothPatternMeshActor>();
	if (!MeshActor) return;

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


void SClothDesignCanvas::SaveStateForUndo()
{
	UndoStack.Add(GetCurrentCanvasState());
	RedoStack.Empty(); // Clear redo on new action
}

FCanvasState SClothDesignCanvas::GetCurrentCanvasState() const
{
	FCanvasState State;
	State.Points = Points;
	State.SelectedPointIndex = SelectedPointIndex;
	State.PanOffset = PanOffset;
	State.ZoomFactor = ZoomFactor;
	return State;
}

void SClothDesignCanvas::RestoreCanvasState(const FCanvasState& State)
{
	Points = State.Points;
	SelectedPointIndex = State.SelectedPointIndex;
	PanOffset = State.PanOffset;
	ZoomFactor = State.ZoomFactor;

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
