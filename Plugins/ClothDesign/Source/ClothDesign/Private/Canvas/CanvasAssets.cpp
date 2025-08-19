#include "Canvas/CanvasAssets.h"

#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "Misc/PackageName.h"

bool FCanvasAssets::SaveShapeAsset(
	const FString& AssetPath,
	const FString& AssetName,
	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	const TArray<TArray<bool>>& CompletedBezierFlags,
	const FInterpCurve<FVector2D>& CurvePoints,
	const TArray<bool>& bUseBezierPerPoint)
{
	if (AssetPath.Contains(TEXT(":")) || AssetPath.Contains(TEXT("?")))
	{
		return false; // reject illegal chars
	}
	
	// Create package path - e.g. /Game/YourFolder/AssetName
	FString PackageName = FString::Printf(TEXT("/Game/ClothDesign/%s/%s"), *AssetPath, *AssetName);
	FString SanitizedPackageName = UPackageTools::SanitizePackageName(PackageName);
	
	UPackage* Package = LoadPackage(nullptr, *SanitizedPackageName, LOAD_None);
	if (!Package)
	{
		// If the package doesn't exist, create it
		Package = CreatePackage(*SanitizedPackageName);
	}
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for saving asset"));
		return false;
	}

	// Force full load of the package (if not already)
	Package->FullyLoad();

	
	// Try to find existing asset
	UClothShapeAsset* ExistingAsset = FindObject<UClothShapeAsset>(Package, *AssetName);
	UClothShapeAsset* TargetAsset = ExistingAsset;

	if (!TargetAsset)
	{
		// If it doesn't exist, create a new one
		TargetAsset = NewObject<UClothShapeAsset>(Package, *AssetName, RF_Public | RF_Standalone);
		if (!TargetAsset)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create new UClothShapeAsset"));
			return false;
		}
	}

	// Clear and copy your canvas data to the asset
	TargetAsset->ClothShapes.Empty();
	TargetAsset->ClothCurvePoints.Empty();



	
	// Copy all completed shapes into asset
	// for (const auto& ShapeCurve : CompletedShapes)
	// {
	for (int32 ShapeIdx = 0; ShapeIdx < CompletedShapes.Num(); ++ShapeIdx)
	{
		const auto& ShapeCurve = CompletedShapes[ShapeIdx];
		const auto& ShapeFlags = CompletedBezierFlags[ShapeIdx];
		
		FShapeData SavedShape;
		

		// int32 i = 0;
		// for (const auto& Point : ShapeCurve.Points)
		// {
		for (int32 i = 0; i < ShapeCurve.Points.Num(); ++i)
		{
			const auto& Point = ShapeCurve.Points[i];

			FCurvePointData NewPoint;
			NewPoint.InputKey = Point.InVal;
			NewPoint.Position = Point.OutVal;
			NewPoint.ArriveTangent = Point.ArriveTangent;
			NewPoint.LeaveTangent = Point.LeaveTangent;
			NewPoint.bUseBezier   = ShapeFlags.IsValidIndex(i) 
									  ? ShapeFlags[i] 
									  : true;
			SavedShape.CompletedClothShape.Add(NewPoint);
			// ++i;
		}

		TargetAsset->ClothShapes.Add(SavedShape);
	}



	
	// Iterate over your FInterpCurve keys (points)
	for (int32 i = 0; i < CurvePoints.Points.Num(); ++i)
	{
		const FInterpCurvePoint<FVector2D>& Point = CurvePoints.Points[i];

		FCurvePointData NewPoint;
		NewPoint.InputKey = Point.InVal;
		NewPoint.Position = Point.OutVal;
		NewPoint.ArriveTangent = Point.ArriveTangent;
		NewPoint.LeaveTangent = Point.LeaveTangent;
		NewPoint.bUseBezier = bUseBezierPerPoint.IsValidIndex(i) ? bUseBezierPerPoint[i] : true;


		// Add to the array
		TargetAsset->ClothCurvePoints.Add(NewPoint);
	}

	
	// Mark dirty and notify asset registry only once
	// TargetAsset->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(TargetAsset);
	

	// Save package to disk
	FString PackageFileName = FPackageName::LongPackageNameToFilename(SanitizedPackageName, FPackageName::GetAssetPackageExtension());
	
	//bool bSaved = UPackage::SavePackage(Package, TargetAsset, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName);
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags   = RF_Public | RF_Standalone;
	SaveArgs.Error           = GError;
	SaveArgs.bWarnOfLongFilename = false;

	bool bSaved = UPackage::SavePackage(
		Package,                  // UPackage* InOuter
		TargetAsset,              // UObject* Base
		*PackageFileName,         // const TCHAR* Filename
		SaveArgs                  // const FSavePackageArgs& Args
	);
	
	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("Asset saved successfully: %s"), *PackageFileName);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save asset: %s"), *PackageFileName);
		return false;
	}
}




