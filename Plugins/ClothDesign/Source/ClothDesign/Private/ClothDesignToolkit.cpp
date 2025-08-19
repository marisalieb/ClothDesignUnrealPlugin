
#include "ClothDesignToolkit.h"
#include "PropertyCustomizationHelpers.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"


#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "ClothDesignModule.h"


// This file was started using the Unreal Engine 5.5 Editor Mode C++ template.
// This template can be created directly in Unreal Engine in the plugins section.
//
// The template code has been modified and extended for this project.
// The unmodified template code is included at the bottom of the file for reference.


#define LOCTEXT_NAMESPACE "ClothDesignEditorModeToolkit"

FClothDesignToolkit::FClothDesignToolkit()
{
}


void FClothDesignToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);

	// custom UI
	ToolkitWidget = SNew(SVerticalBox)


		// SNew(SVerticalBox)
		// open 2d button
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4)
		[
			MakeOpen2DButton()
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4) [ SNew(SSeparator).Thickness(1.5f) ]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4)
		[
			MakeObjectPicker(
				FText::FromString("Cloth Object:"),
				USkeletalMesh::StaticClass(),
				[this]() { return GetSelectedClothMeshPath(); },
				[this](const FAssetData& A) { OnClothMeshSelected(A); },
				true)
		]
		
		// + SVerticalBox::Slot().AutoHeight().Padding(4) [ SNew(SSeparator).Thickness(1.5f) ]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			MakeClothSettingsSection()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			MakeCollisionSection()
		];

}

void FClothDesignToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}

FName FClothDesignToolkit::GetToolkitFName() const
{
	return FName("ClothDesignEditorMode");
}

FText FClothDesignToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "ClothDesignEditorMode Toolkit");
}

TSharedPtr<SWidget> FClothDesignToolkit::GetInlineContent() const
{
	return ToolkitWidget;
}


FReply FClothDesignToolkit::OnOpen2DWindowClicked()
{
	static const FName TwoDTabName("TwoDWindowTab"); // change name here

	FGlobalTabmanager::Get()->TryInvokeTab(TwoDTabName);
	return FReply::Handled();
}


TSharedRef<SWidget> FClothDesignToolkit::MakeOpen2DButton()
{
	return SNew(SButton)
		.Text(FText::FromString("Open 2D Editor"))
		.OnClicked(FOnClicked::CreateLambda([]() -> FReply {
			FModuleManager::GetModuleChecked<FClothDesignModule>("ClothDesign").Spawn2DWindow();
			return FReply::Handled();
		}));
}


TSharedRef<SWidget> FClothDesignToolkit::MakeClothSettingsSection()
{
	return
		SNew(SExpandableArea)
		.AreaTitle(LOCTEXT("ClothSettings", "Cloth Settings"))
		// .AreaTitle(FText::FromString("Cloth Settings"))
		.InitiallyCollapsed(true) // start collapsed by default
		.BodyContent()
		[
			SNew(SVerticalBox)

			// Cloth Material picker
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			[
				MakeObjectPicker(
					FText::FromString("Cloth Material:"),
					UMaterialInterface::StaticClass(),
					[this]() { return GetSelectedTextileMaterialPath(); },
					[this](const FAssetData& A) { OnTextileMaterialSelected(A); },
					false)
			]

			// Preset picker
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			[
				MakePresetPicker()
			]
		];
}

TSharedRef<SWidget> FClothDesignToolkit::MakeCollisionSection()
{
	return
		SNew(SExpandableArea)
		.AreaTitle(LOCTEXT("Collision", "Collision Object"))
		// .AreaTitle(FText::FromString("Cloth Settings"))
		.InitiallyCollapsed(true) // start collapsed by default
		.BodyContent()
		[
			SNew(SVerticalBox)

			// object pickers
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			[
				MakeObjectPicker(
					FText::FromString("Collision Object:"),
					UStaticMesh::StaticClass(),
					[this]() { return GetSelectedCollisionMeshPath(); },
					[this](const FAssetData& A) { OnCollisionMeshSelected(A); },
					true)
			]
		];
}



