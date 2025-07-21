// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClothDesignModule.h"
#include "ClothDesignEditorModeCommands.h"
#include "SClothDesignCanvas.h"
#include "ClothDesignStyle.h"
#include "EditorModeManager.h"
#include "ClothDesignEditorMode.h"
#include "EditorModeRegistry.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Application/SlateApplication.h"
#include "Tickable.h"
#include "Containers/Ticker.h"
#include "PropertyCustomizationHelpers.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "ClothDesignModule"


const FName FClothDesignModule::TwoDTabName(TEXT("TwoDWindowTab"));

void FClothDesignModule::StartupModule()
{

	// This code will execute after module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FClothDesignEditorModeCommands::Register();
	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
	  FClothDesignEditorModeCommands::Get().Open2DWindow,
	  FExecuteAction::CreateRaw(this, &FClothDesignModule::Spawn2DWindow),
	  FCanExecuteAction()
	);
	
	if (!FClothDesignEditorModeCommands::Get().Open2DWindow.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Open2DWindow command NOT valid after Register()!"));
	}
	
	// UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FYourEditorModeModule::RegisterMenus));

	// static const FName TwoDTabName("TwoDWindowTab");
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TwoDTabName,
	  FOnSpawnTab::CreateRaw(this, &FClothDesignModule::OnSpawn2DWindowTab))
	  .SetDisplayName(LOCTEXT("TwoDTabTitle", "2D Editor"))
	  .SetMenuType(ETabSpawnerMenuType::Hidden);


}

void FClothDesignModule::ShutdownModule()
{

	
	// This function may be called during shutdown to clean up module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TwoDTabName);
	UToolMenus::UnregisterOwner(this);

	FClothDesignEditorModeCommands::Unregister();
	
}


void FClothDesignModule::Spawn2DWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FName("TwoDWindowTab"));
}


// TSharedRef<SDockTab> FClothDesignModule::OnSpawn2DWindowTab(const FSpawnTabArgs& Args)
// {
// 	return SNew(SDockTab)
// 		.TabRole(ETabRole::NomadTab)
// 		[
// 			SNew(SClothDesignCanvas) 
// 		];
// }


void FClothDesignModule::OnTabActivated(TSharedPtr<SDockTab> Tab, ETabActivationCause ActivationCause)
{
	if (CanvasWidget.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(CanvasWidget.ToSharedRef());
	}
}

TSharedRef<SDockTab> FClothDesignModule::OnSpawn2DWindowTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
	// SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)

			// Button
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			.HAlign(HAlign_Left) // Optional: Align button to the left
			[
				SNew(SBox)
				.WidthOverride(150.f) // Set desired fixed width here
				[
					SNew(SButton)
					.Text(FText::FromString("Generate Mesh"))
					.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnGenerateMeshClicked))
				]
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(SHorizontalBox)
			
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Background Image:"))
				]
			
				// + SHorizontalBox::Slot()
				// .AutoWidth()
				// .Padding(4)
				// [
				// 	SNew(SObjectPropertyEntryBox)
				// 	.AllowedClass(UTexture2D::StaticClass())
				// 	.ObjectPath(CanvasWidget.ToSharedRef(), &SClothDesignCanvas::GetSelectedTexturePath)
				// 	.OnObjectChanged(CanvasWidget.ToSharedRef(), &SClothDesignCanvas::OnBackgroundTextureSelected)
				// ]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UTexture2D::StaticClass())
					.ObjectPath_Lambda([this]()
					{
						if (this->CanvasWidget.IsValid())
						{
							return this->CanvasWidget->GetSelectedTexturePath();
						}
						return FString();
					})
					.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
					{
						if (this->CanvasWidget.IsValid())
						{
							this->CanvasWidget->OnBackgroundTextureSelected(AssetData);
						}
					})
				]

			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(SHorizontalBox)
			
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Background Image Scale:")))
				]
			
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4)
				[
					SNew(SNumericEntryBox<float>)
					
					// .Value(CanvasWidget.ToSharedRef(), &SClothDesignCanvas::GetBackgroundImageScale)
					// .OnValueChanged(CanvasWidget.ToSharedRef(), &SClothDesignCanvas::OnBackgroundImageScaleChanged)
					// .Value_Lambda([this]() -> TOptional<float>
					// {
					// 	return CanvasWidget.IsValid() ? CanvasWidget->GetBackgroundImageScale() : 1.0f;
					// })
					// .OnValueChanged_Lambda([this](float NewValue)
					// {
					// 	if (CanvasWidget.IsValid())
					// 	{
					// 		CanvasWidget->OnBackgroundImageScaleChanged(NewValue);
					// 	}
					// })
					.Value_Lambda([this]() -> TOptional<float>
						{
							if (CanvasWidget.IsValid())
							{
								return CanvasWidget->GetBackgroundImageScale();
							}
							return TOptional<float>();
						})
						.OnValueChanged_Lambda([this](float NewValue)
						{
							if (CanvasWidget.IsValid())
							{
								CanvasWidget->OnBackgroundImageScaleChanged(NewValue);
							}
						})
					
					.MinValue(0.1f)
					.MaxValue(10.0f)
					.MinSliderValue(0.1f)
					.MaxSliderValue(10.0f)
					.AllowSpin(true)
					.LabelPadding(FMargin(0))
				]
			]


			// Canvas
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(CanvasWidget, SClothDesignCanvas)
			]
			

		];
	
	// Delay focus until next tick/frame to ensure UI is ready
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, NewTab](float DeltaTime)
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
		CanvasWidget->TriangulateAndBuildMesh();
	}
	return FReply::Handled();
}


// TSharedRef<SDockTab> FClothDesignModule::OnSpawn2DWindowTab(const FSpawnTabArgs& Args)
// {
// 	return SNew(SDockTab)
// 	  .TabRole(ETabRole::NomadTab)
// 	  [
// 		SNew(SBox)
// 		.HAlign(HAlign_Center)
// 		.VAlign(VAlign_Center)
// 		[
// 		  SNew(STextBlock)
// 		  .Text(LOCTEXT("TwoDWindowText", "This is the 2D pattern editor window"))
// 		]
// 	  ];
// }

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FClothDesignModule, ClothDesignEditorMode)