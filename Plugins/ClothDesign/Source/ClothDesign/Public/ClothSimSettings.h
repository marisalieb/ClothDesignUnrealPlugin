#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "CoreMinimal.h"

/*
 * Thesis reference:
 * See Chapter 4.8.2 for detailed explanations.
 */


/**
 * @brief Enumerates preset types for cloth simulation properties.
 * 
 * Used to quickly apply common cloth materials with pre-defined physical configurations.
 */
UENUM()
enum class EClothPreset : uint8
{
	Custom,  /**< User-defined cloth settings */
	Denim,   /**< Denim preset */
	Leather, /**< Leather preset */
	Silk,    /**< Silk preset */
	Jersey   /**< Jersey fabric preset */
};

/**
 * @brief Represents a single item in the cloth preset dropdown or UI.
 */
struct FPresetItem
{
	/** The preset type enum value */
	EClothPreset Preset;

	/** Display name for the UI */
	FString DisplayName;

	FPresetItem(EClothPreset InPreset, const FString& InName)
		: Preset(InPreset), DisplayName(InName) {}
};

/**
 * @brief Stores the physical parameters for a cloth material.
 * 
 * Includes density, bending stiffness, area stiffness, tethers, friction, damping, drag,
 * lift, and gravity scaling. Can be used for both presets and custom configurations.
 */
struct FClothPhysicalConfig
{
	/** Mass density of the cloth */
	float Density;

	/** Bending stiffness range */
	float BendStiffnessLow;
	float BendStiffnessHigh;

	/** Area stiffness range */
	float AreaStiffnessLow;
	float AreaStiffnessHigh;

	/** Tether stiffness range */
	float TetherStiffnessLow;
	float TetherStiffnessHigh;

	/** Tether scaling range */
	float TetherScaleLow;
	float TetherScaleHigh;

	/** Friction coefficient */
	float Friction;

	/** Damping factor */
	float Damping;

	/** Aerodynamic drag range */
	float DragLow;
	float DragHigh;

	/** Aerodynamic lift range */
	float LiftLow;
	float LiftHigh;

	/** Gravity scale factor */
	float GravityScale;
};

/**
 * @brief Manages cloth simulation settings and presets.
 * 
 * Provides helper methods to apply presets to a skeletal mesh and configure cloth collision.
 */
struct FClothSimSettings
{
	/** Default constructor */
	FClothSimSettings();

	/** Currently selected preset */
	EClothPreset SelectedPreset = EClothPreset::Custom;

	/** Mapping of preset type to physical configuration */
	TMap<EClothPreset, FClothPhysicalConfig> PresetConfigs;

	/** UI-friendly list of preset items for combo boxes */
	TArray<TSharedPtr<FPresetItem>> PresetOptions;

	/**
	 * @brief Apply a given preset configuration to a skeletal mesh component.
	 * @param SkelComp Skeletal mesh component to apply settings to
	 * @param Preset The cloth physical configuration to apply
	 * @param SelectedPreset Enum representing the selected preset
	 */
	void ApplyPresetToCloth(
		USkeletalMeshComponent* SkelComp,
		const FClothPhysicalConfig& Preset,
		EClothPreset SelectedPreset) const;

	/**
	 * @brief Sets default collision flags for the cloth on a skeletal mesh.
	 * @param SkelComp Skeletal mesh component to configure
	 */
	static void SetClothCollisionFlags(USkeletalMeshComponent* SkelComp);
};