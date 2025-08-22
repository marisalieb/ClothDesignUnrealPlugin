#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "CoreMinimal.h"

/*
 * Thesis reference:
 * See Chapter 4.8.2 for detailed explanations.
 */


/**
 * @enum EClothPreset
 * @brief Enumerates the available cloth simulation presets.
 * 
 * This enum defines various preset configurations for cloth simulation, such as Denim, Leather, Silk, etc.
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
 * @struct FPresetItem
 * @brief Represents a cloth preset item with its type and display name.
 * 
 * This struct is used to store information about a cloth preset, including its type
 * (as an enum value) and its display name for the UI.
 */
struct FPresetItem
{
	/** The preset type enum value */
	EClothPreset Preset;

	/** Display name for the UI */
	FString DisplayName;

	/**
	 * @brief Constructs an FPresetItem with the given preset type and display name.
	 * 
	 * @param InPreset The preset type.
	 * @param InName The display name for the preset.
	 */
	FPresetItem(EClothPreset InPreset, const FString& InName)
		: Preset(InPreset), DisplayName(InName) {}
};

/**
 * @struct FClothPhysicalConfig
 * @brief Defines the physical properties of a cloth simulation configuration.
 * 
 * This struct contains various parameters that control the physical behavior of the cloth,
 * such as density, stiffness, friction, damping, and aerodynamic properties.
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
 * @struct FClothSimSettings
 * @brief Manages the settings for cloth simulation, including presets and their configurations.
 * 
 * This struct provides functionality to manage cloth simulation presets, apply them to skeletal meshes,
 * and configure collision flags.
 */
struct FClothSimSettings
{
	/** 
	 * @brief Default constructor for FClothSimSettings.
	 * 
	 * Initializes the selected preset to Custom.
	 */
	FClothSimSettings();

	/** Currently selected preset */
	EClothPreset SelectedPreset = EClothPreset::Custom;

	/** Mapping of preset type to physical configuration */
	TMap<EClothPreset, FClothPhysicalConfig> PresetConfigs;

	/** UI-friendly list of preset items for combo boxes */
	TArray<TSharedPtr<FPresetItem>> PresetOptions;

	/**
	 * @brief Applies a cloth preset configuration to the specified skeletal mesh component.
	 * 
	 * @param SkelComp The skeletal mesh component to apply the preset to.
	 * @param Preset The physical configuration of the preset.
	 * @param SelectedPreset The selected preset type.
	 */
	void ApplyPresetToCloth(
		USkeletalMeshComponent* SkelComp,
		const FClothPhysicalConfig& Preset,
		EClothPreset SelectedPreset) const;

	/**
	 * @brief Sets collision flags for the cloth simulation on the specified skeletal mesh component.
	 * 
	 * @param SkelComp The skeletal mesh component to configure collision flags for.
	 */
	static void SetClothCollisionFlags(USkeletalMeshComponent* SkelComp);
};