TSharedRef<SWidget> FClothDesignToolkit::MakeObjectPicker(
	const FText& LabelText,
	const UClass* AllowedClass,
	TFunction<FString()> GetPath,
	TFunction<void(const FAssetData&)> OnChanged,
	bool bFilterBySceneUsage)
{
// Start building the property entry box
    SObjectPropertyEntryBox::FArguments EntryBoxArgs;
    EntryBoxArgs.AllowedClass(AllowedClass);
    EntryBoxArgs.ObjectPath_Lambda(MoveTemp(GetPath));
    EntryBoxArgs.OnObjectChanged_Lambda([OnChanged](const FAssetData& Asset)
    {
        OnChanged(Asset);
    });

    // Only add filtering if requested
    if (bFilterBySceneUsage)
    {
        EntryBoxArgs.OnShouldFilterAsset(FOnShouldFilterAsset::CreateLambda([](const FAssetData& AssetData)
        {
            if (!GEditor)
                return true; // filter out if no editor

            UWorld* World = GEditor->GetEditorWorldContext().World();
            if (!World)
                return true;

            for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
            {
                AActor* Actor = *ActorItr;
                if (!Actor)
                    continue;

                TArray<UMeshComponent*> Components;
                Actor->GetComponents<UMeshComponent>(Components);

                for (UMeshComponent* Comp : Components)
                {
                    if (!Comp)
                        continue;

                    UObject* UsedAsset = nullptr;

                    if (USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(Comp))
                    {
                        UsedAsset = SkelComp->GetSkeletalMeshAsset();
                    }
                    else if (UStaticMeshComponent* StaticComp = Cast<UStaticMeshComponent>(Comp))
                    {
                        UsedAsset = StaticComp->GetStaticMesh();
                    }

                    if (UsedAsset && UsedAsset->GetPathName() == AssetData.GetSoftObjectPath().GetAssetPathString())
                    {
                        return false; // asset is in scene → don't filter out
                    }
                }
            }

            return true; // filter out assets not used in scene
        }));
    }

    // Build the horizontal layout with the label and picker
    return SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(4)
        [
            SNew(STextBlock).Text(LabelText)
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(4)
        [
            SNew(SObjectPropertyEntryBox)
            .AllowedClass(EntryBoxArgs._AllowedClass)
            .ObjectPath(EntryBoxArgs._ObjectPath)
            .OnObjectChanged(EntryBoxArgs._OnObjectChanged)
            .OnShouldFilterAsset(EntryBoxArgs._OnShouldFilterAsset) // Will be empty if not filtering
        ];
}


FString FClothDesignToolkit::GetSelectedCollisionMeshPath() const
{
	return SelectedCollisionMesh.IsValid() ? SelectedCollisionMesh->GetPathName() : FString();
}

void FClothDesignToolkit::OnCollisionMeshSelected(const FAssetData& AssetData)
{
	SelectedCollisionMesh = Cast<UStaticMesh>(AssetData.GetAsset());
}


FString FClothDesignToolkit::GetSelectedClothMeshPath() const
{
	return SelectedClothMesh.IsValid() ? SelectedClothMesh->GetPathName() : FString();
}

void FClothDesignToolkit::OnClothMeshSelected(const FAssetData& AssetData)
{
	SelectedClothMesh = Cast<USkeletalMesh>(AssetData.GetAsset());

	if (!SelectedClothMesh.IsValid())
	{
		return;
	}

	// Find all skeletal mesh components in the level that use this mesh
	UWorld* World = nullptr;

#if WITH_EDITOR
	if (GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
#endif

	if (!World)
	{
		return;
	}

	// Apply current preset to all components using this mesh
	// Make a copy (not a reference) and capture it by value
	const FClothPhysicalConfig UsedPreset = SimSettings.PresetConfigs[SimSettings.SelectedPreset];

	ForEachComponentUsingSelectedMesh([this, UsedPreset](USkeletalMeshComponent* Comp)
	{
		SimSettings.ApplyPresetToCloth(Comp, UsedPreset, SimSettings.SelectedPreset);
	});

	for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
	{
		AActor* Actor = *ActorItr;
		if (!Actor)
			continue;

		TArray<USkeletalMeshComponent*> Components;
		Actor->GetComponents<USkeletalMeshComponent>(Components);

		for (USkeletalMeshComponent* SkelComp : Components)
		{
			if (SkelComp && SkelComp->GetSkeletalMeshAsset() == SelectedClothMesh.Get())
			{
				SimSettings.SetClothCollisionFlags(SkelComp);
			}
		}
	}
}

FString FClothDesignToolkit::GetSelectedTextileMaterialPath() const
{
	return SelectedTextileMaterial.IsValid() ? SelectedTextileMaterial->GetPathName() : FString();
}




#if WITH_EDITOR

// New helper: iterate components that use the given mesh (explicit param)
void FClothDesignToolkit::ForEachComponentUsingMesh(USkeletalMesh* Mesh, TFunctionRef<void(USkeletalMeshComponent*)> Op) const
{
	if (!Mesh)
	{
		return;
	}

	UWorld* World = nullptr;
	if (GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
	{
		AActor* Actor = *ActorItr;
		if (!Actor) continue;

		TArray<USkeletalMeshComponent*> Components;
		Actor->GetComponents<USkeletalMeshComponent>(Components);
		for (USkeletalMeshComponent* SkelComp : Components)
		{
			if (!SkelComp) continue;

			// Compare the skeletal mesh asset pointer directly
			if (SkelComp->GetSkeletalMeshAsset() == Mesh)
			{
				Op(SkelComp);
			}
		}
	}
}

#endif // WITH_EDITOR


// Modified OnTextileMaterialSelected: explicitly use the current SelectedClothMesh when applying
void FClothDesignToolkit::OnTextileMaterialSelected(const FAssetData& AssetData)
{
	SelectedTextileMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());

	if (!SelectedTextileMaterial.IsValid())
	{
		return;
	}

#if WITH_EDITOR
	// If no cloth mesh currently selected, do nothing (or optionally notify user)
	USkeletalMesh* TargetMesh = SelectedClothMesh.Get();
	if (!TargetMesh)
	{
		// Optionally: log or notify the user that no mesh is selected
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("ApplyTextileMaterialTx", "Apply Textile Material"));

	// Use the explicit helper so we always test against the current mesh
	ForEachComponentUsingMesh(TargetMesh, [this](USkeletalMeshComponent* SkelComp)
	{
		if (!SkelComp) return;
		SkelComp->Modify();
		const int32 NumMats = SkelComp->GetNumMaterials();
		for (int32 SlotIndex = 0; SlotIndex < NumMats; ++SlotIndex)
		{
			// SkelComp->SetMaterial(SlotIndex, SelectedTextileMaterial.Get());
			UMaterialInterface* BaseMaterial = SelectedTextileMaterial.Get();
			UMaterialInstanceDynamic* MID = SkelComp->CreateAndSetMaterialInstanceDynamicFromMaterial(SlotIndex, BaseMaterial);

		}
		
		SkelComp->MarkRenderStateDirty();
	});
#endif // WITH_EDITOR
}


TSharedRef<SWidget> FClothDesignToolkit::MakePresetPicker()
{
	// Find the currently selected preset shared ptr
	SelectedPresetSharedPtr = nullptr;
	for (const TSharedPtr<FPresetItem>& Item : SimSettings.PresetOptions)
	{
		if (Item->Preset == SimSettings.SelectedPreset)
		{
			SelectedPresetSharedPtr = Item;
			break;
		}
	}

	return
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(STextBlock).Text(FText::FromString("Select Preset:"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(SComboBox<TSharedPtr<FPresetItem>>)
	        .OptionsSource(&SimSettings.PresetOptions)
	        .InitiallySelectedItem(SelectedPresetSharedPtr)
			// This fixes your issue:
	        .OnGenerateWidget_Lambda([](TSharedPtr<FPresetItem> InItem)
	        {
	            return SNew(STextBlock)
	                .Text(FText::FromString(InItem->DisplayName))
	                .Font(FSlateFontInfo(
	                    FPaths::EngineContentDir() / TEXT(
	                        "Slate/Fonts/Roboto-Regular.ttf"), 10));
	        })

	        .OnSelectionChanged_Lambda(
	            [this](TSharedPtr<FPresetItem> NewSelection,
	                   ESelectInfo::Type)
	            {
	                if (NewSelection.IsValid())
	                {
	                    SimSettings.SelectedPreset = NewSelection->Preset;
	                    const FClothPhysicalConfig& UsedPreset = SimSettings
	                        .PresetConfigs[SimSettings.SelectedPreset];

	                    ForEachComponentUsingSelectedMesh(
	                        [this, &UsedPreset](
	                        USkeletalMeshComponent* Comp)
	                        {
	                            SimSettings.ApplyPresetToCloth(
	                                Comp, UsedPreset,
	                                SimSettings.SelectedPreset);
	                        });
	                }
	            })

	        .Content()
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					for (const auto& Item : SimSettings.PresetOptions)
					{
						if (Item->Preset == SimSettings.SelectedPreset)
							return FText::FromString(Item->DisplayName);
					}
					return FText::FromString(TEXT("Select Preset"));
				})
			]
		];
}


