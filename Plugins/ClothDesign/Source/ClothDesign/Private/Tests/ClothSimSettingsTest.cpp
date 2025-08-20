#include "Misc/AutomationTest.h"
#include "ClothSimSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothSimSettings_DefaultsTest,
    "ClothSimSettings.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothSimSettings_DefaultsTest::RunTest(const FString& Parameters)
{
    FClothSimSettings Settings;

    // should contain all 5 presets
    TestEqual(TEXT("PresetConfigs should contain 5 entries"), Settings.PresetConfigs.Num(), 5);
    TestEqual(TEXT("PresetOptions should contain 5 entries"), Settings.PresetOptions.Num(), 5);

    // check a known preset
    const FClothPhysicalConfig* SilkConfig = Settings.PresetConfigs.Find(EClothPreset::Silk);
    TestNotNull(TEXT("Silk preset should exist"), SilkConfig);
    if (SilkConfig)
    {
        TestEqual(TEXT("Silk.Density should match expected"), SilkConfig->Density, 0.12f);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothSimSettings_PresetOptionsNamesTest,
    "ClothSimSettings.PresetOptionsNames",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothSimSettings_PresetOptionsNamesTest::RunTest(const FString& Parameters)
{
    FClothSimSettings Settings;

    // display name should correspond to the enum value
    TestEqual(TEXT("First option should be 'Custom'"), Settings.PresetOptions[0]->DisplayName, FString("Custom"));
    TestEqual(TEXT("Second option should be 'Denim'"), Settings.PresetOptions[1]->DisplayName, FString("Denim"));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothSimSettings_SetClothCollisionFlagsTest,
    "ClothSimSettings.SetClothCollisionFlags",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothSimSettings_SetClothCollisionFlagsTest::RunTest(const FString& Parameters)
{
    USkeletalMeshComponent* SkelComp = NewObject<USkeletalMeshComponent>();
    TestNotNull(TEXT("SkeletalMeshComponent should be created"), SkelComp);

    // initially false
    TestFalse(TEXT("bCollideWithEnvironment initially false"), SkelComp->bCollideWithEnvironment);

    FClothSimSettings::SetClothCollisionFlags(SkelComp);

    TestTrue(TEXT("bCollideWithEnvironment should be true"), SkelComp->bCollideWithEnvironment);
    TestTrue(TEXT("bForceCollisionUpdate should be true"), SkelComp->bForceCollisionUpdate);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothSimSettings_ApplyPreset_CustomTest,
    "ClothSimSettings.ApplyPresetCustom",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothSimSettings_ApplyPreset_CustomTest::RunTest(const FString& Parameters)
{
    FClothSimSettings Settings;

    USkeletalMeshComponent* SkelComp = NewObject<USkeletalMeshComponent>();
    TestNotNull(TEXT("SkeletalMeshComponent should be created"), SkelComp);

    // should early out without modifying anything
    Settings.ApplyPresetToCloth(SkelComp, {}, EClothPreset::Custom);

    TestTrue(TEXT("Still valid after ApplyPresetToCloth with Custom"), IsValid(SkelComp));

    return true;
}


