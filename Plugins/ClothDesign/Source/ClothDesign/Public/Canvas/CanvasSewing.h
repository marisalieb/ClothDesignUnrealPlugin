#pragma once
#include "PatternSewingConstraint.h"
#include "PatternMesh.h"

// // Represents one seam between two shapes
// struct FSeamDefinition
// {
// 	int32 ShapeA, StartA, EndA;
// 	int32 ShapeB, StartB, EndB;
// };

struct FEdgeIndices
{
	int32 Start;
	int32 End;
};

struct FSeamDefinition
{
	int32 ShapeA;
	FEdgeIndices EdgeA;
	int32 ShapeB;
	FEdgeIndices EdgeB;
};

struct FClickTarget
{
	int32 ShapeIndex = INDEX_NONE;
	int32 PointIndex = INDEX_NONE;
};

enum class ESeamClickState : uint8
{
	None,
	ClickedAStart,
	ClickedAEnd,
	ClickedBStart,
	ClickedBEnd
};


struct FCanvasSewing
{
	// State
	TArray<FSeamDefinition> SeamDefinitions;
	// TArray<FPatternSewingConstraint> SewingConstraints;
	TArray<FPatternSewingConstraint> AllDefinedSeams;

	ESeamClickState SeamClickState = ESeamClickState::None;


	FClickTarget AStartTarget, AEndTarget, BStartTarget, BEndTarget;
	
	TArray<TWeakObjectPtr<APatternMesh>> SpawnedPatternActors;

	// TArray<FSeamDefinition> AllSeams;
	
	// Methods
	void FinaliseSeamDefinitionByTargets(
		const FClickTarget& AStart,
		const FClickTarget& AEnd,
		const FClickTarget& BStart,
		const FClickTarget& BEnd,
		const FInterpCurve<FVector2D>& CurvePoints,
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
		const TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors);

	void AlignSeamMeshes(APatternMesh* A, APatternMesh* B);

	// void BuildAndAlignClickedSeam(
	// 	TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors,
	// 	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	// 	const FInterpCurve<FVector2D>& CurvePoints);
	// void BuildAndAlignClickedSeam(
	// 	const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
	// 	const FInterpCurve<FVector2D>& CurvePoints);
	
	void BuildAndAlignSeam(
		const FPatternSewingConstraint& Seam,
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
		const FInterpCurve<FVector2D>& CurvePoints);
	
	void BuildAndAlignAllSeams(
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
		const FInterpCurve<FVector2D>& CurvePoints);

	
	// void MergeLastTwoMeshes();
	
	void ClearAllSeams();

	void MergeSewnGroups();


	// APatternMesh* GetPatternActor(int32 ShapeIndex);
	// int32 GetVertexID(int32 ShapeIndex, int32 PointIndex);

};
