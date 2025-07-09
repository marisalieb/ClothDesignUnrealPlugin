// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/BaseToolkit.h"
#include "ClothDesignEditorMode.h"

#include "PropertyCustomizationHelpers.h"
#include "GameFramework/Actor.h"

/**
 * This FModeToolkit just creates a basic UI panel that allows various InteractiveTools to
 * be initialized, and a DetailsView used to show properties of the active Tool.
 */
class FClothDesignEditorModeToolkit : public FModeToolkit
{
public:
	FClothDesignEditorModeToolkit();

	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;

	// custom UI
	virtual TSharedPtr<SWidget> GetInlineContent() const override;

	FReply OnOpen2DWindowClicked();

	
	// void OnClothObjectPicked(const FAssetData& AssetData);
	// TWeakObjectPtr<AActor> ClothActor;
	// TWeakObjectPtr<UObject> SelectedClothObject;

	// objects for pickers: collision body mesh, cloth, object, material for fabric
	TWeakObjectPtr<USkeletalMesh> SelectedSkeletalMesh;
	TWeakObjectPtr<USkeletalMesh> SelectedClothMesh;
	TWeakObjectPtr<UMaterialInterface> SelectedTextileMaterial;

	// functions for each picker
	FString GetSelectedSkeletalMeshPath() const;
	void OnSkeletalMeshSelected(const FAssetData& AssetData);

	FString GetSelectedClothMeshPath() const;
	void OnClothMeshSelected(const FAssetData& AssetData);

	FString GetSelectedTextileMaterialPath() const;
	void OnTextileMaterialSelected(const FAssetData& AssetData);
	
private:
	TSharedPtr<SWidget> ToolkitWidget;
};
