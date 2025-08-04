
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"

// Required for UCLASS to work:
#include "ClothPatternMeshActor.generated.h"


UCLASS()
class AClothPatternMeshActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cloth Mesh")
	UProceduralMeshComponent* MeshComponent;

	AClothPatternMeshActor();

	// Store the seam vertex IDs you recorded when building
	UPROPERTY()
	TArray<int32> LastSeamVertexIDs;

	// Keep the entire dynamic mesh around so we can query vertex positions
	// at alignment time.
	UE::Geometry::FDynamicMesh3 DynamicMesh;

	
};

