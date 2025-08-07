#pragma once

// Canvas state struct
struct FCanvasState
{

	FInterpCurve<FVector2D> CurvePoints;  // Was: TArray<FVector2D> Points
	TArray<FInterpCurve<FVector2D>> CompletedShapes;



	int32 SelectedPointIndex;
	FVector2D PanOffset;
	float ZoomFactor;
	
	TArray<bool> bUseBezierPerPoint; // For the current curve
	TArray<TArray<bool>> CompletedBezierFlags; // for completed shapes

	
	// Optional equality operator 
	bool operator==(const FCanvasState& Other) const
	{
		return	CurvePoints == Other.CurvePoints &&
				bUseBezierPerPoint == Other.bUseBezierPerPoint && 
				CompletedShapes == Other.CompletedShapes &&
				CompletedBezierFlags == Other.CompletedBezierFlags &&
				SelectedPointIndex == Other.SelectedPointIndex &&
				PanOffset == Other.PanOffset &&
				FMath::IsNearlyEqual(ZoomFactor, Other.ZoomFactor);
	} 
};

