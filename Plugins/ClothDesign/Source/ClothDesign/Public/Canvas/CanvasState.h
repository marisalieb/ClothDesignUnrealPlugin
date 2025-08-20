#ifndef FCanvasState_H
#define FCanvasState_H

# include "Canvas/CanvasSewing.h"

/*
 * Thesis reference:
 * See Chapter 4.4 and 4.9.1 for detailed explanations.
 */

// Canvas state struct
struct FCanvasState
{
	FInterpCurve<FVector2D> CurvePoints;
	TArray<FInterpCurve<FVector2D>> CompletedShapes;
	
	int32 SelectedPointIndex;
	FVector2D PanOffset;
	float ZoomFactor;
	
	TArray<bool> bUseBezierPerPoint; // For the current curve
	TArray<TArray<bool>> CompletedBezierFlags; // for completed shapes

	// --- Sewing data ----
	TArray<FSeamDefinition> SeamDefinitions; // full seam definitions (copy)
	TMap<int32, TSet<int32>> SeamPreviewPoints; // transient preview points shown while sewing
	int32 SeamClickState = 0; // store enum as int for simple comparison
	FIntPoint AStartTarget = FIntPoint(INDEX_NONE, INDEX_NONE); // ShapeIndex, PointIndex
	FIntPoint AEndTarget   = FIntPoint(INDEX_NONE, INDEX_NONE);
	FIntPoint BStartTarget = FIntPoint(INDEX_NONE, INDEX_NONE);
	FIntPoint BEndTarget   = FIntPoint(INDEX_NONE, INDEX_NONE);
	int32 SelectedSeamIndex = INDEX_NONE;
	
	// Optional equality operator 
	bool operator==(const FCanvasState& Other) const
	{
		return	CurvePoints == Other.CurvePoints &&
				bUseBezierPerPoint == Other.bUseBezierPerPoint && 
				CompletedShapes == Other.CompletedShapes &&
				CompletedBezierFlags == Other.CompletedBezierFlags &&
				SelectedPointIndex == Other.SelectedPointIndex &&
				PanOffset == Other.PanOffset &&
				FMath::IsNearlyEqual(ZoomFactor, Other.ZoomFactor); // &&

				// // sewing compares
				// SeamDefinitions == Other.SeamDefinitions &&
				// // SeamPreviewPoints == Other.SeamPreviewPoints &&
				// SeamClickState == Other.SeamClickState &&
				// AStartTarget == Other.AStartTarget &&
				// AEndTarget   == Other.AEndTarget &&
				// BStartTarget == Other.BStartTarget &&
				// BEndTarget   == Other.BEndTarget &&
				// SelectedSeamIndex == Other.SelectedSeamIndex;
	} 
};

#endif

