#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

#include "PatternSewingConstraint.generated.h"

USTRUCT()
struct FPatternSewingConstraint
{
	GENERATED_BODY()
	
	FPatternSewingConstraint():
		MeshA(nullptr),
		VertexIndexA(0),
		MeshB(nullptr),
		VertexIndexB(0)
		{}
	
	UPROPERTY()
	UProceduralMeshComponent* MeshA;

	UPROPERTY()
	int32 VertexIndexA;

	UPROPERTY()
	UProceduralMeshComponent* MeshB;

	UPROPERTY()
	int32 VertexIndexB;
	
	UPROPERTY()
	TArray<FVector2D> ScreenPointsA;

	UPROPERTY()
	TArray<FVector2D> ScreenPointsB;
};
