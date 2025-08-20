#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "ClothShapeAsset.generated.h"

/*
 * Thesis reference:
 * See Chapter 4.9.1 for detailed explanations.
 */



/**
 * @brief Stores a single point on a curve, including position and tangents.
 * 
 * Used for defining cloth patterns and Bezier curve control points.
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
 * @brief Represents a completed cloth shape made of multiple curve points.
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
 * @brief Data asset storing multiple cloth shapes and their curve points.
 * 
 * Can be used to save and load predefined cloth patterns in the editor or at runtime.
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