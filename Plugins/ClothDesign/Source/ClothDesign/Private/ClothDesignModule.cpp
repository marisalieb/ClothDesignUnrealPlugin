
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
#include "CanvasAssets.h"


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
	  .SetDisplayName(LOCTEXT("TwoDTabTitle", "ClothDesign 2D Editor"))
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




void FClothDesignModule::OnTabActivated(TSharedPtr<SDockTab> Tab, ETabActivationCause ActivationCause)
{
	if (CanvasWidget.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(CanvasWidget.ToSharedRef());
	}
}

TSharedRef<SDockTab> FClothDesignModule::OnSpawn2DWindowTab(const FSpawnTabArgs& Args)
{
	SAssignNew(CanvasWidget, SClothDesignCanvas);
	
	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
	// SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SHorizontalBox)
				// — Left column: UI panel, fixed width
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(250)             // whatever width you like
					[
						SNew(SVerticalBox)
						
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(4)
						[
							SNew(SSeparator)
							.Thickness(1.5f)
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
						
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(4)
						[
							SNew(SSeparator)
							.Thickness(1.5f)
						]
					
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(6)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
							.Padding(8)
							[
								SNew(SVerticalBox)
								

								// Load Asset
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
										.Text(FText::FromString("Load:"))
									]

									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(4)
									[
										SNew(SObjectPropertyEntryBox)
										.AllowedClass(UClothShapeAsset::StaticClass())
										.ObjectPath_Lambda([this]()
										{
											if (this->CanvasWidget.IsValid())
											{
												return this->CanvasWidget->GetSelectedShapeAssetPath();
											}
											return FString();
										})
										.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
										{
											if (this->CanvasWidget.IsValid())
											{
												this->CanvasWidget->OnShapeAssetSelected(AssetData);
											}
										})
									]
								]

								// Save As Text Field
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(4)
								[
									SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(FText::FromString("Save As:"))
									]

									+ SHorizontalBox::Slot()
									.FillWidth(1.f)
									.Padding(4, 0)
									[
										SNew(SEditableTextBox)
										.Text_Lambda([this]() { return FText::FromString(CurrentSaveName); })
										.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
										{
											CurrentSaveName = NewText.ToString();
										})
									]
								]

								// Save Button
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(10)
								.HAlign(HAlign_Left)
								[
									SNew(SBox)
									.WidthOverride(240.f)
									[
										SNew(SButton)
										.Text(FText::FromString("Save"))
										.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnSaveClicked))
									]
								]
							]
						]


						
						
						
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(4)
						[
							SNew(SSeparator)
							.Thickness(1.5f)
						]


						
						// generate mesh Button
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
						// sewing Button
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(10)
						.HAlign(HAlign_Left) // Optional: Align button to the left
						[
							SNew(SBox)
							.WidthOverride(150.f) // Set desired fixed width here
							[
								SNew(SButton)
								.Text(FText::FromString("Sewing"))
								.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnSewingClicked))
							]
						]
						

						
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(4)
						[
							SNew(SSeparator)
							.Thickness(1.5f)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(6)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
							.Padding(8)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(10)
								.HAlign(HAlign_Left) // Optional: Align button to the left
								[
									SNew(SBox)
									.WidthOverride(150.f) // Set desired fixed width here
									[
										SNew(SButton)
										.Text(FText::FromString("Clear All"))
										.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnClearClicked))
									]
								]
							]
						]
					]
				]
				
				// Canvas
				// + SVerticalBox::Slot().FillHeight(1.0f)
				// — Right column: the Canvas, filling remaining space
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					CanvasWidget.ToSharedRef()  // Use the already-created CanvasWidget
				]
			]

			
		+ SOverlay::Slot()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Right)
		.Padding(10)
		[
		    SNew(SHorizontalBox)

		    // Draw Button
		    + SHorizontalBox::Slot()
		    .AutoWidth()
		    .Padding(5)
		    [
		        SNew(SButton)
		        // .Text(FText::FromString("Draw"))
		        .ButtonColorAndOpacity(
		            TAttribute<FSlateColor>::CreateLambda([this]() {
		                return GetModeButtonColor(SClothDesignCanvas::EClothEditorMode::Draw);
		            }))
		        .OnClicked_Lambda([this]() {
		            if (CanvasWidget.IsValid()) {
		                CanvasWidget->OnModeButtonClicked(SClothDesignCanvas::EClothEditorMode::Draw);
		            }
		            return FReply::Handled();
		        })
		        .ContentPadding(FMargin(10, 5)) // makes it bigger
		    	[
					SNew(STextBlock)
					.Text(FText::FromString("Draw"))
					.Font(FCoreStyle::GetDefaultFontStyle("Roboto", 11)) 
				]
		    ]

		    // Edit Button
		    + SHorizontalBox::Slot()
		    .AutoWidth()
		    .Padding(5)
		    [
		        SNew(SButton)
		        //.Text(FText::FromString("Edit"))
		        .ButtonColorAndOpacity(
		            TAttribute<FSlateColor>::CreateLambda([this]() {
		                return GetModeButtonColor(SClothDesignCanvas::EClothEditorMode::Select);
		            }))
		        .OnClicked_Lambda([this]() {
		            if (CanvasWidget.IsValid()) {
		                CanvasWidget->OnModeButtonClicked(SClothDesignCanvas::EClothEditorMode::Select);
		            }
		            return FReply::Handled();
		        })
		        .ContentPadding(FMargin(10, 5))
		    	[
					SNew(STextBlock)
					.Text(FText::FromString("Edit"))
					.Font(FCoreStyle::GetDefaultFontStyle("Roboto", 11)) 
				]
		    ]

		    // Sew Button
		    + SHorizontalBox::Slot()
		    .AutoWidth()
		    .Padding(5)
		    [
		        SNew(SButton)
		        //.Text(FText::FromString("Sew"))
		        .ButtonColorAndOpacity(
		            TAttribute<FSlateColor>::CreateLambda([this]() {
		                return GetModeButtonColor(SClothDesignCanvas::EClothEditorMode::Sew);
		            }))
		        .OnClicked_Lambda([this]() {
		            if (CanvasWidget.IsValid()) {
		                CanvasWidget->OnModeButtonClicked(SClothDesignCanvas::EClothEditorMode::Sew);
		            }
		            return FReply::Handled();
		        })
		        .ContentPadding(FMargin(10, 5))
			    [
			    	SNew(STextBlock)
					.Text(FText::FromString("Sew"))
					.Font(FCoreStyle::GetDefaultFontStyle("Roboto", 11)) 
			    ]
		    ]
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

FSlateColor FClothDesignModule::GetModeButtonColor(SClothDesignCanvas::EClothEditorMode Mode) const
{
	if (CanvasWidget.IsValid() && CanvasWidget->GetCurrentMode() == Mode)
	{
		return FSlateColor(FLinearColor(0.686f, 1.f, 0.0f, 1.f)); // Highlighted
	}
	return FSlateColor(FLinearColor::White); // Default color
}


FReply FClothDesignModule::OnGenerateMeshClicked()
{
	if (CanvasWidget.IsValid())
	{
		//CanvasWidget->TriangulateAndBuildMesh();
		// CanvasWidget->TriangulateAndBuildAllMeshes();
		//CanvasWidget->BuildAndAlignClickedSeam(); MergeLastTwoMeshes
		CanvasWidget->MergeLastTwoMeshes();
		// CanvasWidget->MergeAndWeldLastTwoMeshes();

	}
	return FReply::Handled();
}

FReply FClothDesignModule::OnSewingClicked()
{
	if (CanvasWidget.IsValid())
	{
		//CanvasWidget->TriangulateAndBuildMesh();
		// CanvasWidget->SewingStart();
		CanvasWidget->BuildAndAlignClickedSeam();

	}
	return FReply::Handled();
}


// FReply FClothDesignModule::OnSaveClicked()
// {
// 	if (CanvasWidget.IsValid())
// 	{
// 		if (CurrentSaveName.IsEmpty())
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("Please enter a name before saving."));
// 		}
// 		else
// 		{
// 			bool bOK = CanvasWidget->SaveShapeAsset(TEXT("SavedClothMeshes"), CurrentSaveName);
// 			UE_LOG(LogTemp, Log, TEXT("Save %s: %s"), *CurrentSaveName, bOK ? TEXT("Success") : TEXT("FAILED"));
// 		}
// 	}
// 	return FReply::Handled();
// }

