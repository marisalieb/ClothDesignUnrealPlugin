
#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

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

	// sampled points along the 2D pattern boundary (order preserved)
	UPROPERTY()
	TArray<FVector2f> BoundarySamplePoints2D;
	
	// corresponding vertex IDs in DynamicMesh for each sample
	UPROPERTY()
	TArray<int32> BoundarySampleVertexIDs;

	// world-space positions of those vertex IDs (cached)
	UPROPERTY()
	TArray<FVector> BoundarySampleWorldPositions;

	void SetPolyIndexToVID(const TArray<int32>& InMapping) { PolyIndexToVID = InMapping; }
	const TArray<int32>& GetPolyIndexToVID() const { return PolyIndexToVID; }
	
	// Keep the entire dynamic mesh around so we can query vertex positions
	// at alignment time.
	UE::Geometry::FDynamicMesh3 DynamicMesh;

private:
	UPROPERTY()
	TArray<int32> PolyIndexToVID; // maps 2D polygon index to 3D vertex ID
	
	
};

