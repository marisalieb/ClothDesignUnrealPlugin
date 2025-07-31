#pragma once

#include <random>


#include "Widgets/SCompoundWidget.h"
#include "CompGeom/PolygonTriangulation.h"
#include "ProceduralMeshComponent.h"
// #include "Math/InterpCurve.h"

// using namespace UE::Geometry;
#include "VectorTypes.h"
#include "Math/InterpCurve.h"
// #include "Math/Vector2D.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshEditor.h"
#include "MeshRegionBoundaryLoops.h"

#include "PatternSewingConstraint.h"
#include "AClothPatternMeshActor.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshEditor.h"

#include "CleaningOps/RemeshMeshOp.h"
#include "MeshOpPreviewHelpers.h" 
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshEditor.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "MeshDescriptionAdapter.h"
#include "Curve/DynamicGraph.h"
#include "DynamicMesh/InfoTypes.h"
#include "DynamicMesh/DynamicMesh3.h"         // for FDynamicMesh3, FEdgeSplitInfo, etc.
#include "DynamicMeshEditor.h"
#include "Curve/DynamicGraph.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "ConstrainedDelaunay2.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Spatial/PointHashGrid2.h" // optional for poisson
#include "Misc/ScopeLock.h"
#include "GeomTools.h"


// struct FCurvePoint
// {
// 	FInterpCurvePoint<FVector2D> Point;
// 	bool bUseBezierHandle = true;  // default to Bezier
//
// 	// convenience
// 	FVector2D& OutVal()        { return Point.OutVal; }
// 	FVector2D& ArriveTangent() { return Point.ArriveTangent; }
// 	FVector2D& LeaveTangent()  { return Point.LeaveTangent; }
// };
//
// struct FCurve
// {
// 	TArray<FCurvePoint> Points;
// 	float AutoSetTangentStrength = 0.5f;
// 	void AutoSetTangents()
// 	{
// 		// your existing AutoSetTangents logic, but operate on FMyCurvePoint::Point
// 	}
// 	FVector2D Eval(float InVal) const
// 	{
// 		// replicate Shape.Eval(...) by building a temporary FInterpCurve from the .Point members
// 	}
// };
//
// struct FCurveWithFlags
// {
// 	FInterpCurve<FVector2D>   Curve;              // your existing curve
// 	TArray<bool> bUseBezierHandle;   // same length as Curve.Points
//
// 	void AddPoint(const FInterpCurvePoint<FVector2D>& NewPt, bool bBezier)
// 	{
// 		Curve.Points.Add(NewPt);
// 		bUseBezierHandle.Add(bBezier);
// 	}
// };

// Canvas state struct
struct FCanvasState
{
	// TArray<FVector2D> Points;
	// int32 SelectedPointIndex = INDEX_NONE;
	// FVector2D PanOffset = FVector2D::ZeroVector;
	// float ZoomFactor = 1.0f;
	FInterpCurve<FVector2D> CurvePoints;  // Was: TArray<FVector2D> Points
	TArray<FInterpCurve<FVector2D>> CompletedShapes;
	// FCurveWithFlags CurvePoints;
	// TArray<FCurveWithFlags> CompletedShapes; // !!


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
class SClothDesignCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClothDesignCanvas) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
	virtual bool SupportsKeyboardFocus() const override { return true; }

	// --- Mouse handling ---
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// --- Drawing logic ---
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
						  const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
						  int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// Trigger mesh generation from drawn shape
	//void TriangulateAndBuildMesh(const FInterpCurve<FVector2D>& Shape);

	TArray<int32> LastSeamVertexIDs;  // filled each time you build a mesh

	void TriangulateAndBuildMesh(const FInterpCurve<FVector2D>& Shape, bool bRecordSeam = false,
								 int32 StartPointIdx2D = -1, int32 EndPointIdx2D = -1);

	
	
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
	void TriangulateAndBuildAllMeshes();


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


	
protected:
	void CreateProceduralMesh(const TArray<FVector>& Vertices, const TArray<int32>& Indices);

	// Undo/Redo stacks
	TArray<FCanvasState> UndoStack;
	TArray<FCanvasState> RedoStack;

	// State management
	void SaveStateForUndo();
	FCanvasState GetCurrentCanvasState() const;
	void RestoreCanvasState(const FCanvasState& State);

	void Undo();
	void Redo();

	
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


};