#pragma once
#include "PatternSewingConstraint.h"
#include "PatternMesh.h"


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

// // Represents one seam between two shapes
// struct FSeamDefinition
// {
// 	int32 ShapeA, StartA, EndA;
// 	int32 ShapeB, StartB, EndB;
// };

struct FCanvasSewing
{
	// State
	TArray<FPatternSewingConstraint> SewingConstraints;
	TArray<FPatternSewingConstraint> AllDefinedSeams;

	ESeamClickState SeamClickState = ESeamClickState::None;

	FClickTarget AStartTarget, AEndTarget, BStartTarget, BEndTarget;
	
	TArray<TWeakObjectPtr<APatternMesh>> SpawnedPatternActors;

	// TArray<FSeamDefinition> AllSeams;
	
	// Methods
	void FinalizeSeamDefinitionByTargets(
		const FClickTarget& AStart,
		const FClickTarget& AEnd,
		const FClickTarget& BStart,
		const FClickTarget& BEnd,
		const FInterpCurve<FVector2D>& CurvePoints,
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes);

	void AlignSeamMeshes(APatternMesh* A, APatternMesh* B);

	void BuildAndAlignClickedSeam(
		TArray<TWeakObjectPtr<APatternMesh>>& SpawnedPatternActors,
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
		const FInterpCurve<FVector2D>& CurvePoints);

	void MergeLastTwoMeshes();
	
	void ClearAllSeams();
};
