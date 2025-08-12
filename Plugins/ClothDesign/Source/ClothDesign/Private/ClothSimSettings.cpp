#include "ClothSimSettings.h"

#include "Components/SkeletalMeshComponent.h"
#include "ChaosCloth/ChaosClothingSimulationInteractor.h"
#include "ClothingSimulationInteractor.h"
#include "Components/SkeletalMeshComponent.h"                 // USkeletalMeshComponent
// #include "ClothingSystemRuntimeInterface/Public/ClothingSimulationInteractor.h" // UClothingSimulationInteractor
// #include "ClothingSystemRuntimeInterface/Public/ClothingAssetBase.h" // UClothingAssetBase
#include "ClothingAssetBase.h"
#include "ClothingAsset.h" // ClothingSystemRuntimeCommon/Public/ClothingAsset.h
#include "ChaosCloth/ChaosClothConfig.h"

FClothSimSettings::FClothSimSettings()
{
	PresetConfigs.Add(EClothPreset::Custom, {});

    PresetConfigs.Add(EClothPreset::Denim,   {0.25f, 0.85f, 0.85f, 1.0f, 0.99f, 0.8f, 0.03f, 0.15f, 0.07f, 1.05f});
    PresetConfigs.Add(EClothPreset::Leather, {0.35f, 0.9f,  0.9f,  1.0f, 0.99f, 0.85f, 0.04f, 0.18f, 0.09f, 1.2f});
    PresetConfigs.Add(EClothPreset::Silk,    {0.15f, 0.6f,  0.6f,  1.0f, 0.98f, 0.7f,  0.02f, 0.12f, 0.05f, 1.05f});
    PresetConfigs.Add(EClothPreset::Jersey,  {0.20f, 0.75f, 0.75f, 1.0f, 0.99f, 0.75f, 0.025f,0.14f, 0.06f, 0.9f});

    // create UI items (display names)
	PresetOptions.Add(MakeShared<FPresetItem>(EClothPreset::Custom,   TEXT("Custom")));
    PresetOptions.Add(MakeShared<FPresetItem>(EClothPreset::Denim,   TEXT("Denim")));
    PresetOptions.Add(MakeShared<FPresetItem>(EClothPreset::Leather, TEXT("Leather")));
    PresetOptions.Add(MakeShared<FPresetItem>(EClothPreset::Silk,    TEXT("Silk")));
    PresetOptions.Add(MakeShared<FPresetItem>(EClothPreset::Jersey,  TEXT("Jersey")));
    
}

void FClothSimSettings::SetClothCollisionFlags(USkeletalMeshComponent* SkelComp)
{
	if (!SkelComp) return;

	SkelComp->Modify(); // For undo/redo support
	SkelComp->bCollideWithEnvironment = true;
	SkelComp->bForceCollisionUpdate = true;

	// Optionally, update cloth collision immediately
	// SkelComp->UpdateClothCollision();

	SkelComp->MarkRenderStateDirty();
	SkelComp->RecreatePhysicsState();

#if WITH_EDITOR
	SkelComp->PostEditChange();
#endif
}

void FClothSimSettings::ApplyPresetToCloth(USkeletalMeshComponent* SkelComp, const FClothPhysicalConfig& Preset, EClothPreset SelectedPreset) const
{
    if (!SkelComp) return;

    if (SelectedPreset == EClothPreset::Custom)
    {
        UE_LOG(LogTemp, Log, TEXT("ApplyPresetToCloth: Custom preset selected — leaving UE defaults"));
        return;
    }

    UClothingSimulationInteractor* SimInteractor = SkelComp->GetClothingSimulationInteractor();

    const USkeletalMesh* MeshAsset = SkelComp->GetSkeletalMeshAsset();
    if (!MeshAsset) return;

    for (UClothingAssetBase* ClothingBase : MeshAsset->GetMeshClothingAssets())
    {
        if (!ClothingBase) continue;

        if (UClothingAssetCommon* CommonAsset = Cast<UClothingAssetCommon>(ClothingBase))
        {
            if (UChaosClothConfig* ChaosConfig = CommonAsset->GetClothConfig<UChaosClothConfig>())
            {
                // Apply asset-level settings
                ChaosConfig->Density = Preset.Density;
                ChaosConfig->FrictionCoefficient = Preset.Friction;
                ChaosConfig->DampingCoefficient = Preset.Damping;
                ChaosConfig->GravityScale = Preset.GravityScale;

                ChaosConfig->BendingStiffnessWeighted.Low = Preset.BendStiffness;
                ChaosConfig->BendingStiffnessWeighted.High = Preset.BendStiffness;

                ChaosConfig->AreaStiffnessWeighted.Low = Preset.AreaStiffness;
                ChaosConfig->AreaStiffnessWeighted.High = Preset.AreaStiffness;

                ChaosConfig->TetherStiffness.Low = Preset.TetherStiffness;
                ChaosConfig->TetherStiffness.High = Preset.TetherStiffness;

                ChaosConfig->TetherScale.Low = Preset.TetherScale;
                ChaosConfig->TetherScale.High = Preset.TetherScale;

                ChaosConfig->Drag.Low = Preset.Drag;
                ChaosConfig->Drag.High = Preset.Drag;

                ChaosConfig->Lift.Low = Preset.Lift;
                ChaosConfig->Lift.High = Preset.Lift;

                CommonAsset->InvalidateAllCachedData();
                CommonAsset->MarkPackageDirty();
            }
        }
    }
	
    SkelComp->MarkRenderStateDirty();
    SkelComp->RecreatePhysicsState();
#if WITH_EDITOR
    SkelComp->PostEditChange();
#endif
}

