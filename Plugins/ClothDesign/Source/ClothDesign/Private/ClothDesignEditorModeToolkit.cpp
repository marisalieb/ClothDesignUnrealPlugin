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


	// // title of the editor mode
	// + SVerticalBox::Slot()
	// .AutoHeight()
	// .Padding(4)
	// [
	// 	SNew(STextBlock)
	// 	.Text(FText::FromString("Cloth Design Editor"))
	// ]
	//
	// open 2d button
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		// .AutoHeight()
		.FillHeight(1.0f)
		[
			SNew(SButton)
			.Text(FText::FromString("Open 2D Window"))
			.OnClicked(FOnClicked::CreateSP(this, &FClothDesignEditorModeToolkit::OnOpen2DWindowClicked))

		]
	]

	// collision body picker
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Body Object:"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(USkeletalMesh::StaticClass()) // or AActor::StaticClass()
			.ObjectPath(this, &FClothDesignEditorModeToolkit::GetSelectedSkeletalMeshPath)
			.OnObjectChanged(this, &FClothDesignEditorModeToolkit::OnSkeletalMeshSelected)
		]
	]
	
	// cloth object picker
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Cloth Object:"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(USkeletalMesh::StaticClass()) // or AActor::StaticClass()
			.ObjectPath(this, &FClothDesignEditorModeToolkit::GetSelectedClothMeshPath)
			.OnObjectChanged(this, &FClothDesignEditorModeToolkit::OnClothMeshSelected)
		]
	]

	// cloth material picker
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
			.Text(FText::FromString("Cloth Material:"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UMaterialInterface::StaticClass())
			.ObjectPath(this, &FClothDesignEditorModeToolkit::GetSelectedTextileMaterialPath)
			.OnObjectChanged(this, &FClothDesignEditorModeToolkit::OnTextileMaterialSelected)
		]
	]

	// save button
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		// .AutoHeight()
		.FillHeight(1.0f)
		[
			SNew(SButton)
			.Text(FText::FromString("Save Pattern")) // needs additional functionality
		]
	]
		;
	
}


FString FClothDesignEditorModeToolkit::GetSelectedSkeletalMeshPath() const
{
	return SelectedSkeletalMesh.IsValid() ? SelectedSkeletalMesh->GetPathName() : FString();
}

void FClothDesignEditorModeToolkit::OnSkeletalMeshSelected(const FAssetData& AssetData)
{
	SelectedSkeletalMesh = Cast<USkeletalMesh>(AssetData.GetAsset());
}

FString FClothDesignEditorModeToolkit::GetSelectedClothMeshPath() const
{
	return SelectedClothMesh.IsValid() ? SelectedClothMesh->GetPathName() : FString();
}

void FClothDesignEditorModeToolkit::OnClothMeshSelected(const FAssetData& AssetData)
{
	SelectedClothMesh = Cast<USkeletalMesh>(AssetData.GetAsset());
}

FString FClothDesignEditorModeToolkit::GetSelectedTextileMaterialPath() const
{
	return SelectedTextileMaterial.IsValid() ? SelectedTextileMaterial->GetPathName() : FString();
}

void FClothDesignEditorModeToolkit::OnTextileMaterialSelected(const FAssetData& AssetData)
{
	SelectedTextileMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());
}






/*
void FClothDesignEditorModeToolkit::OnClothObjectPicked(const FAssetData& AssetData)
{
	UObject* PickedObject = AssetData.GetAsset();
	if (AActor* PickedActor = Cast<AActor>(PickedObject))
	{
		ClothActor = PickedActor;
		UE_LOG(LogTemp, Log, TEXT("Selected Cloth Actor: %s"), *PickedActor->GetName());
	}
} 

*/

void FClothDesignEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}

TSharedPtr<SWidget> FClothDesignEditorModeToolkit::GetInlineContent() const
{
	return ToolkitWidget;
}



FName FClothDesignEditorModeToolkit::GetToolkitFName() const
{
	return FName("ClothDesignEditorMode");
}

FText FClothDesignEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "ClothDesignEditorMode Toolkit");
}






FReply FClothDesignEditorModeToolkit::OnOpen2DWindowClicked()
{
	static const FName TwoDTabName("TwoDWindowTab"); // change name here

	FGlobalTabmanager::Get()->TryInvokeTab(TwoDTabName);
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
