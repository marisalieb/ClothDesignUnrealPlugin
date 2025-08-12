// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClothDesignToolkit.h"
#include "PropertyCustomizationHelpers.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"


#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "EngineUtils.h"


#define LOCTEXT_NAMESPACE "ClothDesignEditorModeToolkit"

FClothDesignToolkit::FClothDesignToolkit()
{
}


void FClothDesignToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);

	// custom UI
	ToolkitWidget = SNew(SVerticalBox)
		
		// open 2d button
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4)
		[
			MakeOpen2DButton()
		]

		// object pickers
		+ SVerticalBox::Slot()
	    .AutoHeight()
	    .Padding(4)
		[
		   MakeObjectPicker(
			   FText::FromString("Body Object:"),
			   USkeletalMesh::StaticClass(),
			   [this]() { return GetSelectedSkeletalMeshPath(); },
			   [this](const FAssetData& A) { OnSkeletalMeshSelected(A); }
		   )
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4)
		[
			MakeObjectPicker(
				FText::FromString("Cloth Object:"),
				USkeletalMesh::StaticClass(),
				[this]() { return GetSelectedClothMeshPath(); },
				[this](const FAssetData& A) { OnClothMeshSelected(A); }
			)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4)
		[
			MakeObjectPicker(
				FText::FromString("Cloth Material:"),
				UMaterialInterface::StaticClass(),
				[this]() { return GetSelectedTextileMaterialPath(); },
				[this](const FAssetData& A) { OnTextileMaterialSelected(A); }
			)
		]

		// save button
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4)
		[
			SNew(SButton)
			.Text(FText::FromString("Save Setup"))
			// .OnClicked(...) // hook up when ready
		]

		// presets
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4)
		[
			MakePresetPicker()
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

// helper functions to improve readability of the main ui init function
TSharedRef<SWidget> FClothDesignToolkit::MakeOpen2DButton()
{
	return SNew(SButton)
		.Text(FText::FromString("Open 2D Window"))
		.OnClicked(FOnClicked::CreateSP(this, &FClothDesignToolkit::OnOpen2DWindowClicked));
}

TSharedRef<SWidget> FClothDesignToolkit::MakeObjectPicker(
	const FText& LabelText,
	const UClass* AllowedClass,
	TFunction<FString()> GetPath,
	TFunction<void(const FAssetData&)> OnChanged)
{
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
			.AllowedClass(AllowedClass)
			.ObjectPath_Lambda(MoveTemp(GetPath))
			.OnObjectChanged_Lambda([this, OnChanged](const FAssetData& Asset)
			{
				OnChanged(Asset);
			})
			.OnShouldFilterAsset(FOnShouldFilterAsset::CreateLambda([this](const FAssetData& AssetData)
			{
				if (!GEditor)
				return true; // filter out if no editor
			
				UWorld* World = GEditor->GetEditorWorldContext().World();
				if (!World)
				return true;
			
				// We only want to show assets that have at least one component in the world using them
			
				for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
				{
				  // AActor* Actor = ActorItr.Get();
				  AActor* Actor = *ActorItr;
				  if (!Actor)
					continue;
			
				  TArray<USkeletalMeshComponent*> Components;
				  Actor->GetComponents<USkeletalMeshComponent>(Components);
			
				  for (USkeletalMeshComponent* Comp : Components)
				  {
					  if (Comp && Comp->GetSkeletalMeshAsset())
					  {
						  if (Comp->GetSkeletalMeshAsset()->GetPathName() == AssetData.GetSoftObjectPath().GetAssetPathString()
)
						  {
							  return false; // do NOT filter out, asset is used in scene
						  }
					  }
				  }
				}
			
				return true; // filter out assets not used in scene
			}))
		];
}



FString FClothDesignToolkit::GetSelectedSkeletalMeshPath() const
{
	return SelectedSkeletalMesh.IsValid() ? SelectedSkeletalMesh->GetPathName() : FString();
}

void FClothDesignToolkit::OnSkeletalMeshSelected(const FAssetData& AssetData)
{
	SelectedSkeletalMesh = Cast<USkeletalMesh>(AssetData.GetAsset());
	
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

void FClothDesignToolkit::OnTextileMaterialSelected(const FAssetData& AssetData)
{
	SelectedTextileMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());
}


// void FClothDesignToolkit::SetSimFlags(USkeletalMeshComponent* SkelComp)
// {
// 	if (!SkelComp) return;
//
// 	SkelComp->Modify();                      // for undo/redo
// 	SkelComp->bCollideWithEnvironment = true;
// 	SkelComp->bForceCollisionUpdate = true;
//
// 	// SkelComp->UpdateClothCollision();
// 	SkelComp->MarkRenderStateDirty();
// 	SkelComp->RecreatePhysicsState();
//
// #if WITH_EDITOR
// 	SkelComp->PostEditChange();
// #endif
// 	
// }


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
					.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10));
			})

			.OnSelectionChanged_Lambda([this](TSharedPtr<FPresetItem> NewSelection, ESelectInfo::Type)
			{
				if (NewSelection.IsValid())
				{
					SimSettings.SelectedPreset = NewSelection->Preset;
					const FClothPhysicalConfig& UsedPreset = SimSettings.PresetConfigs[SimSettings.SelectedPreset];

					ForEachComponentUsingSelectedMesh([this, &UsedPreset](USkeletalMeshComponent* Comp)
					{
						SimSettings.ApplyPresetToCloth(Comp, UsedPreset, SimSettings.SelectedPreset);
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
	case EClothPreset::Denim:   return FText::FromString("Denim");
	case EClothPreset::Leather: return FText::FromString("Leather");
	case EClothPreset::Silk:    return FText::FromString("Silk");
	case EClothPreset::Jersey:  return FText::FromString("Jersey");
	default:                    return FText::FromString("Unknown");
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
