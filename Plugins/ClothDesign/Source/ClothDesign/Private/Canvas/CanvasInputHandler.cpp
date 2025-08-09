
#include "Canvas/CanvasInputHandler.h"
#include "Widgets/SWidget.h"
#include "Logging/LogMacros.h"
#include "Misc/MessageDialog.h"

#include "ClothDesignCanvas.h"
#include "Canvas/CanvasUtils.h"
#include "Canvas/CanvasSewing.h"



FCanvasInputHandler::FCanvasInputHandler(SClothDesignCanvas* InCanvas)
	: Canvas(InCanvas)
{}

FReply FCanvasInputHandler::HandlePan(const FPointerEvent& MouseEvent)
{
	Canvas->bIsPanning   = true;
	Canvas->LastMousePos = MouseEvent.GetScreenSpacePosition();
	return FReply::Handled();
}


FReply FCanvasInputHandler::HandleDraw(const FGeometry& Geo, const FPointerEvent& MouseEvent)
{
	FVector2D LocalClick  = Geo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	FVector2D CanvasClickPos = Canvas->InverseTransformPoint(LocalClick);

	FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());

	// add a point…
	auto& CurvePoints = Canvas->CurvePoints;
	FInterpCurvePoint<FVector2D> NewPoint;
	NewPoint.InVal     = CurvePoints.Points.Num();
	NewPoint.OutVal    = CanvasClickPos;
	NewPoint.InterpMode= CIM_CurveAuto;
	CurvePoints.Points.Add(NewPoint);
	Canvas->bUseBezierPerPoint.Add(Canvas->bUseBezierPoints);

	if (!Canvas->bUseBezierPoints)
	{
		FCanvasUtils::RecalculateNTangents(CurvePoints, Canvas->bUseBezierPerPoint);
	}

	else if (Canvas->bUseBezierPoints)
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
	
	UE_LOG(LogTemp, Warning, TEXT("Draw: Added point at %f,%f"), CanvasClickPos.X, CanvasClickPos.Y);
	return FReply::Handled();
}

FReply FCanvasInputHandler::HandleSew(const FVector2D& CanvasClickPos)
{
	
	FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());

	// add a point…
	auto& CurvePoints = Canvas->CurvePoints;
	auto& CompletedShapes =Canvas->CompletedShapes;
	auto& SeamClickState =Canvas->GetSewingManager().SeamClickState;
	auto&  AStartTarget=Canvas->GetSewingManager().AStartTarget ;
	auto&  AEndTarget=Canvas->GetSewingManager().AEndTarget;
	auto&  BStartTarget=Canvas->GetSewingManager().BStartTarget ;
	auto&  BEndTarget=Canvas->GetSewingManager().BEndTarget ;
	auto&  SpawnedPatternActors=Canvas->GetSewingManager().SpawnedPatternActors;
	
	 // 1) Find nearest control point across all shapes + current
    const float SelRadius = 10.0f / Canvas->ZoomFactor;
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



	// If user clicked a point on the in-progress shape
	if (BestShape == INDEX_NONE)
	{
		// Ask user whether to finalize this shape now for sewing
		EAppReturnType::Type Choice = FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::FromString(TEXT("You clicked a point on the unfinished shape. Finalize this shape now so you can sew it? (Yes = finalize and allow sewing, No = cancel)"))
		);

		if (Choice == EAppReturnType::Yes)
		{
			int32 NewShapeIndex = Canvas->FinaliseCurrentShape();
			if (NewShapeIndex == INDEX_NONE)
			{
				return FReply::Handled(); // nothing finalized
			}

			// Recompute BestShape/BestPoint so the click maps to the new completed shape:
			BestShape = NewShapeIndex;
			BestPoint = INDEX_NONE;
			for (int i = 0; i < Canvas->CompletedShapes[BestShape].Points.Num(); ++i)
			{
				float D2 = FVector2D::DistSquared(Canvas->CompletedShapes[BestShape].Points[i].OutVal, CanvasClickPos);
				if (D2 <= BestRadiusSq) { BestPoint = i; break; }
			}
			if (BestPoint == INDEX_NONE) return FReply::Handled(); // shouldn't happen, but safe
		}
		else
		{
			// User chose not to finalize — ignore the click for sewing
			return FReply::Handled();
		}
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
    	Canvas->GetSewingManager().FinaliseSeamDefinitionByTargets(AStartTarget, AEndTarget, BStartTarget, BEndTarget, Canvas->CurvePoints, Canvas->CompletedShapes, SpawnedPatternActors);

    	// Reset for next seam
    	SeamClickState = ESeamClickState::None;
    	break;
    }

    return FReply::Handled();
}




