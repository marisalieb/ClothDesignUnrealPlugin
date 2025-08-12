#pragma once

#include "CoreMinimal.h"


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
	float Density;
	float BendStiffness;
	float AreaStiffness;
	float TetherStiffness;
	float TetherScale;
	float Friction;
	float Damping;
	float Drag;
	float Lift;
	float GravityScale;
};


struct FClothSimSettings
{
	FClothSimSettings(); // ctor declared here (define in .cpp)

	
	EClothPreset SelectedPreset = EClothPreset::Custom;

	// TArray<TSharedPtr<FPresetItem>> PresetOptions = {
	// 	MakeShared<FPresetItem>(EClothPreset::Denim, TEXT("Denim")),
	// 	MakeShared<FPresetItem>(EClothPreset::Leather, TEXT("Leather")),
	// 	MakeShared<FPresetItem>(EClothPreset::Silk, TEXT("Silk")),
	// 	MakeShared<FPresetItem>(EClothPreset::Jersey, TEXT("Jersey"))
	// };
	// Holds the configuration for each preset
	TMap<EClothPreset, FClothPhysicalConfig> PresetConfigs;
	
	// UI-friendly list (shared ptr items) used by SComboBox
	TArray<TSharedPtr<FPresetItem>> PresetOptions;
	
	//void ApplyPresetToCloth(USkeletalMeshComponent* SkelComp, const FClothPhysicalConfig& Preset);
	void ApplyPresetToCloth(USkeletalMeshComponent* SkelComp, const FClothPhysicalConfig& Preset, EClothPreset SelectedPreset) const;


	static void SetClothCollisionFlags(USkeletalMeshComponent* SkelComp);

	// Put future sim-related settings or functions here,
	// like setting collision flags on the cloth asset
};