bool FCanvasAssets::LoadCanvasState(UClothShapeAsset* ClothAsset, FCanvasState& OutState)
{
    if (!ClothAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid shape asset to load."));
        return false;
    }

    // Start with a fresh state
    OutState = FCanvasState(); 

    // 1) Completed shapes
    for (int32 ShapeIdx = 0; ShapeIdx < ClothAsset->ClothShapes.Num(); ++ShapeIdx)
    {
        const FShapeData& SavedShape = ClothAsset->ClothShapes[ShapeIdx];

        FInterpCurve<FVector2D> Curve;
        TArray<bool> BezierFlags;
        Curve.Points.Reserve(SavedShape.CompletedClothShape.Num());
        BezierFlags.Reserve(SavedShape.CompletedClothShape.Num());

        for (const FCurvePointData& P : SavedShape.CompletedClothShape)
        {
            FInterpCurvePoint<FVector2D> Pt;
            Pt.InVal        = P.InputKey;
            Pt.OutVal       = P.Position;
            Pt.ArriveTangent= P.ArriveTangent;
            Pt.LeaveTangent = P.LeaveTangent;
            Pt.InterpMode   = CIM_CurveAuto;

            Curve.Points.Add(Pt);
            BezierFlags.Add(P.bUseBezier);
        }

        if (Curve.Points.Num() > 0)
        {
            OutState.CompletedShapes.Add(MoveTemp(Curve));
            OutState.CompletedBezierFlags.Add(MoveTemp(BezierFlags));
        }
    }

    // 2) Current (in-progress) shape
    {
        FInterpCurve<FVector2D> Curve;
        Curve.Points.Reserve(ClothAsset->ClothCurvePoints.Num());
        OutState.bUseBezierPerPoint.Reserve(ClothAsset->ClothCurvePoints.Num());

        for (const FCurvePointData& P : ClothAsset->ClothCurvePoints)
        {
            FInterpCurvePoint<FVector2D> Pt;
            Pt.InVal        = P.InputKey;
            Pt.OutVal       = P.Position;
            Pt.ArriveTangent= P.ArriveTangent;
            Pt.LeaveTangent = P.LeaveTangent;
            Pt.InterpMode   = CIM_CurveAuto;

            Curve.Points.Add(Pt);
            OutState.bUseBezierPerPoint.Add(P.bUseBezier);
        }

        OutState.CurvePoints       = MoveTemp(Curve);
    }

    // reset selection / pan / zoom to defaults
    OutState.SelectedPointIndex = INDEX_NONE;
    OutState.PanOffset          = FVector2D::ZeroVector;
    OutState.ZoomFactor         = 5.0f;

    return true;
}


// manage the assets here

FString FCanvasAssetManager::GetSelectedShapeAssetPath() const
{
	return ClothAsset.IsValid() ? ClothAsset->GetPathName() : FString();
}

// Moved from OnShapeAssetSelected()
bool FCanvasAssetManager::OnShapeAssetSelected(const FAssetData& AssetData, FCanvasState& OutState)
{
	ClothAsset = Cast<UClothShapeAsset>(AssetData.GetAsset());
	if (!ClothAsset.IsValid())
		return false;

	UE_LOG(LogTemp, Log, TEXT("Selected shape: %s"), *ClothAsset->GetName());

	// Moved from LoadShapeAssetData()
	return LoadShapeAssetData(OutState);
}

bool FCanvasAssetManager::LoadShapeAssetData(FCanvasState& OutState) const
{
	if (!ClothAsset.IsValid())
		return false;
	
	return FCanvasAssets::LoadCanvasState(ClothAsset.Get(), OutState);
}

