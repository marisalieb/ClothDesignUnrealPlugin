#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

#include "PatternSewingConstraint.generated.h"

/*
 * Thesis reference:
 * See Chapter 4.7 and 4.8.1 for detailed explanations.
 */



/**
 * @struct FPatternSewingConstraint
 * @brief Represents a sewing constraint between two procedural mesh components.
 * 
 * This struct is used to define a sewing constraint between two meshes, including their vertex indices
 * and corresponding screen points for visualization or processing.
 */
USTRUCT()
struct FPatternSewingConstraint
{
	GENERATED_BODY()

	/**
	 * @brief Default constructor for FPatternSewingConstraint.
	 * 
	 * Initializes the mesh pointers to nullptr and vertex indices to 0.
	 */
	FPatternSewingConstraint()
		: MeshA(nullptr)
		, VertexIndexA(0)
		, MeshB(nullptr)
		, VertexIndexB(0)
	{}

	/** Pointer to the first procedural mesh component involved in the constraint. */
	UPROPERTY()
	UProceduralMeshComponent* MeshA;

	/** Vertex index on the first mesh component. */
	UPROPERTY()
	int32 VertexIndexA;

	/** Pointer to the second procedural mesh component involved in the constraint. */
	UPROPERTY()
	UProceduralMeshComponent* MeshB;

	/** Vertex index on the second mesh component. */
	UPROPERTY()
	int32 VertexIndexB;

	/** Screen-space points corresponding to the first mesh component. */
	UPROPERTY()
	TArray<FVector2D> ScreenPointsA;

	/** Screen-space points corresponding to the second mesh component. */
	UPROPERTY()
	TArray<FVector2D> ScreenPointsB;
};
