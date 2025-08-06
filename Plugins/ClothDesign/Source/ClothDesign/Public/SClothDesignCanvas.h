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
#include "CanvasUtils.h"
#include "CanvasAssets.h"


class SClothDesignCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClothDesignCanvas) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
	virtual bool SupportsKeyboardFocus() const override { return true; }

	// --- Mouse handling ---
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// // --- Drawing logic ---
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
							  const FSlateRect& CullingRect, FSlateWindowElementList& OutDrawElements,
							  int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	TArray<int32> LastSeamVertexIDs;  // filled each time you build a mesh

	void TriangulateAndBuildMesh(const FInterpCurve<FVector2D>& Shape, bool bRecordSeam = false,
								 int32 StartPointIdx2D = -1, int32 EndPointIdx2D = -1);

	void CreateProceduralMesh(const TArray<FVector>& Vertices, const TArray<int32>& Indices);

	void TriangulateAndBuildAllMeshes();


	
	float ZoomFactor = 5.0f;
	FVector2D PanOffset = FVector2D::ZeroVector;
	
	FVector2D TransformPoint(const FVector2D& Point) const;
	
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// for selectable point and movement
	mutable int32 SelectedPointIndex = INDEX_NONE;
	mutable bool bIsDragging = false;
	
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// need different modes so button down isnt always just adding new points
	enum class EClothEditorMode
	{
		Draw,
		Select,
		Move,
		Sew
	};

	USkeletalMeshComponent* SkeletalMesh = nullptr;
	
	EClothEditorMode CurrentMode = EClothEditorMode::Draw;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;

	FVector2D InverseTransformPoint(const FVector2D& ScreenPoint) const;

	// Add some member state variables
	bool bIsDraggingPoint = false;
	bool bIsDraggingShape = false;
	bool bIsShapeSelected = false;
	bool IsPointNearLine(const FVector2D& P, const FVector2D& A, const FVector2D& B, float Threshold) const;


	TWeakObjectPtr<UTexture2D> BackgroundTexture;

	FString GetSelectedTexturePath() const;
	void OnBackgroundTextureSelected(const FAssetData& AssetData);
	float BackgroundImageScale = 1.0f;

	
	TOptional<float> GetBackgroundImageScale() const;
	void OnBackgroundImageScaleChanged(float NewScale);


	// This holds your curve control points and interpolation mode
	FInterpCurve<FVector2D> CurvePoints;
	//FCurveWithFlags CurvePoints; // !!
	TArray<bool> bUseBezierPerPoint; 


	enum class ETangentHandle
	{
		None,
		Arrive,
		Leave
	};

	// for bezier handles, toggle
	bool bSeparateTangents = false;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	void FocusViewportOnPoints();

	// for drawing multiple shapes
	TArray<FInterpCurve<FVector2D>> CompletedShapes;
	//TArray<FCurveWithFlags> CompletedShapes; // !!
	TArray<TArray<bool>> CompletedBezierFlags;


	mutable int32 SelectedShapeIndex = INDEX_NONE;



	// sewing
	TArray<FPatternSewingConstraint> SewingConstraints;

	void AddSewingConstraints(
		AActor* PatternPiece1,
		const TArray<int32>& Vertices1,
		AActor* PatternPiece2,
		const TArray<int32>& Vertices2,
		float Stiffness,
		TArray<FPatternSewingConstraint>& OutConstraints
	);

	void SewingTest();
	void ApplySewingConstraints();

	FVector GetSkinnedVertexPosition(USkeletalMeshComponent* MeshComp, int32 VertexIndex);

	void SewingStart();
	void ApplySpringSeamForces(const TArray<FPatternSewingConstraint>& Constraints, float DeltaTime);



	// sew
	enum class ESeamClickState : uint8
	{
		None,
		ClickedAStart,
		ClickedAEnd,
		ClickedBStart,
		ClickedBEnd
	};

	ESeamClickState SeamClickState = ESeamClickState::None;
	FVector2D AStart, AEnd, BStart, BEnd;
	void FinalizeSeamDefinition(
		const FVector2D& AStart,
		const FVector2D& AEnd,
		const FVector2D& BStart,
		const FVector2D& BEnd);
	
	TArray<FPatternSewingConstraint> AllDefinedSeams;
	bool bIsSeamReady = false;

	// In your .h
	int32 SeamAStartIndex  = INDEX_NONE;
	int32 SeamAEndIndex    = INDEX_NONE;
	int32 SeamBStartIndex  = INDEX_NONE;
	int32 SeamBEndIndex    = INDEX_NONE;


	
	struct FClickTarget {
		int32 ShapeIndex;
		int32 PointIndex;
	};
	
	FClickTarget AStartTarget, AEndTarget, BStartTarget, BEndTarget;
	FVector2D GetPointFromShape(int32 ShapeIndex, int32 PointIndex);
	
	void FinalizeSeamDefinitionByIndex(FClickTarget AStart, FClickTarget AEnd, FClickTarget BStart, FClickTarget BEnd);

	void FinalizeSeamDefinitionByTargets(
		const FClickTarget& AStart,
		const FClickTarget& AEnd,
		const FClickTarget& BStart,
		const FClickTarget& BEnd);

	// void SClothDesignCanvas::AlignSeamMeshes(
	// 	AClothPatternMeshActor* MeshActorA,
	// 	AClothPatternMeshActor* MeshActorB
	// );
	void AlignSeamMeshes(AClothPatternMeshActor* A, AClothPatternMeshActor* B);

	//UE::Geometry::FDynamicMesh3 DynamicMesh;

	void BuildAndAlignClickedSeam();

	void MergeLastTwoMeshes();
	void MergeAndWeldLastTwoMeshes();

	// Represents one seam between two shapes
	struct FSeamDefinition
	{
		int32 ShapeA, StartA, EndA;
		int32 ShapeB, StartB, EndB;
	};

	TArray<FSeamDefinition> AllSeams;






	void RecalculateNTangents(
		FInterpCurve<FVector2D>& Curve,
		const TArray<bool>&      bBezierFlags);

	FReply OnModeButtonClicked(EClothEditorMode NewMode);
	EClothEditorMode GetCurrentMode() const { return CurrentMode; }


	bool SaveShapeAsset(const FString& AssetPath, const FString& AssetName);

	FString GetSelectedShapeAssetPath() const;
	void OnShapeAssetSelected(const FAssetData& AssetData);
	TWeakObjectPtr<UClothShapeAsset> ClothAsset;
	void LoadShapeAssetData();
	void AddPointToCanvas(const FCurvePointData& Point);
	void ClearCurrentShapeData();
	void ClearCurvePointArrays();

	const TArray<FInterpCurve<FVector2D>>& GetCompletedShapes() const { return CompletedShapes; }
	const TArray<TArray<bool>>& GetCompletedBezierFlags() const { return CompletedBezierFlags; }
	const FInterpCurve<FVector2D>& GetCurrentCurvePoints() const { return CurvePoints; }
	const TArray<bool>& GetCurrentBezierFlags() const { return bUseBezierPerPoint; }


	
	
