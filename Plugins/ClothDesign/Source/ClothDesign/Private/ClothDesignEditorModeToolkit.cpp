// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClothDesignEditorModeToolkit.h"
#include "ClothDesignEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "ClothDesignEditorModeToolkit"

FClothDesignEditorModeToolkit::FClothDesignEditorModeToolkit()
{
}

void FClothDesignEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);

	// custom UI
	ToolkitWidget = SNew(SVerticalBox)

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4)
	[
		SNew(STextBlock)
		.Text(FText::FromString("Cloth Design Editor"))
	]

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Open 2D Window"))
			.OnClicked(FOnClicked::CreateSP(this, &FClothDesignEditorModeToolkit::OnOpen2DWindowClicked))

		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Save Pattern")) // needs additional functionality
		]
	];
}

void FClothDesignEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FClothDesignEditorModeToolkit::GetToolkitFName() const
{
	return FName("ClothDesignEditorMode");
}

FText FClothDesignEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "ClothDesignEditorMode Toolkit");
}

TSharedPtr<SWidget> FClothDesignEditorModeToolkit::GetInlineContent() const
{
	return ToolkitWidget;
}


FReply FClothDesignEditorModeToolkit::OnOpen2DWindowClicked()
{
	static const FName TwoDTabName("TwoDWindowTab"); // change name here

	FGlobalTabmanager::Get()->TryInvokeTab(TwoDTabName);
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