FReply FCanvasInputHandler::HandleSelect(const FVector2D& CanvasClickPos)
{
    // Alias the data members on the helper
    auto& CurvePoints        = Canvas->CurvePoints;
    auto& CompletedShapes    = Canvas->CompletedShapes;
    auto& BezierFlags        = Canvas->CompletedBezierFlags;
    auto& bUseBezierPerPoint = Canvas->bUseBezierPerPoint;
    float  ZoomFactor        = Canvas->ZoomFactor;
    // float  TangentRadius     = Canvas->TangentHandleRadius;
	float TangentRadius = 25.0f; // or whatever pixel radius

    // 1) Try selecting a tangent handle on completed shapes
    for (int32 ShapeIndex = 0; ShapeIndex < CompletedShapes.Num(); ++ShapeIndex)
    {
        const auto& Shape = CompletedShapes[ShapeIndex];
        const auto& Flags = BezierFlags[ShapeIndex];
        int32 NumPts = Shape.Points.Num();

        for (int32 i = 0; i < NumPts; ++i)
        {
            const auto& Pt = Shape.Points[i];
            FVector2D World = Pt.OutVal;

            FVector2D Arrive = Flags[i]
                ? (World - Pt.ArriveTangent)
                : (World + (i > 0 ? Shape.Points[i-1].OutVal : World)) * 0.5f;

            FVector2D Leave = Flags[i]
                ? (World + Pt.LeaveTangent)
                : (World + (i < NumPts-1 ? Shape.Points[i+1].OutVal : World)) * 0.5f;

            if (!Flags[i]) continue;

            if (FVector2D::Distance(CanvasClickPos, Arrive) < TangentRadius / ZoomFactor)
            {
                FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());
                Canvas->SelectedShapeIndex    = ShapeIndex;
                Canvas->SelectedPointIndex    = i;
                Canvas->SelectedTangentHandle = SClothDesignCanvas::ETangentHandle::Arrive;
                Canvas->bIsDraggingTangent    = true;
                return FReply::Handled();  // capture/focus done by widget
            }
            else if (FVector2D::Distance(CanvasClickPos, Leave) < TangentRadius / ZoomFactor)
            {
                FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());
                Canvas->SelectedShapeIndex    = ShapeIndex;
                Canvas->SelectedPointIndex    = i;
                Canvas->SelectedTangentHandle = SClothDesignCanvas::ETangentHandle::Leave;
                Canvas->bIsDraggingTangent    = true;
                return FReply::Handled();
            }
        }
    }

    // 2) Then the in-progress shape’s handles
    for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
    {
        const auto& Pt = CurvePoints.Points[i];
        if (!bUseBezierPerPoint[i])
            continue;

        FVector2D Arrive = Pt.OutVal - Pt.ArriveTangent;
        FVector2D Leave  = Pt.OutVal + Pt.LeaveTangent;

        if (FVector2D::Distance(CanvasClickPos, Arrive) < TangentRadius / ZoomFactor)
        {
            FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());
            Canvas->SelectedShapeIndex    = INDEX_NONE;
            Canvas->SelectedPointIndex    = i;
            Canvas->SelectedTangentHandle = SClothDesignCanvas::ETangentHandle::Arrive;
            Canvas->bIsDraggingTangent    = true;
            return FReply::Handled();
        }
        else if (FVector2D::Distance(CanvasClickPos, Leave) < TangentRadius / ZoomFactor)
        {
            FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());
            Canvas->SelectedShapeIndex    = INDEX_NONE;
            Canvas->SelectedPointIndex    = i;
            Canvas->SelectedTangentHandle = SClothDesignCanvas::ETangentHandle::Leave;
            Canvas->bIsDraggingTangent    = true;
            return FReply::Handled();
        }
    }

    // 3) Selecting whole points on completed shapes
    const float SelectionRadius = 10.f;
    for (int32 ShapeIndex = 0; ShapeIndex < CompletedShapes.Num(); ++ShapeIndex)
    {
        const auto& Shape = CompletedShapes[ShapeIndex];
        for (int32 i = 0; i < Shape.Points.Num(); ++i)
        {
            FVector2D WorldPoint = Shape.Points[i].OutVal;
            if (FVector2D::Distance(WorldPoint, CanvasClickPos) < SelectionRadius / ZoomFactor)
            {
                FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());
                Canvas->SelectedShapeIndex = ShapeIndex;
                Canvas->SelectedPointIndex = i;
                Canvas->bIsShapeSelected   = true;
                Canvas->bIsDraggingPoint   = true;
                return FReply::Handled();
            }
        }
    }

    // 4) Selecting points in the in-progress curve
    for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
    {
        FVector2D WorldPoint = CurvePoints.Points[i].OutVal;
        if (FVector2D::Distance(WorldPoint, CanvasClickPos) < SelectionRadius / ZoomFactor)
        {
            FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());
            Canvas->SelectedShapeIndex = INDEX_NONE;
            Canvas->SelectedPointIndex = i;
            Canvas->bIsShapeSelected   = true;
            Canvas->bIsDraggingPoint   = true;
            return FReply::Handled();
        }
    }

    // Nothing selected—handled, but no capture/focus
    return FReply::Handled();
}


