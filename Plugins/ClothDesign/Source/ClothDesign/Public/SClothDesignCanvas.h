#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Math/InterpCurve.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "PatternSewingConstraint.h"
#include "ClothPatternMeshActor.h"
#include "Misc/ScopeLock.h"
#include "ClothShapeAsset.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "MeshOpPreviewHelpers.h" 
#include "Canvas/CanvasAssets.h"


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

	// part of paint
	void RecalculateNTangents(
		FInterpCurve<FVector2D>& Curve,
		const TArray<bool>&      bBezierFlags);


	


	
	// create mesh from 2d points to 3d procedural mesh
	TArray<int32> LastSeamVertexIDs;  // filled each time you build a mesh

	static bool IsPointInPolygon(const FVector2f& Test, const TArray<FVector2f>& Poly);

	void TriangulateAndBuildMesh(const FInterpCurve<FVector2D>& Shape, bool bRecordSeam = false,
								 int32 StartPointIdx2D = -1, int32 EndPointIdx2D = -1);

	void CreateProceduralMesh(const TArray<FVector>& Vertices, const TArray<int32>& Indices);

	void TriangulateAndBuildAllMeshes();

	UE::Geometry::FDynamicMesh3 LastBuiltMesh;
	TArray<int32>  LastBuiltSeamVertexIDs;





	
	// pretty universal variables
	float ZoomFactor = 5.0f;
	FVector2D PanOffset = FVector2D::ZeroVector;
	mutable int32 SelectedPointIndex = INDEX_NONE;
	bool bIsDraggingPoint = false;
	bool bIsDraggingShape = false;
	bool bIsShapeSelected = false;
	mutable bool bIsDragging = false;
	FInterpCurve<FVector2D> CurvePoints;
	TArray<bool> bUseBezierPerPoint; 
	// for drawing multiple shapes
	TArray<FInterpCurve<FVector2D>> CompletedShapes;
	TArray<TArray<bool>> CompletedBezierFlags;
	mutable int32 SelectedShapeIndex = INDEX_NONE;
	bool bIsPanning = false;
	FVector2D LastMousePos = FVector2D::ZeroVector;
	bool bUseBezierPoints = true;
	bool bIsDraggingTangent = false;
	
	







	// sewing
	TArray<FPatternSewingConstraint> SewingConstraints;
	enum class ESeamClickState : uint8
	{
		None,
		ClickedAStart,
		ClickedAEnd,
		ClickedBStart,
		ClickedBEnd
	};
	ESeamClickState SeamClickState = ESeamClickState::None;
	TArray<FPatternSewingConstraint> AllDefinedSeams;
	struct FClickTarget {int32 ShapeIndex;int32 PointIndex;};
	FClickTarget AStartTarget, AEndTarget, BStartTarget, BEndTarget;

	
	void FinalizeSeamDefinitionByTargets(
		const FClickTarget& AStart,
		const FClickTarget& AEnd,
		const FClickTarget& BStart,
		const FClickTarget& BEnd);
	
	void AlignSeamMeshes(AClothPatternMeshActor* A, AClothPatternMeshActor* B);

	// triangluate two meshes + align seams
	void BuildAndAlignClickedSeam();

	// then merge those two separate seams
	void MergeLastTwoMeshes();
	// void MergeAndWeldLastTwoMeshes();
	
	// // COMING SEWING FEATURE!! 
	// // Represents one seam between two shapes
	// struct FSeamDefinition
	// {
	// 	int32 ShapeA, StartA, EndA;
	// 	int32 ShapeB, StartB, EndB;
	// };
	// TArray<FSeamDefinition> AllSeams;
	
	// void AddSewingConstraints(
	// 	AActor* PatternPiece1,
	// 	const TArray<int32>& Vertices1,
	// 	AActor* PatternPiece2,
	// 	const TArray<int32>& Vertices2,
	// 	float Stiffness,
	// 	TArray<FPatternSewingConstraint>& OutConstraints
	// );









	
	// save and load of shapes
	FString GetSelectedShapeAssetPath() const;
	void OnShapeAssetSelected(const FAssetData& AssetData);
	TWeakObjectPtr<UClothShapeAsset> ClothAsset;
	void LoadShapeAssetData();
	void AddPointToCanvas(const FCurvePointData& Point);
	void ClearCurrentShapeData();




	
	// simple getters used in the module class 
	const TArray<FInterpCurve<FVector2D>>& GetCompletedShapes() const { return CompletedShapes; }
	const TArray<TArray<bool>>& GetCompletedBezierFlags() const { return CompletedBezierFlags; }
	const FInterpCurve<FVector2D>& GetCurrentCurvePoints() const { return CurvePoints; }
	const TArray<bool>& GetCurrentBezierFlags() const { return bUseBezierPerPoint; }
	// ui getter for the mode, so draw, edit or sew
	EClothEditorMode GetCurrentMode() const { return CurrentMode; }

	
private:
	

	// member vars
	FGeometry LastGeometry;
	// triangluate and seams
	TArray<TWeakObjectPtr<AClothPatternMeshActor>> SpawnedPatternActors;



	
	// FVector2D ViewOffset = FVector2D::ZeroVector;
	

};