
#include "ClothDesignModule.h"
#include "ClothDesignCommands.h"
#include "ClothDesignCanvas.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Application/SlateApplication.h"
#include "Tickable.h"
#include "Containers/Ticker.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "EditorModeRegistry.h"
#include "ClothDesignEditorMode.h"
#include "ClothDesignStyle.h"

// This file was started using the Unreal Engine 5.5 Editor Mode C++ template.
// This template can be created directly in Unreal Engine in the plugins section.
//
// The template code has been modified and extended for this project.
// The unmodified template code is included at the bottom of the file for reference.

#define LOCTEXT_NAMESPACE "ClothDesignModule"


const FName FClothDesignModule::TwoDTabName(TEXT("TwoDWindowTab"));

void FClothDesignModule::StartupModule()
{
	FClothDesignStyle::Initialize();

	// This code will execute after the module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FClothDesignCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
	  FClothDesignCommands::Get().Open2DWindow,
	  FExecuteAction::CreateRaw(this, &FClothDesignModule::Spawn2DWindow),
	  FCanExecuteAction()
	);
	
	if (!FClothDesignCommands::Get().Open2DWindow.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Open2DWindow command NOT valid after Register()!"));
	}
	
	// static const FName TwoDTabName("TwoDWindowTab");
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TwoDTabName,
	  FOnSpawnTab::CreateRaw(this, &FClothDesignModule::OnSpawn2DWindowTab))
	  .SetDisplayName(LOCTEXT("TwoDTabTitle", "ClothDesign 2D Editor"))
	  .SetMenuType(ETabSpawnerMenuType::Hidden);

	
}

void FClothDesignModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up module.  For modules that support dynamic reloading,
	// call this function before unloading the module.
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TwoDTabName);
	UToolMenus::UnregisterOwner(this);

	FClothDesignCommands::Unregister();
	FClothDesignStyle::Shutdown();

}


bool FClothDesignModule::CheckSkeletonAssetExists()
{
	static const FString SkeletonPath = TEXT("/Game/ClothDesignAssets/SkelAsset/SK_ProcMesh.SK_ProcMesh");
	USkeleton* SkeletonAsset = LoadObject<USkeleton>(nullptr, *SkeletonPath);

	if (!SkeletonAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Skeleton asset not found at '%s'"), *SkeletonPath);

		FText DialogText = FText::FromString(
		"Could not load the skeleton asset.\n"
		"Please ensure all required files, including the skeleton, were copied into the project's Content folder during installation.");
		
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		return false;
	}

	return true;
}


void FClothDesignModule::Spawn2DWindow()
{
	if (!CheckSkeletonAssetExists())
	{
		// Skeleton missing, do not open the tab
		return;
	}
	
	FGlobalTabmanager::Get()->TryInvokeTab(FName("TwoDWindowTab"));
}

void FClothDesignModule::OnTabActivated(TSharedPtr<SDockTab> Tab, ETabActivationCause ActivationCause)
{
	if (CanvasWidget.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(CanvasWidget.ToSharedRef());
	}
}

TSharedRef<SWidget> FClothDesignModule::MakeObjectPicker(
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
		];
}


TSharedRef<SWidget> FClothDesignModule::MakeBackgroundControls()
{
    return SNew(SExpandableArea)
        .AreaTitle(LOCTEXT("BackgroundImageSection", "Background Image"))
        .InitiallyCollapsed(true) // starts hidden
        .BodyContent()
        [
            SNew(SVerticalBox)

            // === Image picker ===
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(4)
                [
                    SNew(STextBlock).Text(LOCTEXT("BGImageLabel", "Image:"))
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(4)
                [
                    SNew(SObjectPropertyEntryBox)
                    .AllowedClass(UTexture2D::StaticClass())
                    .ObjectPath_Lambda([this]() {
                        return CanvasWidget.IsValid()
                            ? CanvasWidget->GetSelectedTexturePath()
                            : FString();
                    })
                    .OnObjectChanged_Lambda([this](const FAssetData& Asset) {
                        if (CanvasWidget.IsValid())
                            CanvasWidget->OnBackgroundTextureSelected(Asset);
                    })
                ]
            ]

            // === Scale control ===
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(4)
                [
                    SNew(STextBlock).Text(LOCTEXT("BGScaleLabel", "Image Scale:"))
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(4)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this]() {
                        return CanvasWidget.IsValid()
                            ? CanvasWidget->GetBackgroundImageScale()
                            : TOptional<float>();
                    })
                    .OnValueChanged_Lambda([this](float NewVal) {
                        if (CanvasWidget.IsValid())
                            CanvasWidget->OnBackgroundImageScaleChanged(NewVal);
                    })
                    .MinValue(0.01f).MaxValue(10.0f)
                    .AllowSpin(true)
                ]
            ]
        ];
}



