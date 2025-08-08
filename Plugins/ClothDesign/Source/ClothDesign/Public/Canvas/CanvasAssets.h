#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Math/InterpCurve.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "PatternSewingConstraint.h"
#include "PatternMesh.h"
#include "Misc/ScopeLock.h"
#include "ClothShapeAsset.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "MeshOpPreviewHelpers.h" 
//#include "SClothDesignCanvas.h"
#include "PatternMesh.h"
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
#include "PatternMesh.h"
#include "CanvasState.h"          // for FCanvasState


// class SClothDesignCanvas;


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

	// bool LoadShapeAssetData(UClothShapeAsset* ClothAsset, FLoadedShapeData& OutData);
	static bool LoadCanvasState(
	UClothShapeAsset* ClothAsset,
	/* out */ FCanvasState& OutState);

};


/** 
 * Holds the currently selected shape asset and loads/saves its canvas state. 
 */
struct FCanvasAssetManager
{
	TWeakObjectPtr<UClothShapeAsset> ClothAsset;
	FString GetSelectedShapeAssetPath() const;
	bool OnShapeAssetSelected(const FAssetData& AssetData, FCanvasState& OutState);

private:
	bool LoadShapeAssetData(FCanvasState& OutState);
};
