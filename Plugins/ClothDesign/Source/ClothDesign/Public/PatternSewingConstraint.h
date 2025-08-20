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
 * @brief Represents a sewing constraint between two procedural mesh vertices in the cloth pattern editor.
 * 
 * This struct stores references to two mesh components and their respective vertex indices that are constrained
 * together. Additionally, it keeps track of corresponding screen-space points for each vertex, useful for
 * visualization and interactive sewing operations in the UI.
 */
USTRUCT()
struct FPatternSewingConstraint
{
	GENERATED_BODY()

	/** Default constructor initializing pointers to null and indices to zero. */
	FPatternSewingConstraint()
		: MeshA(nullptr)
		, VertexIndexA(0)
		, MeshB(nullptr)
		, VertexIndexB(0)
	{}

	/** 
	 * @brief First mesh involved in the sewing constraint. 
	 * This should be a procedural mesh component whose vertex is constrained to another mesh's vertex.
	 */
	UPROPERTY()
	UProceduralMeshComponent* MeshA;

	/** 
	 * @brief Index of the vertex in MeshA that is constrained.
	 */
	UPROPERTY()
	int32 VertexIndexA;

	/** 
	 * @brief Second mesh involved in the sewing constraint.
	 * This is the mesh whose vertex is connected to MeshA's vertex.
	 */
	UPROPERTY()
	UProceduralMeshComponent* MeshB;

	/** 
	 * @brief Index of the vertex in MeshB that is constrained.
	 */
	UPROPERTY()
	int32 VertexIndexB;

	/**
	 * @brief Screen-space coordinates corresponding to the vertex in MeshA.
	 * Used primarily for visualization in the UI and for interactive sewing.
	 */
	UPROPERTY()
	TArray<FVector2D> ScreenPointsA;

	/**
	 * @brief Screen-space coordinates corresponding to the vertex in MeshB.
	 * Used primarily for visualization in the UI and for interactive sewing.
	 */
	UPROPERTY()
	TArray<FVector2D> ScreenPointsB;
};
