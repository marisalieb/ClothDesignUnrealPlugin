#include "ClothPatternMeshActor.h"


AClothPatternMeshActor::AClothPatternMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = MeshComponent;
}