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



/**
 * @class FClothDesignToolkit
 * @brief A toolkit for designing and simulating cloth in the editor.
 * 
 * This class provides a user interface and functionality for designing cloth, including
 * selecting meshes, applying presets, and configuring simulation settings.
 */
class FClothDesignToolkit : public FModeToolkit
{
public:
	/** Constructor */
	FClothDesignToolkit();

	/**
	 * @brief Initializes the toolkit with the given host and owning mode.
	 * 
	 * @param InitToolkitHost The host for the toolkit.
	 * @param InOwningMode The owning editor mode.
	 */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;

	/**
	 * @brief Retrieves the names of the tool palettes.
	 * 
	 * @param PaletteNames Array to populate with palette names.
	 */
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	/**
	 * @brief Gets the name of the toolkit.
	 * 
	 * @return The name of the toolkit.
	 */
	virtual FName GetToolkitFName() const override;

	/**
	 * @brief Gets the base name of the toolkit.
	 * 
	 * @return The base name of the toolkit as text.
	 */
	virtual FText GetBaseToolkitName() const override;

	/**
	 * @brief Gets the inline content widget for the toolkit.
	 * 
	 * @return A shared pointer to the inline content widget.
	 */
	virtual TSharedPtr<SWidget> GetInlineContent() const override;


private:
	/** The root widget created by Init and returned from GetInlineContent. */
	TSharedPtr<SWidget> ToolkitWidget;

	/** Command list for toolkit-specific actions (e.g., open 2D window, apply presets). */
	TSharedPtr<FUICommandList> ToolkitCommandList;

	/** Selected static mesh used as a collision body for cloth. */
	TWeakObjectPtr<UStaticMesh> SelectedCollisionMesh;

	/** Selected skeletal mesh representing the cloth for simulation. */
	TWeakObjectPtr<USkeletalMesh> SelectedClothMesh;

	/** Selected material used as the textile appearance. */
	TWeakObjectPtr<UMaterialInterface> SelectedTextileMaterial;


	/**
	 * @brief Handles the "Open 2D Window" button click event.
	 * 
	 * @return A reply indicating the result of the click event.
	 */
	FReply OnOpen2DWindowClicked();

	/**
	 * @brief Creates the "Open 2D Window" button widget.
	 * 
	 * @return A shared reference to the button widget.
	 */
	TSharedRef<SWidget> MakeOpen2DButton();

	/**
	 * @brief Creates an object picker widget.
	 * 
	 * @param LabelText The label text for the picker.
	 * @param AllowedClass The class of objects allowed for selection.
	 * @param GetPath A function to get the current path of the selected object.
	 * @param OnChanged A function to call when the selection changes.
	 * @param bFilterBySceneUsage Whether to filter objects by scene usage.
	 * @return A shared reference to the object picker widget.
	 */
	TSharedRef<SWidget> MakeObjectPicker(
		const FText& LabelText,
		const UClass* AllowedClass,
		TFunction<FString()> GetPath,
		TFunction<void(const FAssetData&)> OnChanged,
		bool bFilterBySceneUsage);

	/**
	 * @brief Creates the cloth settings section widget.
	 * 
	 * @return A shared reference to the settings section widget.
	 */
	TSharedRef<SWidget> MakeClothSettingsSection();

	/**
	 * @brief Creates the collision section widget.
	 * 
	 * @return A shared reference to the collision section widget.
	 */
	TSharedRef<SWidget> MakeCollisionSection();


	/**
	 * @brief Gets the path of the selected collision mesh.
	 * 
	 * @return The path of the selected collision mesh.
	 */
	FString GetSelectedCollisionMeshPath() const;

	/**
	 * @brief Handles the selection of a collision mesh.
	 * 
	 * @param AssetData The asset data of the selected collision mesh.
	 */
	void OnCollisionMeshSelected(const FAssetData& AssetData);

	/**
	 * @brief Gets the path of the selected cloth mesh.
	 * 
	 * @return The path of the selected cloth mesh.
	 */
	FString GetSelectedClothMeshPath() const;

	/**
	 * @brief Handles the selection of a cloth mesh.
	 * 
	 * @param AssetData The asset data of the selected cloth mesh.
	 */
	void OnClothMeshSelected(const FAssetData& AssetData);

	/**
	 * @brief Gets the path of the selected textile material.
	 * 
	 * @return The path of the selected textile material.
	 */
	FString GetSelectedTextileMaterialPath() const;

	/**
	 * @brief Handles the selection of a textile material.
	 * 
	 * @param AssetData The asset data of the selected textile material.
	 */
	void OnTextileMaterialSelected(const FAssetData& AssetData);
	

	/**
	 * @brief Executes an operation on each component using the specified skeletal mesh.
	 * 
	 * @param Mesh The skeletal mesh to search for.
	 * @param Op The operation to execute on each component.
	 */
	void ForEachComponentUsingMesh(USkeletalMesh* Mesh, TFunctionRef<void(USkeletalMeshComponent*)> Op) const;

	/** Cloth simulation settings and preset storage used by the UI. */
	FClothSimSettings SimSettings;

	/** Shared pointer to the currently selected preset UI item. */
	TSharedPtr<FPresetItem> SelectedPresetSharedPtr;

	
	/**
	 * @brief Creates the preset picker widget.
	 * 
	 * @return A shared reference to the preset picker widget.
	 */
	TSharedRef<SWidget> MakePresetPicker();

	/**
	 * @brief Handles the selection of a preset.
	 * 
	 * @param NewSelection The newly selected preset.
	 * @param SelectInfo Information about the selection event.
	 */
	void OnPresetSelected(TSharedPtr<FPresetItem> NewSelection, ESelectInfo::Type SelectInfo);

	/**
	 * @brief Gets the display name of the specified preset.
	 * 
	 * @param Preset The preset to get the display name for.
	 * @return The display name of the preset.
	 */
	FText GetPresetDisplayName(EClothPreset Preset);

	/**
	 * @brief Executes an operation on each component using the currently selected mesh.
	 * 
	 * @param Fn The operation to execute on each component.
	 */
	void ForEachComponentUsingSelectedMesh(TFunctionRef<void(USkeletalMeshComponent*)> Fn);

	/** Unit test access for toolkit internals. */
	friend class FClothDesignToolkitTest;
};



#endif
