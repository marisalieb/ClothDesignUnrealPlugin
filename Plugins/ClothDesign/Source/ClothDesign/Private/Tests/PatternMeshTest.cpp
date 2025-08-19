#include "PatternMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatternMesh_DefaultsTest,
	"PatternMesh.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatternMesh_DefaultsTest::RunTest(const FString& Parameters)
{
	// Get the current world
	UWorld* World = GEngine->GetCurrentPlayWorld();
	if (!World)
	{
		AddWarning(TEXT("No world available for test"));
		return true;
	}

	// Spawn the actor
	APatternMesh* PatternMesh = World->SpawnActor<APatternMesh>();
	TestNotNull(TEXT("PatternMesh should be spawned"), PatternMesh);

	// Test the MeshComponent
	TestNotNull(TEXT("MeshComponent should exist"), PatternMesh->MeshComponent);
	TestTrue(TEXT("MeshComponent should be movable"), PatternMesh->MeshComponent->Mobility == EComponentMobility::Movable);
	TestTrue(TEXT("MeshComponent should be visible"), PatternMesh->MeshComponent->IsVisible());
	TestTrue(TEXT("MeshComponent collision should be enabled"), PatternMesh->MeshComponent->GetCollisionEnabled() != ECollisionEnabled::NoCollision);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatternMesh_PolyIndexMappingTest,
	"PatternMesh.PolyIndexMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatternMesh_PolyIndexMappingTest::RunTest(const FString& Parameters)
{
	// Create the object (UObject or Actor, depending on your class)
	APatternMesh* PatternMesh = NewObject<APatternMesh>();
	TestNotNull(TEXT("PatternMesh should be created"), PatternMesh);

	// Prepare test data
	TArray<int32> Mapping = { 3, 5, 7, 9 };

	// Test setter
	PatternMesh->SetPolyIndexToVID(Mapping);

	// Test getter
	const TArray<int32>& Retrieved = PatternMesh->GetPolyIndexToVID();
	TestEqual(TEXT("PolyIndexToVID array size should match"), Retrieved.Num(), Mapping.Num());
	for (int32 i = 0; i < Mapping.Num(); ++i)
	{
		TestEqual(FString::Printf(TEXT("PolyIndexToVID element %d"), i), Retrieved[i], Mapping[i]);
	}

	return true;
}
