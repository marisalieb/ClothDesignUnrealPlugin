#pragma once

#include "CoreMinimal.h"
#include "PatternSewingConstraint.generated.h"

/**
 * @struct FPatternSewingConstraint
 * @brief Represents a constraint between two points (vertices) on separate skeletal meshes
 *        used in pattern sewing or cloth simulation.
 *
 * This structure defines a link between a vertex on one skeletal mesh and a vertex
 * on another skeletal mesh, including the physical properties of the constraint
 * and optional screen-space representation data. It can be used for stitching
 * virtual fabric together in simulation or visualization contexts.
 */

USTRUCT()
struct FPatternSewingConstraint
{
	GENERATED_BODY()

	/** 
	 * @brief Pointer to the first skeletal mesh component participating in the constraint.
	 */
	UPROPERTY()
	USkeletalMeshComponent* MeshA;

	/** 
	 * @brief Index of the vertex on the first skeletal mesh that is part of the constraint.
	 */
	UPROPERTY()
	int32 VertexIndexA;

	/** 
	 * @brief Pointer to the second skeletal mesh component participating in the constraint.
	 */
	UPROPERTY()
	USkeletalMeshComponent* MeshB;

	/** 
	 * @brief Index of the vertex on the second skeletal mesh that is part of the constraint.
	 */
	UPROPERTY()
	int32 VertexIndexB;

	/** 
	 * @brief Stiffness of the sewing constraint, controlling how strongly the two vertices are held together.
	 * A higher value means the constraint resists stretching more.
	 */
	UPROPERTY()
	float Stiffness;

	/** 
	 * @brief Screen-space points representing the position of the first mesh's vertices for visualization or debugging.
	 */
	UPROPERTY()
	TArray<FVector2D> ScreenPointsA;

	/** 
	 * @brief Screen-space points representing the position of the second mesh's vertices for visualization or debugging.
	 */
	UPROPERTY()
	TArray<FVector2D> ScreenPointsB;
};
