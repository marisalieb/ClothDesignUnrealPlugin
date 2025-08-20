#pragma once
// Using #pragma once here because this header contains U macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "CoreMinimal.h"

/*
 * Thesis reference:
 * See Chapter 4.8.2 for detailed explanations.
 */

UENUM()
enum class EClothPreset : uint8
{
	Custom,
	Denim,
	Leather,
	Silk,
	Jersey
};

struct FPresetItem
{
	EClothPreset Preset;
	FString DisplayName;

	FPresetItem(EClothPreset InPreset, const FString& InName)
		: Preset(InPreset), DisplayName(InName) {}
};


struct FClothPhysicalConfig
{
	float Density; // single value
	float BendStiffnessLow;
	float BendStiffnessHigh;
	float AreaStiffnessLow;
	float AreaStiffnessHigh;
	float TetherStiffnessLow;
	float TetherStiffnessHigh;
	float TetherScaleLow;
	float TetherScaleHigh;
	
	float Friction; // single value
	float Damping; // single value
	float DragLow;
	float DragHigh;
	float LiftLow;
	float LiftHigh;
	float GravityScale; // single value
};



struct FClothSimSettings
{
	FClothSimSettings(); // ctor declared here (define in .cpp)
	
	EClothPreset SelectedPreset = EClothPreset::Custom;
	
	// Holds the configuration for each preset
	TMap<EClothPreset, FClothPhysicalConfig> PresetConfigs;
	
	// UI-friendly list (shared ptr items) used by SComboBox
	TArray<TSharedPtr<FPresetItem>> PresetOptions;
	
	//void ApplyPresetToCloth(USkeletalMeshComponent* SkelComp, const FClothPhysicalConfig& Preset);
	void ApplyPresetToCloth(
		USkeletalMeshComponent* SkelComp,
		const FClothPhysicalConfig& Preset,
		EClothPreset SelectedPreset) const;

	static void SetClothCollisionFlags(USkeletalMeshComponent* SkelComp);
	
};