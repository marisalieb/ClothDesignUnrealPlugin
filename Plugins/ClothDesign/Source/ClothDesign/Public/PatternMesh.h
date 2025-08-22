
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
 * @class APatternMesh
 * @brief Represents a procedural mesh actor used for cloth simulation or pattern design.
 * 
 * This class encapsulates a procedural mesh component and provides functionality for managing
 * boundary samples, seam vertices, and dynamic mesh data.
 */
UCLASS()
class APatternMesh : public AActor
{
	GENERATED_BODY()

public:

	/** 
	 * @brief Procedural mesh component representing the cloth or pattern mesh.
	 * 
	 * This component is used to render and manipulate the mesh in the scene.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cloth Mesh")
	UProceduralMeshComponent* MeshComponent;

	/**
	 * @brief Default constructor for APatternMesh.
	 * 
	 * Initializes the procedural mesh component and other member variables.
	 */
	APatternMesh();

	/** 
	 * @brief Stores the vertex IDs of the last seam created on the mesh.
	 */
	UPROPERTY()
	TArray<int32> LastSeamVertexIDs;

	/** 
	 * @brief Stores 2D sample points along the boundary of the mesh.
	 * 
	 * These points are used for operations such as boundary processing or visualization.
	 */
	UPROPERTY()
	TArray<FVector2f> BoundarySamplePoints2D;

	/** 
	 * @brief Stores the vertex IDs corresponding to the boundary sample points.
	 */
	UPROPERTY()
	TArray<int32> BoundarySampleVertexIDs;

	/** 
	 * @brief Stores the world positions of the boundary sample points.
	 */
	UPROPERTY()
	TArray<FVector> BoundarySampleWorldPositions;

	/**
	 * @brief Sets the mapping from polygon indices to vertex IDs.
	 * 
	 * @param InMapping The mapping array to set.
	 */
	void SetPolyIndexToVID(const TArray<int32>& InMapping) { PolyIndexToVID = InMapping; }

	/**
	 * @brief Retrieves the mapping from polygon indices to vertex IDs.
	 * 
	 * @return A reference to the mapping array.
	 */
	const TArray<int32>& GetPolyIndexToVID() const { return PolyIndexToVID; }

	/** 
	 * @brief The dynamic mesh data structure used for mesh operations.
	 * 
	 * This is part of the Unreal Engine Geometry Framework.
	 */
	UE::Geometry::FDynamicMesh3 DynamicMesh;

private:

	/** 
	 * @brief Stores the mapping from polygon indices to vertex IDs.
	 * 
	 * This is used internally for managing mesh data.
	 */
	UPROPERTY()
	TArray<int32> PolyIndexToVID;
};


