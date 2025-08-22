#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "ClothShapeAsset.generated.h"

/*
 * Thesis reference:
 * See Chapter 4.9.1 for detailed explanations.
 */



/**
 * @struct FCurvePointData
 * @brief Represents a single point on a curve, including its position, tangents, and interpolation settings.
 * 
 * This struct is used to define points on a curve for cloth shapes, supporting Bezier interpolation.
 */
USTRUCT(BlueprintType)
struct FCurvePointData
{
	GENERATED_BODY()
	
	/** X value of the curve point (input key) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	float InputKey = 0.0f;

	/** Position of the point in 2D space (output value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	FVector2D Position = FVector2D::ZeroVector;

	/** Tangent arriving to this point for Bezier interpolation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	FVector2D ArriveTangent = FVector2D::ZeroVector;

	/** Tangent leaving this point for Bezier interpolation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	FVector2D LeaveTangent = FVector2D::ZeroVector;

	/** Whether Bezier interpolation should be used for this point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Curve Point")
	bool bUseBezier = false;
};

/**
 * @struct FShapeData
 * @brief Represents a completed cloth shape defined by an array of curve points.
 * 
 * This struct is used to store the data for a completed cloth shape, which is defined by multiple curve points.
 */
USTRUCT(BlueprintType)
struct FShapeData
{
	GENERATED_BODY()

	/** Array of curve points defining the completed shape */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Shape")
	TArray<FCurvePointData> CompletedClothShape;
};

/**
 * @class UClothShapeAsset
 * @brief A data asset for managing cloth shapes, including editable curves and completed shapes.
 * 
 * This class provides functionality to store and manage cloth shapes, including their curve points
 * and completed shapes for use in cloth simulation or design.
 */
UCLASS(BlueprintType)
class UClothShapeAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Curve points for the current editable shape */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Shapes")
	TArray<FCurvePointData> ClothCurvePoints;

	/** Collection of completed cloth shapes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Shapes")
	TArray<FShapeData> ClothShapes;
};