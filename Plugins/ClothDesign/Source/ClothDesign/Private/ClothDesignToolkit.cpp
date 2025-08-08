// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClothDesignToolkit.h"
#include "ClothDesignEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "ClothDesignEditorModeToolkit"

FClothDesignToolkit::FClothDesignToolkit()
{
}


// helper functions to imrpove readability of the main ui init function
TSharedRef<SWidget> FClothDesignToolkit::MakeOpen2DButton()
{
	return SNew(SButton)
		.Text(FText::FromString("Open 2D Window"))
		.OnClicked(FOnClicked::CreateSP(this, &FClothDesignToolkit::OnOpen2DWindowClicked));
}

TSharedRef<SWidget> FClothDesignToolkit::MakeObjectPicker(
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
			.Text(FText::FromString("Save Pattern"))
			// .OnClicked(...) // hook up when ready
		];
	// + SVerticalBox::Slot()
	// .AutoHeight()
	// .Padding(4)
	// [
	// 	SNew(SVerticalBox)
	// 	+ SVerticalBox::Slot()
	// 	// .AutoHeight()
	// 	.FillHeight(1.0f)
	// 	[
	// 		SNew(SButton)
	// 		.Text(FText::FromString("Open 2D Window"))
	// 		.OnClicked(FOnClicked::CreateSP(this, &FClothDesignToolkit::OnOpen2DWindowClicked))
	//
	// 	]
	// ]

	// // collision body picker
	// + SVerticalBox::Slot()
	// .AutoHeight()
	// .Padding(4)
	// [
	// 	SNew(SHorizontalBox)
	//
	// 	+ SHorizontalBox::Slot()
	// 	.AutoWidth()
	// 	.Padding(4)
	// 	[
	// 		SNew(STextBlock)
	// 		.Text(FText::FromString("Body Object:"))
	// 	]
	//
	// 	+ SHorizontalBox::Slot()
	// 	.AutoWidth()
	// 	.Padding(4)
	// 	[
	// 		SNew(SObjectPropertyEntryBox)
	// 		.AllowedClass(USkeletalMesh::StaticClass()) // or AActor::StaticClass()
	// 		.ObjectPath(this, &FClothDesignToolkit::GetSelectedSkeletalMeshPath)
	// 		.OnObjectChanged(this, &FClothDesignToolkit::OnSkeletalMeshSelected)
	// 	]
	// ]
	//
	// // cloth object picker
	// + SVerticalBox::Slot()
	// .AutoHeight()
	// .Padding(4)
	// [
	// 	SNew(SHorizontalBox)
	//
	// 	+ SHorizontalBox::Slot()
	// 	.AutoWidth()
	// 	.Padding(4)
	// 	[
	// 		SNew(STextBlock)
	// 		.Text(FText::FromString("Cloth Object:"))
	// 	]
	//
	// 	+ SHorizontalBox::Slot()
	// 	.AutoWidth()
	// 	.Padding(4)
	// 	[
	// 		SNew(SObjectPropertyEntryBox)
	// 		.AllowedClass(USkeletalMesh::StaticClass()) // or AActor::StaticClass()
	// 		.ObjectPath(this, &FClothDesignToolkit::GetSelectedClothMeshPath)
	// 		.OnObjectChanged(this, &FClothDesignToolkit::OnClothMeshSelected)
	// 	]
	// ]
	//
	// // cloth material picker
	// + SVerticalBox::Slot()
	// .AutoHeight()
	// .Padding(2)
	// [
	// 	SNew(SHorizontalBox)
	//
	// 	+ SHorizontalBox::Slot()
	// 	.AutoWidth()
	// 	.Padding(4)
	// 	[
	// 		SNew(STextBlock)
	// 		.Text(FText::FromString("Cloth Material:"))
	// 	]
	//
	// 	+ SHorizontalBox::Slot()
	// 	.AutoWidth()
	// 	.Padding(4)
	// 	[
	// 		SNew(SObjectPropertyEntryBox)
	// 		.AllowedClass(UMaterialInterface::StaticClass())
	// 		.ObjectPath(this, &FClothDesignToolkit::GetSelectedTextileMaterialPath)
	// 		.OnObjectChanged(this, &FClothDesignToolkit::OnTextileMaterialSelected)
	// 	]
	// ]

	// // save button
	// + SVerticalBox::Slot()
	// .AutoHeight()
	// .Padding(4)
	// [
	// 	SNew(SVerticalBox)
	// 	+ SVerticalBox::Slot()
	// 	// .AutoHeight()
	// 	.FillHeight(1.0f)
	// 	[
	// 		SNew(SButton)
	// 		.Text(FText::FromString("Save Pattern")) // needs additional functionality
	// 	]
	// ]
	// 	;
	
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
}

FString FClothDesignToolkit::GetSelectedTextileMaterialPath() const
{
	return SelectedTextileMaterial.IsValid() ? SelectedTextileMaterial->GetPathName() : FString();
}

void FClothDesignToolkit::OnTextileMaterialSelected(const FAssetData& AssetData)
{
	SelectedTextileMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());
}






/*
void FClothDesignToolkit::OnClothObjectPicked(const FAssetData& AssetData)
{
	UObject* PickedObject = AssetData.GetAsset();
	if (AActor* PickedActor = Cast<AActor>(PickedObject))
	{
		ClothActor = PickedActor;
		UE_LOG(LogTemp, Log, TEXT("Selected Cloth Actor: %s"), *PickedActor->GetName());
	}
} 

*/

void FClothDesignToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}

TSharedPtr<SWidget> FClothDesignToolkit::GetInlineContent() const
{
	return ToolkitWidget;
}



FName FClothDesignToolkit::GetToolkitFName() const
{
	return FName("ClothDesignEditorMode");
}

FText FClothDesignToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "ClothDesignEditorMode Toolkit");
}






FReply FClothDesignToolkit::OnOpen2DWindowClicked()
{
	static const FName TwoDTabName("TwoDWindowTab"); // change name here

	FGlobalTabmanager::Get()->TryInvokeTab(TwoDTabName);
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
