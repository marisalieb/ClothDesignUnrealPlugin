#include "PatternMesh.h"


APatternMesh::APatternMesh()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = MeshComponent;

	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->bUseAsyncCooking = true;

	MeshComponent->SetVisibility(true);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
}