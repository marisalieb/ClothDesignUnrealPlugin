#pragma once


#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

#include "PatternSewingConstraint.generated.h"

USTRUCT()
struct FPatternSewingConstraint
{
	GENERATED_BODY()

	UPROPERTY()
	UProceduralMeshComponent* MeshA;

	UPROPERTY()
	int32 VertexIndexA;

	UPROPERTY()
	UProceduralMeshComponent* MeshB;

	UPROPERTY()
	int32 VertexIndexB;

	// UPROPERTY()
	// float Stiffness;

	
	UPROPERTY()
	TArray<FVector2D> ScreenPointsA;

	UPROPERTY()
	TArray<FVector2D> ScreenPointsB;
};
