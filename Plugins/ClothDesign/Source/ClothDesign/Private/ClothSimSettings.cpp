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
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/Factory.h"
#include "ClothingAssetBase.h"          // UClothingAssetBase / UClothingAssetCommon
#include "ChaosCloth/ChaosClothConfig.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Editor.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetRegistryModule.h"




FClothSimSettings::FClothSimSettings()
{
	PresetConfigs.Add(EClothPreset::Custom, {});
    

    PresetConfigs.Add(EClothPreset::Denim,
    {0.42f, 0.85f, 0.95f, 0.85f, 0.95f, 0.9f, 0.97f, 1.0f, 1.f,
    0.85f, 0.08f, 0.18f, 0.88f, 0.07f, 1.f, 1.12f});

    PresetConfigs.Add(EClothPreset::Leather,  
    {0.6f, 0.99f, 1.f, 0.95f, 1.f, 1.0f, 1.f, 1.0f, 1.f, 
    0.95f, 0.25f,0.072f, 1.f, 0.06f, 1.f, 1.2f});
    
    PresetConfigs.Add(EClothPreset::Silk,
    {0.12f, 0.50f, 0.70f, 0.50f, 0.70f, 0.68f, 0.83f, 1.0f, 1.0f,
    0.72f, 0.025f, 0.45f, 1.0f, 0.10f, 1.0f, 0.90f});

    PresetConfigs.Add(EClothPreset::Jersey,
    {0.18f, 0.4f, 0.6f, 0.4f, 0.6f, 0.6f, 0.8f, 0.95f, 1.f,
    0.75f, 0.08f, 0.45f, 0.9f, 0.06f, 1.f, 1.05f});

    
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

                ChaosConfig->BendingStiffnessWeighted.Low = Preset.BendStiffnessLow;
                ChaosConfig->BendingStiffnessWeighted.High = Preset.BendStiffnessHigh;


                ChaosConfig->AreaStiffnessWeighted.Low = Preset.AreaStiffnessLow;
                ChaosConfig->AreaStiffnessWeighted.High = Preset.AreaStiffnessHigh;


                ChaosConfig->TetherStiffness.Low = Preset.TetherStiffnessLow;
                ChaosConfig->TetherStiffness.High = Preset.TetherStiffnessHigh;


                ChaosConfig->TetherScale.Low = Preset.TetherScaleLow;
                ChaosConfig->TetherScale.High = Preset.TetherScaleHigh;


                ChaosConfig->Drag.Low = Preset.DragLow;
                ChaosConfig->Drag.High = Preset.DragHigh;


                ChaosConfig->Lift.Low = Preset.LiftLow;
                ChaosConfig->Lift.High = Preset.LiftHigh;

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





// bool FClothSimSettings::CreateAndBindClothingAssetForSkeletalMesh(
//     USkeletalMesh* TargetSkel,
//     const FString& ClothAssetPackagePath, // "/Game/Cloth/MyClothAsset"
//     int32 MeshLODIndex = 0,
//     int32 SectionIndex = 0)
// {
// #if WITH_EDITOR
//     if (!TargetSkel) return false;
//
//     // 1) Create the clothing asset in Content (Editor-only)
//     // Try to use an existing Chaos cloth factory if available (plugin: ChaosClothAssetEditor / ChaosClothAssetTools)
//     UObject* NewAsset = nullptr;
//     const FString PackageName = ClothAssetPackagePath;
//     const FString AssetName = FPackageName::GetShortName(PackageName);
//
//     // Create package
//     UPackage* Package = CreatePackage(*PackageName);
//     Package->FullyLoad();
//
//     // Try to find an appropriate factory (ChaosClothAssetFactory) via AssetTools
//     UFactory* Factory = nullptr;
//     {
//         // This is the pattern: the editor plugin usually exposes a factory called ChaosClothAssetFactory.
//         // If you have that plugin, you can instantiate it directly:
//         // #include "ClothAssetFactory.h"  (from ChaosClothAssetTools)
//         // Factory = NewObject<UChaosClothAssetFactory>(GetTransientPackage());
//
//         // Fallback: create a generic UObject asset and cast later (not ideal)
//     }
//
//     // Preferred: use AssetTools to create asset with factory (if you have the factory)
//     if (Factory)
//     {
//         FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
//         NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), UClothingAssetCommon::StaticClass(), Factory);
//     }
//     else
//     {
//         // Fallback: create a blank clothing asset object in package (you will need to fill it)
//         NewAsset = NewObject<UClothingAssetCommon>(Package, UClothingAssetCommon::StaticClass(), *AssetName, RF_Public | RF_Standalone);
//         if (NewAsset)
//         {
//             FAssetRegistryModule::AssetCreated(NewAsset);
//             Package->MarkPackageDirty();
//             // Save package to disk so it shows in Content Browser:
//             FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
//             UPackage::SavePackage(Package, NewAsset, RF_Public | RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_None);
//         }
//     }
//
//    
//
//
//     return true;
// #else
//     return false;
// #endif
// }