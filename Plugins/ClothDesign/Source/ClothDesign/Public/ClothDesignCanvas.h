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
#include "Canvas/CanvasAssets.h"
#include "Canvas/CanvasSewing.h"


class SClothDesignCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClothDesignCanvas) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
	virtual bool SupportsKeyboardFocus() const override { return true; }

	FVector2D TransformPoint(const FVector2D& Point) const;
	FVector2D InverseTransformPoint(const FVector2D& ScreenPoint) const;

	// // --- Drawing logic ---
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
							  const FSlateRect& CullingRect, FSlateWindowElementList& OutDrawElements,
							  int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	
	// --- Mouse handling ---
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;

	void FocusViewportOnPoints();
	
	enum class EClothEditorMode
	{
		Draw,
		Select,
		Move,
		Sew
	};
	EClothEditorMode CurrentMode = EClothEditorMode::Draw;
	FReply OnModeButtonClicked(EClothEditorMode NewMode);

	
	// Undo/Redo stacks
	TArray<FCanvasState> UndoStack;
	TArray<FCanvasState> RedoStack;
	FCanvasState GetCurrentCanvasState() const;
	void RestoreCanvasState(const FCanvasState& State);

	// for mousemove and mousebuttonup, onkeyup
	enum class ETangentHandle
	{
		None,
		Arrive,
		Leave
	};
	bool bSeparateTangents = false;
	ETangentHandle SelectedTangentHandle = ETangentHandle::None;
	

	// background texture
	TWeakObjectPtr<UTexture2D> BackgroundTexture;
	FString GetSelectedTexturePath() const;
	void OnBackgroundTextureSelected(const FAssetData& AssetData);
	float BackgroundImageScale = 1.0f;
	TOptional<float> GetBackgroundImageScale() const;
	void OnBackgroundImageScaleChanged(float NewScale);



	// save and load of shapes
	FString GetSelectedShapeAssetPath() const;
	void OnShapeAssetSelected(const FAssetData& AssetData);
	void ClearAllShapeData();

	FCanvasSewing SewingManager;
	void SewingClick();
	void MergeClick();
	void ClearAllSewing();
	void GenerateMeshesClick();
	FReply SaveClick(const FString& SaveName);

	
	// simple getters used in the module class 
	const TArray<FInterpCurve<FVector2D>>& GetCompletedShapes() const { return CompletedShapes; }
	const TArray<TArray<bool>>& GetCompletedBezierFlags() const { return CompletedBezierFlags; }
	const FInterpCurve<FVector2D>& GetCurrentCurvePoints() const { return CurvePoints; }
	const TArray<bool>& GetCurrentBezierFlags() const { return bUseBezierPerPoint; }
	// ui getter for the mode, so draw, edit or sew
	EClothEditorMode GetCurrentMode() const { return CurrentMode; }
	FCanvasSewing& GetSewingManager() { return SewingManager; }
	const FCanvasSewing& GetSewingManager() const { return SewingManager; }
	
	
	// pretty universal variables
	float ZoomFactor = 5.0f;
	FVector2D PanOffset = FVector2D::ZeroVector;
	mutable int32 SelectedPointIndex = INDEX_NONE;
	mutable int32 SelectedShapeIndex = INDEX_NONE;
	FVector2D LastMousePos = FVector2D::ZeroVector;
	bool bIsDraggingPoint = false;
	bool bIsDraggingShape = false;
	bool bIsDraggingTangent = false;
	bool bIsShapeSelected = false;
	mutable bool bIsDragging = false;
	bool bIsPanning = false;
	
	FInterpCurve<FVector2D> CurvePoints;
	TArray<FInterpCurve<FVector2D>> CompletedShapes;
	TArray<bool> bUseBezierPerPoint;
	TArray<TArray<bool>> CompletedBezierFlags;
	bool bUseBezierPoints = true;


private:
	FGeometry LastGeometry;
	TArray<TWeakObjectPtr<APatternMesh>> LastBuiltActors;
	
	// for save load managing 
	FCanvasAssetManager AssetManager;
};