FReply FClothDesignModule::OnSaveClicked()
{
	if (!CanvasWidget.IsValid())
	{
		return FReply::Handled();
	}

	if (CurrentSaveName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Please enter a name before saving."));
		return FReply::Handled();
	}

	// Extract required data from the canvas
	TArray<FInterpCurve<FVector2D>> CompletedShapes = CanvasWidget->GetCompletedShapes();
	TArray<TArray<bool>> CompletedBezierFlags = CanvasWidget->GetCompletedBezierFlags();
	FInterpCurve<FVector2D> CurvePoints = CanvasWidget->GetCurrentCurvePoints();
	TArray<bool> bUseBezierPerPoint = CanvasWidget->GetCurrentBezierFlags();

	// Save
	bool bOK = FCanvasAssets::SaveShapeAsset(
		TEXT("SavedClothMeshes"),
		CurrentSaveName,
		CompletedShapes,
		CompletedBezierFlags,
		CurvePoints,
		bUseBezierPerPoint
	);

	UE_LOG(LogTemp, Log, TEXT("Save %s: %s"), *CurrentSaveName, bOK ? TEXT("Success") : TEXT("FAILED"));

	return FReply::Handled();
}



FReply FClothDesignModule::OnClearClicked()
{
	// ClearCurrentShapeData
	if (CanvasWidget.IsValid())
	{
		CanvasWidget->ClearCurrentShapeData();

	}
	return FReply::Handled();
}



void FClothDesignModule::SaveCurrentShapesToAsset()
{
	UClothShapeAsset* NewAsset = NewObject<UClothShapeAsset>(GetTransientPackage(), UClothShapeAsset::StaticClass(), NAME_None, RF_Public | RF_Standalone);

	// Fill NewAsset->Shapes with your data
	// ...

	FString PackageName = TEXT("/Game/Cloth/GeneratedShapeAsset");
	FString AssetName = TEXT("GeneratedShapeAsset");

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UObject* FinalAsset = AssetToolsModule.Get().CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), UClothShapeAsset::StaticClass(), nullptr);
    
	// Copy data into FinalAsset if needed (or use a custom factory)
}




#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FClothDesignModule, ClothDesignEditorMode)