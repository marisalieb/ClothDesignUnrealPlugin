
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
	
	// FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());

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
	
    // If user didn't hit anything, just consume the click and exit
    if (BestPoint == INDEX_NONE)
    {
        UE_LOG(LogTemp, Verbose, TEXT("Sew click missed all control points."));
        return FReply::Handled();
    }
	
	// ensure this clicked shape has a spawned mesh - otherwise bail out immediately
	if (!Canvas->GetSewingManager().ValidateMeshForShape(BestShape, SpawnedPatternActors, true))
	{
		return FReply::Handled();
	}
	// if the clicked point is on a completed shape, validate that shape has a generated mesh.
	// If it's an in-progress shape (BestShape == INDEX_NONE), we cannot validate here.
	if (BestShape != INDEX_NONE)
	{
		if (!Canvas->GetSewingManager().ValidateMeshForShape(BestShape, SpawnedPatternActors, true))
		{
			// validation showed a dialog; abort and do not record any points or advance state
			return FReply::Handled();
		}
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
    	FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());
    	AStartTarget = { BestShape, BestPoint };
    	SeamClickState = ESeamClickState::ClickedAStart;
    	Canvas->GetSewingManager().AddPreviewPoint(BestShape, BestPoint);
    	UE_LOG(LogTemp, Log, TEXT("Sew: AStart = [%d,%d]"), BestShape, BestPoint);
    	break;

    case ESeamClickState::ClickedAStart:
    	// Ensure second click is on the SAME shape as the first (AStart)
    	if (BestShape != AStartTarget.ShapeIndex)
    	{
    		FMessageDialog::Open(EAppMsgType::Ok,
				FText::FromString(TEXT("Please click a point on the same shape as the previous point.")));
    		UE_LOG(LogTemp, Warning, TEXT("Sew: AEnd rejected because it is on a different shape (%d) than AStart (%d)"), BestShape, AStartTarget.ShapeIndex);
    		return FReply::Handled(); // do not advance state or add preview point
    	}
    	FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());

    	AEndTarget = { BestShape, BestPoint };
    	SeamClickState = ESeamClickState::ClickedAEnd;
    	Canvas->GetSewingManager().AddPreviewPoint(BestShape, BestPoint);

    	UE_LOG(LogTemp, Log, TEXT("Sew: AEnd   = [%d,%d]"), BestShape, BestPoint);
    	break;
    	
    case ESeamClickState::ClickedAEnd:
    	FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());

    	BStartTarget = { BestShape, BestPoint };
    	SeamClickState = ESeamClickState::ClickedBStart;
    	Canvas->GetSewingManager().AddPreviewPoint(BestShape, BestPoint);

    	UE_LOG(LogTemp, Log, TEXT("Sew: BStart = [%d,%d]"), BestShape, BestPoint);
    	break;
    	
    case ESeamClickState::ClickedBStart:
    	// Ensure fourth click is on the SAME shape as the third (BStart)
    	if (BestShape != BStartTarget.ShapeIndex)
    	{
    		FMessageDialog::Open(EAppMsgType::Ok,
				FText::FromString(TEXT("Please click a point on the same shape as the previous point.")));
    		UE_LOG(LogTemp, Warning, TEXT("Sew: BEnd rejected because it is on a different shape (%d) than BStart (%d)"), BestShape, BStartTarget.ShapeIndex);
    		return FReply::Handled(); // do not advance state or add preview point
    	}
    	FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());

    	BEndTarget = { BestShape, BestPoint };

    	SeamClickState = ESeamClickState::ClickedBEnd;
    	bIsSeamReady = true;
    	Canvas->GetSewingManager().AddPreviewPoint(BestShape, BestPoint);
    	UE_LOG(LogTemp, Log, TEXT("Sew: BEnd   = [%d,%d]"), BestShape, BestPoint);

    	if (!Canvas->GetSewingManager().ValidateMeshesForTargets(AStartTarget, BStartTarget, SpawnedPatternActors, true))
    	{
    		// Dialog already shown; abort finalise and reset preview state, keep user in BEnd so they can retry.
    		Canvas->GetSewingManager().CurrentSeamPreviewPoints.Empty();
    		SeamClickState = ESeamClickState::None; // or decide appropriate rollback behavior
    		return FReply::Handled();
    	}

    	
    	// 3) Finalize: now you have (shape,index) for all four clicks
    	Canvas->GetSewingManager().FinaliseSeamDefinitionByTargets(
    		AStartTarget, AEndTarget, BStartTarget,
    		BEndTarget, Canvas->CurvePoints,
    		Canvas->CompletedShapes, SpawnedPatternActors);
    	
    	// then update canvas cache and repaint
    	Canvas->UpdateSewnPointSets();
    	Canvas->GetSewingManager().CurrentSeamPreviewPoints.Empty();

    	// Reset for next seam
    	SeamClickState = ESeamClickState::None;
    	break;

    default:
    	ensureMsgf(false,
			TEXT("Unhandled ESeamClickState in seam click handler: %d"),
			static_cast<int32>(SeamClickState));
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
	
	// 3) Selecting whole points on completed shapes
	constexpr float SelectionRadius = 10.f;
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
	
	float TangentRadius = 25.0f;

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



	
	// used to match the coordinate system for the seam selection
	const FVector2D LocalMousePos = Canvas->TransformPoint(CanvasClickPos);

	constexpr float SeamSelectionRadius = 25.0f ; // pixels threshold (tune as needed)
	constexpr float SeamSelectionRadiusSq = SeamSelectionRadius * SeamSelectionRadius;

	UE_LOG(LogTemp, Warning, TEXT("HandleSelect called, click pos: (%f, %f), ZoomFactor: %f, Seams count:"), CanvasClickPos.X, CanvasClickPos.Y, ZoomFactor);
	UE_LOG(LogTemp, Warning, TEXT("Seams count: %d"), Canvas->GetSewingManager().SeamDefinitions.Num());

	const TArray<FSeamDefinition>& Seams = Canvas->GetSewingManager().SeamDefinitions;
	for (int32 s = 0; s < Seams.Num(); ++s)
	{
	    const FSeamDefinition& SD = Seams[s];

	    auto ResolvePoint = [&](int32 ShapeIndex, int32 PtIdx, FVector2D& OutPatternPt) -> bool {
	        if (PtIdx == INDEX_NONE) return false;
	        if (ShapeIndex == INDEX_NONE)
	        {
	            if (Canvas->CurvePoints.Points.IsValidIndex(PtIdx))
	            {
	                OutPatternPt = Canvas->CurvePoints.Points[PtIdx].OutVal;
	                return true;
	            }
	            return false;
	        }
	        if (!Canvas->CompletedShapes.IsValidIndex(ShapeIndex)) return false;
	        const auto& Shape = Canvas->CompletedShapes[ShapeIndex];
	        if (!Shape.Points.IsValidIndex(PtIdx)) return false;
	        OutPatternPt = Shape.Points[PtIdx].OutVal;
	        return true;
	    };

	    FVector2D Astart, Bstart, Aend, Bend;
	    bool bAstart = ResolvePoint(SD.ShapeA, SD.EdgeA.Start, Astart);
	    bool bBstart = ResolvePoint(SD.ShapeB, SD.EdgeB.Start, Bstart);
	    bool bAend   = ResolvePoint(SD.ShapeA, SD.EdgeA.End,   Aend);
	    bool bBend   = ResolvePoint(SD.ShapeB, SD.EdgeB.End,   Bend);
		UE_LOG(LogTemp, Warning, TEXT("Checking seam %d: bAstart=%d bBstart=%d bAend=%d bBend=%d"), s, bAstart, bBstart, bAend, bBend);

		
	    // check start-start line
	    if (bAstart && bBstart)
	    {
	        FVector2D Ps = Canvas->TransformPoint(Astart);
	        FVector2D Pe = Canvas->TransformPoint(Bstart);
	    	float DistSq = DistPointToSegmentSq(LocalMousePos, Ps, Pe);
	    	UE_LOG(LogTemp, Warning, TEXT("Seam %d start-start dist sq: %f (threshold %f)"), s, DistSq, SeamSelectionRadiusSq);
	    	if (DistSq <= SeamSelectionRadiusSq)
	    	{
	    		UE_LOG(LogTemp, Warning, TEXT("Seam %d start-start line selected"), s);
	    		FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());

	            // select this seam
	            Canvas->SelectedSeamIndex = s;
	            Canvas->SelectedShapeIndex = INDEX_NONE; // clear point selection
	            Canvas->SelectedPointIndex = INDEX_NONE;
	    		UE_LOG(LogTemp, Warning, TEXT("Seam %d selected!"), s);
	    		//UE_LOG(LogTemp, Warning, TEXT("Checking seam %d start-start line: Ps=(%f,%f) Pe=(%f,%f) Click=(%f,%f) DistSq=%f Threshold=%f"), 
					//s, Ps.X, Ps.Y, Pe.X, Pe.Y, CanvasClickPos.X, CanvasClickPos.Y, DistPointToSegmentSq(CanvasClickPos, Ps, Pe), SeamSelectionRadiusSq);

	            // optionally store which sub-segment clicked (start or end) if needed
	            return FReply::Handled();
	        }
	    }

	    // check end-end line
	    if (bAend && bBend)
	    {
	        FVector2D Ps = Canvas->TransformPoint(Aend);
	        FVector2D Pe = Canvas->TransformPoint(Bend);
	    	// Before distance test: log everything
	    	//FVector2D LocalMousePos = /* whatever you get from Geometry.AbsoluteToLocal(...) at the call site */;
	    	UE_LOG(LogTemp, Warning, TEXT("Debug coords: LocalMouse=(%f,%f) CanvasClick(pos maybe pattern)=(%f,%f) Ps=(%f,%f) Pe=(%f,%f) Zoom=%f Pan=(%f,%f)"),
				LocalMousePos.X, LocalMousePos.Y,
				CanvasClickPos.X, CanvasClickPos.Y,
				Ps.X, Ps.Y, Pe.X, Pe.Y,
				Canvas->ZoomFactor, Canvas->PanOffset.X, Canvas->PanOffset.Y);
	    	
	    	float DistSq = DistPointToSegmentSq(LocalMousePos, Ps, Pe);
	    	UE_LOG(LogTemp, Warning, TEXT("Seam %d end-end dist sq: %f (threshold %f)"), s, DistSq, SeamSelectionRadiusSq);
	    	if (DistSq <= SeamSelectionRadiusSq)
	    	{
	    		UE_LOG(LogTemp, Warning, TEXT("Seam %d end-end line selected"), s);
	    		FCanvasUtils::SaveStateForUndo(Canvas->UndoStack, Canvas->RedoStack, Canvas->GetCurrentCanvasState());

	            Canvas->SelectedSeamIndex = s;
	            Canvas->SelectedShapeIndex = INDEX_NONE;
	            Canvas->SelectedPointIndex = INDEX_NONE;
	    		UE_LOG(LogTemp, Warning, TEXT("Seam %d selected!"), s);
	    // 		UE_LOG(LogTemp, Warning, TEXT("Checking seam %d end-end line: Ps=(%f,%f) Pe=(%f,%f) Click=(%f,%f) DistSq=%f Threshold=%f"), 
					// s, Ps.X, Ps.Y, Pe.X, Pe.Y, CanvasClickPos.X, CanvasClickPos.Y, DistPointToSegmentSq(CanvasClickPos, Ps, Pe), SeamSelectionRadiusSq);

	            return FReply::Handled();
	        }
	    }
	}

	// if nothing matched, clear seam selection (or keep it — choose UX)
	// Canvas->SelectedSeamIndex = INDEX_NONE;
	Canvas->SelectedSeamIndex = INDEX_NONE;
	Canvas->SelectedShapeIndex = INDEX_NONE;
	Canvas->SelectedPointIndex = INDEX_NONE;
	
    // Nothing selected—handled, but no capture/focus
    return FReply::Handled();
}

// returns squared distance from point P to segment AB (all FVector2D)
float FCanvasInputHandler::DistPointToSegmentSq(const FVector2D& P, const FVector2D& A, const FVector2D& B)
{
	const FVector2D AB = B - A;
	const FVector2D AP = P - A;
	float ABLen2 = AB.SizeSquared();
	if (ABLen2 <= KINDA_SMALL_NUMBER)
	{
		return AP.SizeSquared(); // A==B degenerate
	}
	float t = FVector2D::DotProduct(AP, AB) / ABLen2;
	t = FMath::Clamp(t, 0.0f, 1.0f);
	FVector2D Closest = A + AB * t;
	return FVector2D::DistSquared(Closest, P);
}
