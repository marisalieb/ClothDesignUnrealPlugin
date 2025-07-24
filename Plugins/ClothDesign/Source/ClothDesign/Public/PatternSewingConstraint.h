#pragma once


#include "CoreMinimal.h"
#include "PatternSewingConstraint.generated.h"

USTRUCT()
struct FPatternSewingConstraint
{
	GENERATED_BODY()

	UPROPERTY()
	USkeletalMeshComponent* MeshA;

	UPROPERTY()
	int32 VertexIndexA;

	UPROPERTY()
	USkeletalMeshComponent* MeshB;

	UPROPERTY()
	int32 VertexIndexB;

	UPROPERTY()
	float Stiffness;
	
};