TSharedRef<SWidget> FClothDesignModule::MakeLoadSavePanel()
{
    return SNew(SExpandableArea)
        .AreaTitle(LOCTEXT("LoadSaveSection", "Save / Load"))
        .InitiallyCollapsed(true)
        .BodyContent()
        [

                SNew(SVerticalBox)

                + SVerticalBox::Slot().AutoHeight().Padding(2)
                [
                    MakeObjectPicker(
                        LOCTEXT("LoadLabel", "Load:"),
                        UClothShapeAsset::StaticClass(),
                        [this]() { return CanvasWidget->GetSelectedShapeAssetPath(); },
                        [this](auto Asset) { CanvasWidget->OnShapeAssetSelected(Asset); }
                    )
                ]
                
                + SVerticalBox::Slot().AutoHeight().Padding(4)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SNew(STextBlock).Text(LOCTEXT("SaveAsLabel", "Save As:"))
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.f).Padding(4,0)
                    [
                        SNew(SEditableTextBox)
                        .Text_Lambda([this]() { return FText::FromString(CurrentSaveName); })
                        .OnTextCommitted_Lambda([this](auto NewText, ETextCommit::Type) {
                            CurrentSaveName = NewText.ToString();
                        })
                    ]
                ]
                + SVerticalBox::Slot()
		        .AutoHeight()
		        .Padding(10)
		        .HAlign(HAlign_Left)
                [
                    SNew(SBox).WidthOverride(250.f)
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("SaveBtn", "Save"))
                        .OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnSaveClicked))
                    ]
                ]
        ];
}

TSharedRef<SWidget> FClothDesignModule::MakeActionButtons()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)

			// Generate Mesh
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			.HAlign(HAlign_Left)
			[
				SNew(SBox)
				.WidthOverride(250.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("GenerateMeshBtn", "Generate Meshes"))
					.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnGenerateMeshClicked))
				]
			]

			// Sewing
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			.HAlign(HAlign_Left)
			[
				SNew(SBox)
				.WidthOverride(250.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("SewingBtn", "Sewing"))
					.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnSewingClicked))
				]
			]
			// Merge meshes
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			.HAlign(HAlign_Left)
			[
				SNew(SBox)
				.WidthOverride(250.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("MergeMeshesBtn", "Merge Meshes"))
					.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnMergeMeshesClicked))
				]
			]

		];
}

TSharedRef<SWidget> FClothDesignModule::MakeClearButtons()
{
	return SNew(SVerticalBox)
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6)
		[
			// SNew(SBorder)
			// .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			// .Padding(8)
			// [
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10)
				.HAlign(HAlign_Left) // Optional: Align button to the left
				[
					SNew(SBox)
					.WidthOverride(250.f) // Set desired fixed width here
					[
						SNew(SButton)
						.Text(FText::FromString("Clear All"))
						.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnClearClicked))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10)
				.HAlign(HAlign_Left) // Optional: Align button to the left
				[
					SNew(SBox)
					.WidthOverride(250.f) // Set desired fixed width here
					[
						SNew(SButton)
						.Text(FText::FromString("Clear Sewing"))
						.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnClearSewingClicked))
					]
				]
			// ]
		];
	
}

TSharedRef<SWidget> FClothDesignModule::MakeModeButton(
		SClothDesignCanvas::EClothEditorMode InMode,
		const FText& InLabel)
{
	return SNew(SButton)
		.ButtonColorAndOpacity(
			TAttribute<FSlateColor>::CreateLambda([this, InMode]() {
				return GetModeButtonColor(InMode);
			}))
		.OnClicked_Lambda([this, InMode]() {
			if (CanvasWidget.IsValid())
			{
				CanvasWidget->OnModeButtonClicked(InMode);
			}
			return FReply::Handled();
		})
		.ContentPadding(FMargin(10, 5))
		[
			SNew(STextBlock)
			.Text(InLabel)
			.Font(FCoreStyle::GetDefaultFontStyle("Roboto", 11))
		];
}


