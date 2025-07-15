
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

// Required for UCLASS to work:
#include "AClothPatternMeshActor.generated.h" // necessary because UE's UCLASS relies on the UHT


UCLASS()
class AClothPatternMeshActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UProceduralMeshComponent* MeshComponent;

	AClothPatternMeshActor();
};