void FClothDesignToolkit::OnPresetSelected(TSharedPtr<FPresetItem> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedPresetSharedPtr = NewSelection;
		SimSettings.SelectedPreset = NewSelection->Preset;

		UE_LOG(LogTemp, Log, TEXT("Preset selected: %s"), *SelectedPresetSharedPtr->DisplayName);
		// Use SelectedPreset->Preset here to access enum value

		// // Re-apply the newly selected preset to all components that use the mesh
		// ForEachComponentUsingSelectedMesh([this](USkeletalMeshComponent* Comp){
		// 	SimSettings.ApplyToCloth(Comp);
		// });
		// Make a copy (not a reference) and capture it by value
		const FClothPhysicalConfig UsedPreset = SimSettings.PresetConfigs[SimSettings.SelectedPreset];

		ForEachComponentUsingSelectedMesh([this, UsedPreset](USkeletalMeshComponent* Comp)
		{
			SimSettings.ApplyPresetToCloth(Comp, UsedPreset, SimSettings.SelectedPreset);
		});
	}
}


FText FClothDesignToolkit::GetPresetDisplayName(EClothPreset Preset)
{
	switch (Preset)
	{
	case EClothPreset::Denim: return FText::FromString("Denim");
	case EClothPreset::Leather: return FText::FromString("Leather");
	case EClothPreset::Silk: return FText::FromString("Silk");
	case EClothPreset::Jersey: return FText::FromString("Jersey");
	default: return FText::FromString("Unknown");
	}
}

