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
 * @brief Toolkit that provides the Cloth Design editor's UI and tool palettes.
 * 
 * The toolkit bridges the editor mode and Slate UI: it initialises in-editor panels,
 * exposes inline content for embedding in the host editor, and centralises command
 * wiring and object picker state. Keeping toolkit logic here keeps the mode class
 * lightweight and makes UI tests easier to write.
 */
class FClothDesignToolkit : public FModeToolkit
{
public:
	/** Constructor */
	FClothDesignToolkit();

	/** FModeToolkit interface ------------------------------------------------- */

	/**
	 * @brief Initialise the toolkit and create UI content.
	 *
	 * Called when the toolkit is created; responsible for building the toolkit's
	 * widget hierarchy, registering commands, and binding callbacks.
	 *
	 * @param InitToolkitHost The host for toolkit docking and lifetime.
	 * @param InOwningMode Weak pointer to the owning editor mode.
	 */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;

	/**
	 * @brief Returns the names of tool palettes provided by this toolkit.
	 *
	 * Tool palettes group related tools for the editor UI; exposing their names
	 * allows the host to render palette tabs or lists.
	 *
	 * @param PaletteNames Output array to append palette names to.
	 */
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	/** IToolkit interface ---------------------------------------------------- */

	/**
	 * @brief Returns the toolkit's unique FName.
	 *
	 * Used by the host to identify and persist toolkit state.
	 *
	 * @return Toolkit FName identifier.
	 */
	virtual FName GetToolkitFName() const override;

	/**
	 * @brief Returns the base display name for this toolkit.
	 *
	 * Shown in the host UI and used for accessibility/labels.
	 *
	 * @return Localised text for the toolkit name.
	 */
	virtual FText GetBaseToolkitName() const override;

	/**
	 * @brief Returns the toolkit's inline content widget.
	 *
	 * This widget is embedded directly into the mode UI; it should be the
	 * primary interactive area the user manipulates (pickers, settings, buttons).
	 *
	 * @return Shared pointer to the widget to embed inline.
	 */
	virtual TSharedPtr<SWidget> GetInlineContent() const override;


private:
	/** The root widget created by Init and returned from GetInlineContent. */
	TSharedPtr<SWidget> ToolkitWidget; /**< Holds the constructed UI so it stays alive as long as the toolkit exists. */

	/** Command list for toolkit-specific actions (open 2D window, apply presets, etc.). */
	TSharedPtr<FUICommandList> ToolkitCommandList; /**< Centralises command bindings so shortcuts and toolbar buttons behave consistently. */

	// --- Object picker state -------------------------------------------------

	/** Selected static mesh used as a collision body for cloth. */
	TWeakObjectPtr<UStaticMesh> SelectedCollisionMesh; /**< Remembered so the picker reflects current selection and can apply it to scene components. */

	/** Selected skeletal mesh representing the cloth for simulation. */
	TWeakObjectPtr<USkeletalMesh> SelectedClothMesh; /**< Stored so simulation settings and apply operations know which mesh to operate on. */

	/** Selected material used as the textile appearance. */
	TWeakObjectPtr<UMaterialInterface> SelectedTextileMaterial; /**< Stored to populate UI and to apply materials to spawned actors. */

	// --- UI callbacks & helpers ---------------------------------------------

	/**
	 * @brief Handler for the "Open 2D Window" button click.
	 *
	 * Triggers creation or focusing of the 2D cloth design tab so the user
	 * can edit patterns in the dedicated canvas.
	 *
	 * @return FReply indicating whether the click was handled.
	 */
	FReply OnOpen2DWindowClicked();

	/**
	 * @brief Creates the "Open 2D" toolbar/button widget.
	 *
	 * Encapsulates button styling and command binding so the host can
	 * embed it consistently across different layouts.
	 *
	 * @return The constructed button widget.
	 */
	TSharedRef<SWidget> MakeOpen2DButton();

	/**
	 * @brief Builds a labelled object picker widget.
	 *
	 * Central helper for creating the various asset pickers (collision, cloth, material),
	 * keeping behaviour and layout consistent.
	 *
	 * @param LabelText The visible label to display beside the picker.
	 * @param AllowedClass The UClass type the picker should restrict selections to.
	 * @param GetPath Function that returns the currently selected asset path for initial display.
	 * @param OnChanged Callback invoked when the user selects a new asset.
	 * @param bFilterBySceneUsage If true, filter assets by whether they are used in the current scene.
	 * @return The constructed labelled picker widget.
	 */
	TSharedRef<SWidget> MakeObjectPicker(
		const FText& LabelText,
		const UClass* AllowedClass,
		TFunction<FString()> GetPath,
		TFunction<void(const FAssetData&)> OnChanged,
		bool bFilterBySceneUsage);

