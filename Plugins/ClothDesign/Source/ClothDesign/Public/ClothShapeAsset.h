#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "ClothShapeAsset.generated.h"

USTRUCT(BlueprintType)
struct FCurvePointData
{
	GENERATED_BODY()
	
	// The X value for the curve point (used as InVal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	float InputKey = 0.0f;

	// The actual position of the point in 2D space (used as OutVal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	FVector2D Position = FVector2D::ZeroVector;

	// Tangents
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	FVector2D ArriveTangent = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	FVector2D LeaveTangent = FVector2D::ZeroVector;

	// Whether to use Bezier for this point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	bool bUseBezier = false;
	
};

USTRUCT(BlueprintType)
struct FShapeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Shape")
	TArray<FCurvePointData> CompletedClothShape;
};


UCLASS(BlueprintType)
class UClothShapeAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Shapes")
	TArray<FCurvePointData> ClothCurvePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Shapes")
	TArray<FShapeData> ClothShapes;

};