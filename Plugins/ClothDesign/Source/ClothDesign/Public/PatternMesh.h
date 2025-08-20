
#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"

// Required for UCLASS to work:
#include "PatternMesh.generated.h"

/*
 * Thesis reference:
 * See Chapter 4.6 for detailed explanations.
 */


/**
 * @brief Represents a single cloth pattern mesh actor in the scene.
 * 
 * This actor encapsulates a procedural mesh component along with metadata for
 * sewing, boundary sampling, and vertex alignment. It maintains both 2D pattern
 * information and corresponding 3D mesh data, allowing seamless interaction
 * between the design canvas and the runtime mesh.
 */
UCLASS()
class APatternMesh : public AActor
{
	GENERATED_BODY()

public:

	/** 
	 * @brief The main procedural mesh component for this pattern mesh.
	 * Exposed as read-only in Blueprints for visualization and runtime operations.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cloth Mesh")
	UProceduralMeshComponent* MeshComponent;

	/** Default constructor. Initializes internal state and components. */
	APatternMesh();

	/**
	 * @brief Stores the vertex IDs used in the last sewing operation.
	 * Useful for reconstructing seams or reapplying constraints.
	 */
	UPROPERTY()
	TArray<int32> LastSeamVertexIDs;

	/**
	 * @brief Sampled points along the 2D boundary of the cloth pattern.
	 * The order of points is preserved to maintain polygon topology.
	 */
	UPROPERTY()
	TArray<FVector2f> BoundarySamplePoints2D;

	/**
	 * @brief Corresponding vertex IDs in the dynamic 3D mesh for each sampled 2D boundary point.
	 */
	UPROPERTY()
	TArray<int32> BoundarySampleVertexIDs;

	/**
	 * @brief Cached world-space positions of the boundary vertices.
	 * Speeds up alignment and visualization without recalculating transforms.
	 */
	UPROPERTY()
	TArray<FVector> BoundarySampleWorldPositions;

	/**
	 * @brief Set the mapping from 2D polygon indices to 3D vertex IDs.
	 * @param InMapping Array mapping polygon index to vertex ID.
	 */
	void SetPolyIndexToVID(const TArray<int32>& InMapping) { PolyIndexToVID = InMapping; }

	/**
	 * @brief Get the current mapping from 2D polygon indices to 3D vertex IDs.
	 * @return Reference to the internal PolyIndexToVID array.
	 */
	const TArray<int32>& GetPolyIndexToVID() const { return PolyIndexToVID; }

	/**
	 * @brief Full dynamic mesh data associated with this pattern.
	 * Maintained for alignment, sewing, and vertex queries at runtime.
	 */
	UE::Geometry::FDynamicMesh3 DynamicMesh;

private:

	/**
	 * @brief Maps polygon indices from 2D pattern space to 3D vertex IDs in DynamicMesh.
	 * This mapping allows quick lookups when constructing boundaries and seams.
	 */
	UPROPERTY()
	TArray<int32> PolyIndexToVID;
};


