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


struct FCanvasSewing
{
	// State
	TArray<FPatternSewingConstraint> SewingConstraints;
	TArray<FPatternSewingConstraint> AllDefinedSeams;

	ESeamClickState SeamClickState = ESeamClickState::None;

	FClickTarget AStartTarget, AEndTarget, BStartTarget, BEndTarget;
	
	TArray<TWeakObjectPtr<APatternMesh>> SpawnedPatternActors;

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
	// void Reset()
	// {
	// 	SewingConstraints.Empty();
	// 	AllDefinedSeams.Empty();
	// 	SeamClickState = ESeamClickState::None;
	// 	AStartTarget = FClickTarget();
	// 	AEndTarget = FClickTarget();
	// 	BStartTarget = FClickTarget();
	// 	BEndTarget = FClickTarget();
	// }
};