protected:

	// Undo/Redo stacks
	TArray<FCanvasState> UndoStack;
	TArray<FCanvasState> RedoStack;

	// State management
	// void SaveStateForUndo();
	FCanvasState GetCurrentCanvasState() const;
	void RestoreCanvasState(const FCanvasState& State);

	// void Undo();
	// void Redo();

	
	/** All the procedural mesh actors we’ve spawned for patterns */
	TArray<TWeakObjectPtr<AClothPatternMeshActor>> SpawnedPatternActors;
	UE::Geometry::FDynamicMesh3 LastBuiltMesh;
	TArray<int32>  LastBuiltSeamVertexIDs;
	
private:
	TArray<FVector2D> Points;

	ETangentHandle SelectedTangentHandle = ETangentHandle::None;
	bool bIsDraggingTangent = false;
	float TangentHandleRadius = 25.0f; // or whatever pixel radius

	// Pan and zoom state for focus
	// FVector2D ViewOffset = FVector2D::ZeroVector;
	// Optional: store last geometry for sizing
	FGeometry LastGeometry;

	bool bUseBezierPoints = true;

	FVector2D ViewOffset = FVector2D::ZeroVector;
	bool bIsPanning = false;
	FVector2D LastMousePos = FVector2D::ZeroVector;


	static bool IsPointInPolygon(
	const FVector2f& Test, 
	const TArray<FVector2f>& Poly);

};