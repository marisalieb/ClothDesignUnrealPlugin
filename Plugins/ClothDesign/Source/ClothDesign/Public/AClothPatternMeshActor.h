#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "AClothPatternMeshActor.generated.h" // Required for UCLASS processing by Unreal Header Tool

/**
 * @class AClothPatternMeshActor
 * @brief Actor that represents a cloth pattern mesh in the scene, using a procedural mesh component.
 *
 * This actor is designed to store and render dynamically generated cloth pattern geometry.
 * It keeps both the procedural mesh for rendering and a dynamic mesh for precise geometry queries.
 *
 * Responsibilities:
 * - Holds a procedural mesh for visual representation of the cloth.
 * - Retains seam vertex IDs used during mesh building.
 * - Stores a complete dynamic mesh for vertex-level operations such as alignment or simulation.
 *
 * Usage:
 * @code
 * AClothPatternMeshActor* PatternMeshActor = World->SpawnActor<AClothPatternMeshActor>();
 * PatternMeshActor->MeshComponent->CreateMeshSection(...);
 * @endcode
 */
UCLASS()
class AClothPatternMeshActor : public AActor
{
	GENERATED_BODY()

public:

	/**
	 * @brief Procedural mesh component used to render the cloth pattern geometry.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UProceduralMeshComponent* MeshComponent;

	/**
	 * @brief Default constructor.
	 *
	 * Initializes default subobjects and prepares the actor for procedural mesh creation.
	 */
	AClothPatternMeshActor();

	/**
	 * @brief Stores the seam vertex IDs recorded during mesh building.
	 *
	 * These IDs can be used to identify specific seam edges or vertices
	 * for sewing or alignment operations in later stages.
	 */
	UPROPERTY()
	TArray<int32> LastSeamVertexIDs;

	/**
	 * @brief Full dynamic mesh representation of the cloth pattern.
	 *
	 * Retained to allow direct vertex-level queries and manipulations,
	 * especially during alignment or simulation steps.
	 */
	UE::Geometry::FDynamicMesh3 DynamicMesh;
};
