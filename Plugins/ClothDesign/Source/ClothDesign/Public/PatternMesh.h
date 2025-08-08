
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"

// Required for UCLASS to work:
#include "PatternMesh.generated.h"


UCLASS()
class APatternMesh : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cloth Mesh")
	UProceduralMeshComponent* MeshComponent;

	APatternMesh();

	// Store the seam vertex IDs you recorded when building
	UPROPERTY()
	TArray<int32> LastSeamVertexIDs;

	// Keep the entire dynamic mesh around so we can query vertex positions
	// at alignment time.
	UE::Geometry::FDynamicMesh3 DynamicMesh;

	
};