	/**
	 * @brief Constructs the cloth settings UI section (presets, simulation options).
	 *
	 * Grouping these controls simplifies layout changes and keeps simulation-related
	 * controls together for discoverability.
	 *
	 * @return A widget containing cloth simulation controls.
	 */
	TSharedRef<SWidget> MakeClothSettingsSection();

	/**
	 * @brief Constructs the collision section UI (collision mesh picker, options).
	 *
	 * Separating collision UI reduces cognitive load and encapsulates related
	 * validation logic (e.g. checking the mesh is compatible).
	 *
	 * @return A widget containing collision options.
	 */
	TSharedRef<SWidget> MakeCollisionSection();


	// --- Pickers: path getters and selection callbacks ----------------------

	/**
	 * @brief Returns the path for the currently selected collision mesh.
	 *
	 * Used to display current selection in the picker text field.
	 *
	 * @return Path string, or empty if none selected.
	 */
	FString GetSelectedCollisionMeshPath() const;

	/**
	 * @brief Called when a collision mesh asset is chosen in the picker.
	 *
	 * Updates internal state and any dependent UI or components.
	 *
	 * @param AssetData The chosen asset's metadata.
	 */
	void OnCollisionMeshSelected(const FAssetData& AssetData);

	/**
	 * @brief Returns the path for the currently selected cloth skeletal mesh.
	 *
	 * @return Path string, or empty if none selected.
	 */
	FString GetSelectedClothMeshPath() const;

	/**
	 * @brief Called when a cloth mesh asset is chosen in the picker.
	 *
	 * Updates the toolkit's selection and may refresh dependent UI or example components.
	 *
	 * @param AssetData The chosen asset's metadata.
	 */
	void OnClothMeshSelected(const FAssetData& AssetData);

	/**
	 * @brief Returns the path for the currently selected textile material.
	 *
	 * @return Path string, or empty if none selected.
	 */
	FString GetSelectedTextileMaterialPath() const;

	/**
	 * @brief Called when a textile material is chosen in the picker.
	 *
	 * Updates internal state and previews where relevant.
	 *
	 * @param AssetData The chosen asset's metadata.
	 */
	void OnTextileMaterialSelected(const FAssetData& AssetData);

	/**
	 * @brief Iterates over components using the provided skeletal mesh and invokes an operation.
	 *
	 * Useful for applying settings or updates to all components that use the selected mesh.
	 *
	 * @param Mesh The skeletal mesh to search for.
	 * @param Op Operation to apply to each matching USkeletalMeshComponent.
	 */
	void ForEachComponentUsingMesh(USkeletalMesh* Mesh, TFunctionRef<void(USkeletalMeshComponent*)> Op) const;


	// --- Presets and simulation settings -----------------------------------

	/** Cloth simulation settings and preset storage used by the UI. */
	FClothSimSettings SimSettings; /**< Holds preset configs and helper functions to apply them. */

	/** Shared pointer to the currently selected preset UI item (mirrors SimSettings.SelectedPreset). */
	TSharedPtr<FPresetItem> SelectedPresetSharedPtr; /**< Maintains combo box selection state and avoids copying. */

	/**
	 * @brief Builds the preset selection UI (combo box with preset entries).
	 *
	 * Centralising creation ensures the list order and display names are consistent.
	 *
	 * @return The preset picker widget.
	 */
	TSharedRef<SWidget> MakePresetPicker();

	/**
	 * @brief Called when the user selects a different preset from the UI.
	 *
	 * Updates SimSettings.SelectedPreset and any immediate preview/application.
	 *
	 * @param NewSelection Shared pointer to the chosen preset item.
	 * @param SelectInfo The selection source (user click, navigation, etc.).
	 */
	void OnPresetSelected(TSharedPtr<FPresetItem> NewSelection, ESelectInfo::Type SelectInfo);

	/**
	 * @brief Returns the display name for the given preset enum.
	 *
	 * Keeps the UI labels and localisation logic in one place.
	 *
	 * @param Preset The preset enum to query.
	 * @return Localised display name for the preset.
	 */
	FText GetPresetDisplayName(EClothPreset Preset);

	/**
	 * @brief Iterate over components that use the currently-selected cloth mesh and invoke a function.
	 *
	 * Convenience wrapper around ForEachComponentUsingMesh using the toolkit's selected cloth mesh.
	 *
	 * @param Fn Operation to run on each USkeletalMeshComponent that uses the selected mesh.
	 */
	void ForEachComponentUsingSelectedMesh(TFunctionRef<void(USkeletalMeshComponent*)> Fn);

	/** Unit test access for toolkit internals. */
	friend class FClothDesignToolkitTest;
};



#endif
