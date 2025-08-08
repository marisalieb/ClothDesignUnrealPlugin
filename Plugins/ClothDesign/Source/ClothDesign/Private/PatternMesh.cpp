#include "PatternMesh.h"


APatternMesh::APatternMesh()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = MeshComponent;

	
	// These two lines are important
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->bUseAsyncCooking = true;

	// Optional: If you want it to be visible in editor
	MeshComponent->SetVisibility(true);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
}