TSharedRef<SWidget> FClothDesignModule::MakeModeToolbar()
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth().Padding(5)
		[
			MakeModeButton(
				SClothDesignCanvas::EClothEditorMode::Draw,
				LOCTEXT("Mode_Draw", "Draw")
			)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth().Padding(5)
		[
			MakeModeButton(
				SClothDesignCanvas::EClothEditorMode::Select,
				LOCTEXT("Mode_Edit", "Edit")
			)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth().Padding(5)
		[
			MakeModeButton(
				SClothDesignCanvas::EClothEditorMode::Sew,
				LOCTEXT("Mode_Sew", "Sew")
			)
		];
}

FSlateColor FClothDesignModule::GetModeButtonColor(SClothDesignCanvas::EClothEditorMode Mode) const
{
	if (CanvasWidget.IsValid() && CanvasWidget->GetCurrentMode() == Mode)
	{
		return FSlateColor(FLinearColor(0.686f, 1.f, 0.0f, 1.f)); // Highlighted
	}
	return FSlateColor(FLinearColor::White); // Default color
}

TSharedRef<SWidget> FClothDesignModule::MakeModeReminderBox()
{
	const FLinearColor ReminderColour(0.831, .0f, 1.f, 1.f);

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			// Reminder text, initially hidden
			SAssignNew(CanvasWidget->ModeReminderText, STextBlock)
			.Text(FText::FromString(""))
			.Visibility(EVisibility::Collapsed)
			.ColorAndOpacity(ReminderColour)
			.Font(FCoreStyle::GetDefaultFontStyle("Roboto", 11))

		];

}





TSharedRef<SDockTab> FClothDesignModule::OnSpawn2DWindowTab(const FSpawnTabArgs& Args)
{
	SAssignNew(CanvasWidget, SClothDesignCanvas);

	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()  // main layout
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(250.f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot().AutoHeight().Padding(4) [ SNew(SSeparator).Thickness(1.5f) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(2) [ MakeBackgroundControls() ]
						+ SVerticalBox::Slot().AutoHeight().Padding(4) [ SNew(SSeparator).Thickness(1.5f) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(10)[ MakeActionButtons() ]
						+ SVerticalBox::Slot().AutoHeight().Padding(4) [ SNew(SSeparator).Thickness(1.5f) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(6) [ MakeClearButtons() ]
						+ SVerticalBox::Slot().AutoHeight().Padding(4) [ SNew(SSeparator).Thickness(1.5f) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(6) [ MakeLoadSavePanel() ]
					]
				]

				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					CanvasWidget.ToSharedRef()
				]
			]

			+ SOverlay::Slot()  // top-right toolbar
			.VAlign(VAlign_Top).HAlign(HAlign_Right).Padding(10)
			[
				MakeModeToolbar()
			]
			
			+ SOverlay::Slot()  // top-right toolbar
			.VAlign(VAlign_Top).HAlign(HAlign_Left).Padding(FMargin(260, 20, 0, 0))  // Left=300, Top=10, Right=0, Bottom=0

			[
				MakeModeReminderBox()
			]
			
		];

	// Delay focus until next tick/frame to ensure UI is ready
	//FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, NewTab](float DeltaTime)
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float)
	{
		if (CanvasWidget.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(CanvasWidget.ToSharedRef());
		}
		return false; // don't keep ticking
	}));
	
	return NewTab;
}




FReply FClothDesignModule::OnGenerateMeshClicked()
{
	if (CanvasWidget.IsValid())
	{
		CanvasWidget->GenerateMeshesClick();
	}
	return FReply::Handled();
}



FReply FClothDesignModule::OnSewingClicked()
{
	if (CanvasWidget.IsValid())
	{
		CanvasWidget->SewingClick();
	}
	return FReply::Handled();
}

FReply FClothDesignModule::OnMergeMeshesClicked()
{
	if (CanvasWidget.IsValid())
	{
		CanvasWidget->MergeClick();
	}
	return FReply::Handled();
}

FReply FClothDesignModule::OnSaveClicked()
{
	if (CanvasWidget.IsValid())
	{
		CanvasWidget->SaveClick(CurrentSaveName);
	}
	return FReply::Handled();
}



FReply FClothDesignModule::OnClearClicked()
{
	// ClearAllShapeData
	if (CanvasWidget.IsValid())
	{
		CanvasWidget->ClearAllShapeData();
	}
	return FReply::Handled();
}

FReply FClothDesignModule::OnClearSewingClicked()
{
	if (CanvasWidget.IsValid())
	{
		CanvasWidget->ClearAllSewing();

	}
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FClothDesignModule, ClothDesign)


/*
------------------------------------------
Original Unreal Engine Template Code
------------------------------------------
// Copyright Epic Games, Inc. All Rights Reserved.

#include "NewEditorModeModule.h"
#include "NewEditorModeEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "NewEditorModeModule"

void FNewEditorModeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FNewEditorModeEditorModeCommands::Register();
}

void FNewEditorModeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FNewEditorModeEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNewEditorModeModule, NewEditorModeEditorMode)
*/