// helper: iterate components using the picked mesh and call a lambda
void FClothDesignToolkit::ForEachComponentUsingSelectedMesh(TFunctionRef<void(USkeletalMeshComponent*)> Fn)
{
#if WITH_EDITOR
	if (!SelectedClothMesh.IsValid() || !GEditor) return;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		// AActor* Actor = It->GetActor();
		AActor* Actor = *It;

		if (!Actor) continue;

		TArray<USkeletalMeshComponent*> Comps;
		Actor->GetComponents<USkeletalMeshComponent>(Comps);

		for (USkeletalMeshComponent* Comp : Comps)
		{
			if (!Comp) continue;
			if (Comp->GetSkeletalMeshAsset() == SelectedClothMesh.Get())
			{
				Fn(Comp);
			}
		}
	}
#endif
}

#undef LOCTEXT_NAMESPACE

/*
------------------------------------------
Original Unreal Engine Template Code
------------------------------------------
// Copyright Epic Games, Inc. All Rights Reserved.

#include "NewEditorModeEditorModeToolkit.h"
#include "NewEditorModeEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "NewEditorModeEditorModeToolkit"

FNewEditorModeEditorModeToolkit::FNewEditorModeEditorModeToolkit()
{
}

void FNewEditorModeEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FNewEditorModeEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FNewEditorModeEditorModeToolkit::GetToolkitFName() const
{
	return FName("NewEditorModeEditorMode");
}

FText FNewEditorModeEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "NewEditorModeEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
*/
