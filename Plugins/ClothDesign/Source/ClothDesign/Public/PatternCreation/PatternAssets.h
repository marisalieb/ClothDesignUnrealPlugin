#ifndef FPatternAssets_H
#define FPatternAssets_H

#include "ClothShapeAsset.h"
#include "Canvas/CanvasState.h" 

// /*
//  * Thesis reference:
//  * See Chapter 4.9.1 for detailed explanations.
//  */



/**
 * @brief Stores all data needed to reconstruct a previously completed shape.
 * 
 * This struct exists to encapsulate both the geometric points and the Bezier flags
 * for completed shapes, enabling saving, loading, and manipulation of user-created shapes.
 */
struct FLoadedShapeData
{
    TArray<FInterpCurve<FVector2D>> CompletedShapes; /**< Stores each completed shape's curve data. */
    TArray<TArray<bool>> CompletedBezierFlags;      /**< Tracks which points in each shape use Bezier handles. */
    FInterpCurve<FVector2D> CurvePoints;           /**< Stores the current working curve points. */
    TArray<bool> bUseBezierPerPoint;               /**< Indicates Bezier usage per point for the working curve. */
};

/**
 * @brief Provides static functions to save and load canvas-related assets.
 * 
 * This class centralises asset I/O to ensure canvas state persistence is handled consistently,
 * supporting undoable workflows and reproducible designs.
 */
class FPatternAssets
{
public:
    /**
     * @brief Saves the given shape data as an asset to disk.
     * @param AssetPath Path to save the asset at.
     * @param AssetName Name of the asset to create.
     * @param CompletedShapes Curves representing completed shapes.
     * @param CompletedBezierFlags Bezier flags for each completed shape.
     * @param CurvePoints Current working curve points.
     * @param bUseBezierPerPoint Bezier usage flags for the current curve.
     * @return True if the asset was successfully saved, false otherwise.
     * 
     * Saving assets allows designs to be persisted beyond the current session,
     * supporting collaboration and reuse.
     */
    static bool SaveShapeAsset(
        const FString& AssetPath,
        const FString& AssetName,
        const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
        const TArray<TArray<bool>>& CompletedBezierFlags,
        const FInterpCurve<FVector2D>& CurvePoints,
        const TArray<bool>& bUseBezierPerPoint
    );

    /**
     * @brief Loads the canvas state from a given cloth asset.
     * @param ClothAsset The asset to load from.
     * @param OutState Canvas state to populate with asset data.
     * @return True if the asset was successfully loaded, false otherwise.
     * 
     * This method abstracts asset loading to ensure canvas state can be reliably restored,
     * keeping user progress and selections intact.
     */
    static bool LoadCanvasState(
        UClothShapeAsset* ClothAsset,
        FCanvasState& OutState
    );
};

/**
 * @brief Manages the currently selected shape asset and handles its state loading.
 * 
 * This class provides high-level control over which asset is active in the canvas,
 * ensuring that selection and loading are consistent with user expectations.
 */
class FPatternAssetManager
{
public:
    TWeakObjectPtr<UClothShapeAsset> ClothAsset; /**< Currently selected shape asset. */

    /**
     * @brief Returns the file path of the currently selected shape asset.
     * @return Path of the selected asset as a string.
     * 
     * Useful for displaying asset information or resolving asset references.
     */
    FString GetSelectedShapeAssetPath() const;

    /**
     * @brief Updates the currently selected shape asset based on user selection.
     * @param AssetData Data for the asset selected by the user.
     * @param OutState Canvas state to populate after loading.
     * @return True if the asset was successfully selected and loaded, false otherwise.
     * 
     * This ensures the canvas reflects the user's chosen asset and preserves prior state where possible.
     */
    bool OnShapeAssetSelected(const FAssetData& AssetData, FCanvasState& OutState);

private:
    /**
     * @brief Internal function to load shape asset data into the canvas state.
     * @param OutState Canvas state to populate.
     * @return True if the shape data was successfully loaded, false otherwise.
     * 
     * Encapsulates the low-level loading details to maintain separation of concerns and keep public methods simple.
     */
    bool LoadShapeAssetData(FCanvasState& OutState) const;
};


#endif

