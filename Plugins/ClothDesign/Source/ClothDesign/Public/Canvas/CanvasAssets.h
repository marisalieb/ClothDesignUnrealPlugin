#ifndef FCanvasAssets_H
#define FCanvasAssets_H

#include "ClothShapeAsset.h"
#include "CanvasState.h" 

/*
 * Thesis reference:
 * See Chapter 4.9.1 for detailed explanations.
 */


struct FLoadedShapeData
{
	TArray<FInterpCurve<FVector2D>> CompletedShapes;
	TArray<TArray<bool>> CompletedBezierFlags;
	FInterpCurve<FVector2D> CurvePoints;
	TArray<bool> bUseBezierPerPoint;
};


struct FCanvasAssets
{

	static bool SaveShapeAsset(
		const FString& AssetPath,
		const FString& AssetName,
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
		const TArray<TArray<bool>>& CompletedBezierFlags,
		const FInterpCurve<FVector2D>& CurvePoints,
		const TArray<bool>& bUseBezierPerPoint
	);
	
	static bool LoadCanvasState(
		UClothShapeAsset* ClothAsset,
		FCanvasState& OutState);

};

// holds the currently selected shape asset and loads/saves its canvas state
struct FCanvasAssetManager
{
public:
	TWeakObjectPtr<UClothShapeAsset> ClothAsset;
	FString GetSelectedShapeAssetPath() const;
	bool OnShapeAssetSelected(const FAssetData& AssetData, FCanvasState& OutState);

private:
	bool LoadShapeAssetData(FCanvasState& OutState) const;
	
};

#endif

