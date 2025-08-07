
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
#include "Canvas/CanvasAssets.h"


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

TSharedRef<SWidget> FClothDesignModule::MakeObjectPicker(
	const FText& LabelText,
	UClass* AllowedClass,
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
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SHorizontalBox)
            
			+ SHorizontalBox::Slot().AutoWidth().Padding(4)
			[
				SNew(STextBlock).Text(LOCTEXT("BGImageLabel","Background Image:"))
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
				.OnObjectChanged_Lambda([this](const FAssetData& Asset){
					if (CanvasWidget.IsValid())
						CanvasWidget->OnBackgroundTextureSelected(Asset);
				})
			]

		]   

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SHorizontalBox)
            
			+ SHorizontalBox::Slot().AutoWidth().Padding(4)
			[
				SNew(STextBlock).Text(LOCTEXT("BGScaleLabel","Background Image Scale:"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4)
			[
				SNew(SNumericEntryBox<float>)
				.Value_Lambda([this]() {
					return CanvasWidget.IsValid()
						? CanvasWidget->GetBackgroundImageScale()
						: TOptional<float>();
				})
				.OnValueChanged_Lambda([this](float NewVal){
					if (CanvasWidget.IsValid())
						CanvasWidget->OnBackgroundImageScaleChanged(NewVal);
				})
				.MinValue(0.1f).MaxValue(10.0f)
				.AllowSpin(true)
			]
		];
}

TSharedRef<SWidget> FClothDesignModule::MakeLoadSavePanel()
{ 
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().Padding(2)
			[
				MakeObjectPicker(
					LOCTEXT("LoadLabel","Load:"),
					UClothShapeAsset::StaticClass(),
					[this](){ return CanvasWidget->GetSelectedShapeAssetPath(); },
					[this](auto Asset){ CanvasWidget->OnShapeAssetSelected(Asset); }
				)
			]
			
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				  [ SNew(STextBlock).Text(LOCTEXT("SaveAsLabel","Save As:")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4,0)
				  [ SNew(SEditableTextBox)
					.Text_Lambda([this](){ return FText::FromString(CurrentSaveName); })
					.OnTextCommitted_Lambda([this](auto NewText, ETextCommit::Type){
						CurrentSaveName = NewText.ToString();
					})
				  ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10).HAlign(HAlign_Left)
			[
				SNew(SBox).WidthOverride(240.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("SaveBtn","Save"))
					.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnSaveClicked))
				]
			]
		];
}

TSharedRef<SWidget> FClothDesignModule::MakeActionButtons()
{
	return SNew(SVerticalBox)

		// Generate Mesh
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			.WidthOverride(150.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("GenerateMeshBtn", "Generate Mesh"))
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
			.WidthOverride(150.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("SewingBtn", "Sewing"))
				.OnClicked(FOnClicked::CreateRaw(this, &FClothDesignModule::OnSewingClicked))
			]
		];
}

TSharedRef<SWidget> FClothDesignModule::MakeClearAllButton()
{
	return SNew(SVerticalBox)
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
						+ SVerticalBox::Slot().AutoHeight().Padding(6) [ MakeLoadSavePanel() ]
						+ SVerticalBox::Slot().AutoHeight().Padding(4) [ SNew(SSeparator).Thickness(1.5f) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(10)[ MakeActionButtons() ]
						+ SVerticalBox::Slot().AutoHeight().Padding(4) [ SNew(SSeparator).Thickness(1.5f) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(6) [ MakeClearAllButton() ]
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
	
	FString PackageName = TEXT("/Game/Cloth/GeneratedShapeAsset");
	FString AssetName = TEXT("GeneratedShapeAsset");

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UObject* FinalAsset = AssetToolsModule.Get().CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), UClothShapeAsset::StaticClass(), nullptr);
    
}


#undef LOCTEXT_NAMESPACE

// IMPLEMENT_MODULE(FClothDesignModule, ClothDesignEditorMode)
IMPLEMENT_MODULE(FClothDesignModule, ClothDesign)