// void FClothSimSettings::ApplyPresetToCloth(USkeletalMeshComponent* SkelComp, const FClothPhysicalConfig& Preset)
// {
//     if (!SkelComp)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("ApplyToCloth: SkelComp null"));
//         return;
//     }
//
// 	if (SelectedPreset == EClothPreset::Custom)
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("ApplyPresetToCloth: Custom preset selected — leaving UE defaults"));
// 		return;
// 	}
//
//     // find simulation interactor
//     UClothingSimulationInteractor* SimInteractor = SkelComp->GetClothingSimulationInteractor();
//     if (!SimInteractor)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("ApplyToCloth: No clothing simulation interactor on component '%s'"), *SkelComp->GetName());
//         return;
//     }
// 	
// 	if (!SkelComp || !SkelComp->GetSkeletalMeshAsset())
// 		return;
//
// 	USkeletalMesh* MeshAsset = SkelComp->GetSkeletalMeshAsset();
//
// 	for (UClothingAssetBase* ClothingAssetBase : MeshAsset->GetMeshClothingAssets())
// 	{
// 		if (UClothingAssetCommon* CommonAsset = Cast<UClothingAssetCommon>(ClothingAssetBase))
// 		{
// 			for (auto& ConfigPair : CommonAsset->ClothConfigs)
// 			{
// 				if (UChaosClothConfig* ChaosConfig = Cast<UChaosClothConfig>(ConfigPair.Value))
// 				{
// 					ChaosConfig->Density = Preset.Density;
// 					ChaosConfig->GravityScale = Preset.GravityScale;
// 					
// 					ChaosConfig->FrictionCoefficient = Preset.Friction;
// 					ChaosConfig->DampingCoefficient = Preset.Damping;
// 					
// 					ChaosConfig->BendingStiffnessWeighted.Low = Preset.BendStiffness;
// 					ChaosConfig->BendingStiffnessWeighted.High = Preset.BendStiffness;
//
// 					ChaosConfig->AreaStiffnessWeighted.Low = Preset.AreaStiffness;
// 					ChaosConfig->AreaStiffnessWeighted.High = Preset.AreaStiffness;
// 					
// 					ChaosConfig->TetherStiffness.Low  = Preset.TetherStiffness;
// 					ChaosConfig->TetherStiffness.High = Preset.TetherStiffness;
// 					
// 					ChaosConfig->TetherScale.Low  = Preset.TetherScale;
// 					ChaosConfig->TetherScale.High = Preset.TetherScale;
// 					
// 					ChaosConfig->Drag.Low  = Preset.Drag;
// 					ChaosConfig->Drag.High = Preset.Drag;
//
// 					ChaosConfig->Lift.Low  = Preset.Lift;
// 					ChaosConfig->Lift.High = Preset.Lift;
//
// 				}
// 			}
//
// 	
// #if WITH_EDITOR
// 			CommonAsset->MarkPackageDirty();
// 			CommonAsset->PostEditChange();
// #endif
// 		}
// 	}
//
// 	// Force the skeletal mesh component to refresh its physics
// 	SkelComp->RecreatePhysicsState();
// }

