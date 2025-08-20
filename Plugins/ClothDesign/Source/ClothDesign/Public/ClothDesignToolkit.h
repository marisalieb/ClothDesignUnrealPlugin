#ifndef FClothDesignToolkit_H
#define FClothDesignToolkit_H

#include "Toolkits/BaseToolkit.h"
#include "ClothDesignEditorMode.h"
#include "GameFramework/Actor.h"
#include "ClothSimSettings.h"

/*
 * Thesis reference:
 * See Chapter 4.2 and 4.3 for detailed explanations.
 */

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
	TSharedPtr<FUICommandList> ToolkitCommandList;
	// objects for pickers: collision body mesh, cloth, object, material for fabric
	TWeakObjectPtr<UStaticMesh> SelectedCollisionMesh;
	TWeakObjectPtr<USkeletalMesh> SelectedClothMesh;
	TWeakObjectPtr<UMaterialInterface> SelectedTextileMaterial;

	FReply OnOpen2DWindowClicked();

	TSharedRef<SWidget> MakeOpen2DButton();
	
	TSharedRef<SWidget> MakeObjectPicker(
		const FText& LabelText,
		const UClass* AllowedClass,
		TFunction<FString()> GetPath,
		TFunction<void(const FAssetData&)> OnChanged,
		bool bFilterBySceneUsage);

	TSharedRef<SWidget> MakeClothSettingsSection();
	TSharedRef<SWidget> MakeCollisionSection();

	
	// functions for each picker
	FString GetSelectedCollisionMeshPath() const;
	void OnCollisionMeshSelected(const FAssetData& AssetData);

	FString GetSelectedClothMeshPath() const;
	void OnClothMeshSelected(const FAssetData& AssetData);

	FString GetSelectedTextileMaterialPath() const;
	void OnTextileMaterialSelected(const FAssetData& AssetData);
	// void ForEachComponentUsingSelectedMesh(TFunctionRef<void(USkeletalMeshComponent*)> Op) const;
	void ForEachComponentUsingMesh(USkeletalMesh* Mesh, TFunctionRef<void(USkeletalMeshComponent*)> Op) const;

	

	//TSharedPtr<FPresetItem> SelectedPreset = PresetOptions[0];
	FClothSimSettings SimSettings;
	TSharedPtr<FPresetItem> SelectedPresetSharedPtr; // mirrors SimSettings.SelectedPreset
	
	TSharedRef<SWidget> MakePresetPicker();
	
	void OnPresetSelected(TSharedPtr<FPresetItem> NewSelection, ESelectInfo::Type SelectInfo);

	FText GetPresetDisplayName(EClothPreset Preset);
	void ForEachComponentUsingSelectedMesh(TFunctionRef<void(USkeletalMeshComponent*)> Fn);
	

	//static void SetClothCollisionFlags(USkeletalMeshComponent* SkelComp);
	friend class FClothDesignToolkitTest;

};


#endif
