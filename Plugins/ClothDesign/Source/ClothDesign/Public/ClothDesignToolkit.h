#pragma once

#include "Toolkits/BaseToolkit.h"
#include "ClothDesignEditorMode.h"
#include "GameFramework/Actor.h"
#include "ClothSimSettings.h"  // include your new struct header

// UENUM()
// enum class EClothPreset : uint8
// {
// 	Denim,
// 	Leather,
// 	Silk,
// 	Jersey,
// 	Max UMETA(Hidden)  // Optional helper
// };
//
// struct FPresetItem
// {
// 	EClothPreset Preset;
// 	FString DisplayName;
//
// 	FPresetItem(EClothPreset InPreset, const FString& InName)
// 		: Preset(InPreset), DisplayName(InName) {}
// };


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




	// TArray<TSharedPtr<FPresetItem>> PresetOptions = {
	// 	MakeShared<FPresetItem>(EClothPreset::Denim, TEXT("Denim")),
	// 	MakeShared<FPresetItem>(EClothPreset::Leather, TEXT("Leather")),
	// 	MakeShared<FPresetItem>(EClothPreset::Silk, TEXT("Silk")),
	// 	MakeShared<FPresetItem>(EClothPreset::Jersey, TEXT("Jersey"))
	// };

	//TSharedPtr<FPresetItem> SelectedPreset = PresetOptions[0];
	FClothSimSettings SimSettings;
	TSharedPtr<FPresetItem> SelectedPresetSharedPtr; // mirrors SimSettings.SelectedPreset
	
	TSharedRef<SWidget> MakePresetPicker();
	
	void OnPresetSelected(TSharedPtr<FPresetItem> NewSelection, ESelectInfo::Type SelectInfo);

	FText GetPresetDisplayName(EClothPreset Preset);
	void ForEachComponentUsingSelectedMesh(TFunctionRef<void(USkeletalMeshComponent*)> Fn);

	//static void SetClothCollisionFlags(USkeletalMeshComponent* SkelComp);

};


