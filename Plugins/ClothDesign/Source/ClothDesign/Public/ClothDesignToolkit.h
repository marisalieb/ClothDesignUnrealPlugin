#pragma once

#include "Toolkits/BaseToolkit.h"
#include "ClothDesignEditorMode.h"
#include "GameFramework/Actor.h"


class FClothDesignToolkit : public FModeToolkit
{
public:
	FClothDesignToolkit();

	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;

	// custom UI
	virtual TSharedPtr<SWidget> GetInlineContent() const override;

	
private:
	TSharedPtr<SWidget> ToolkitWidget;

	// objects for pickers: collision body mesh, cloth, object, material for fabric
	TWeakObjectPtr<USkeletalMesh> SelectedSkeletalMesh;
	TWeakObjectPtr<USkeletalMesh> SelectedClothMesh;
	TWeakObjectPtr<UMaterialInterface> SelectedTextileMaterial;

	FReply OnOpen2DWindowClicked();

	TSharedRef<SWidget> MakeOpen2DButton();
	
	TSharedRef<SWidget> MakeObjectPicker(
		const FText& LabelText,
		const UClass* AllowedClass,
		TFunction<FString()> GetPath,
		TFunction<void(const FAssetData&)> OnChanged);
	
	// functions for each picker
	FString GetSelectedSkeletalMeshPath() const;
	void OnSkeletalMeshSelected(const FAssetData& AssetData);

	FString GetSelectedClothMeshPath() const;
	void OnClothMeshSelected(const FAssetData& AssetData);

	FString GetSelectedTextileMaterialPath() const;
	void OnTextileMaterialSelected(const FAssetData& AssetData);
};
