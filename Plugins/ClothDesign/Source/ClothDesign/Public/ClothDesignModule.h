#ifndef FClothDesignModule_H
#define FClothDesignModule_H

#include "Modules/ModuleManager.h"
#include "ClothDesignCanvas.h"

/*
 * Thesis reference:
 * See Chapter 4.2 and 4.3 for detailed explanations.
 */

/**
 * @brief Main module class for the Cloth Design editor plugin.
 *
 * This module initialises and tears down the editor integration, registers commands,
 * and provides the UI entry point for the 2D cloth design window. By centralising
 * module lifecycle and UI creation here, the plugin keeps startup/shutdown concerns
 * and UI wiring separate from canvas logic, making the system easier to test and extend.
 */
class FClothDesignModule : public IModuleInterface
{
public:
	// --- IModuleInterface implementation ---

	/**
	 * @brief Called when the module is starting up.
	 *
	 * Initialises plugin state, registers editor commands and UI, and prepares
	 * any resources required by the 2D editor. This ensures the editor integration
	 * is available once the module is loaded.
	 */
	virtual void StartupModule() override;

	/**
	 * @brief Called when the module is shutting down.
	 *
	 * Cleans up registered commands, UI, and any resources to avoid leaking
	 * editor state across reloads or shutdown. Keeps module lifecycle deterministic.
	 */
	virtual void ShutdownModule() override;

	/** 
	 * @brief Name identifier for the 2D editor tab.
	 *
	 * Used to spawn or find the 2D editor tab consistently across the host editor.
	 */
	static const FName TwoDTabName;

	/**
	 * @brief Spawns or focuses the 2D design window for the plugin.
	 *
	 * Encapsulates the logic to create the tab, register content, and ensure
	 * a single consistent entry point for the 2D editor UI.
	 */
	void Spawn2DWindow();

private:
	/** Bound command list for the plugin's UI actions. */
	TSharedPtr<FUICommandList> PluginCommands; /**< Stores mapped UI commands so they can be invoked from menus or toolbars. */

	/** The primary canvas widget for the 2D editor (kept so module can manage lifetime). */
	TSharedPtr<SClothDesignCanvas> CanvasWidget; /**< Holds the central canvas instance used by the 2D editor tab. */

	/** Currently entered save name used by the save UI. */
	FString CurrentSaveName = TEXT("ShapeName"); /**< Default save name to reduce friction for new saves. */

	/**
	 * @brief Called when a tab changes activation state.
	 *
	 * Allows the module to react to the 2D tab being focused or hidden (for example
	 * to pause background tasks or refresh UI when reactivated).
	 *
	 * @param Tab The tab that changed activation.
	 * @param ActivationCause Why the tab was activated.
	 */
	void OnTabActivated(TSharedPtr<SDockTab> Tab, ETabActivationCause ActivationCause);

	/**
	 * @brief Creates an object-picker widget with a label.
	 *
	 * This helper centralises object-picker creation so different pickers
	 * look and behave consistently across the UI.
	 *
	 * @param LabelText Label to show next to the picker (user-facing).
	 * @param AllowedClass Class type that the picker should allow.
	 * @param GetPath Function returning current path string (used to initialise display).
	 * @param OnChanged Callback invoked when the user selects a different asset.
	 * @return A Slate widget containing the labelled object picker.
	 */
	TSharedRef<SWidget> MakeObjectPicker(
		const FText& LabelText,
		const UClass* AllowedClass,
		TFunction<FString()> GetPath,
		TFunction<void(const FAssetData&)> OnChanged);

	/**
	 * @brief Builds widgets that control background texture and related options.
	 *
	 * Grouping background controls into a single helper simplifies layout and
	 * ensures consistent behaviour when embedding the controls in different panels.
	 *
	 * @return A Slate widget containing background controls.
	 */
	TSharedRef<SWidget> MakeBackgroundControls();

	/**
	 * @brief Creates the load/save panel used by the editor.
	 *
	 * Centralises the load/save UI so behaviour (validation, default names) is consistent.
	 *
	 * @return A Slate widget representing the load/save area.
	 */
	TSharedRef<SWidget> MakeLoadSavePanel();

	/**
	 * @brief Builds primary action buttons (generate meshes, save, merge, etc.).
	 *
	 * Encapsulating action buttons keeps layout and binding logic in one place.
	 *
	 * @return A Slate widget containing action buttons.
	 */
	TSharedRef<SWidget> MakeActionButtons();

	/**
	 * @brief Builds clear/reset buttons for the UI.
	 *
	 * Separates destructive actions into a dedicated area and allows consistent confirmation handling.
	 *
	 * @return A Slate widget with clear-related buttons.
	 */
	TSharedRef<SWidget> MakeClearButtons();

	/**
	 * @brief Creates a single mode-selection button for the toolbar.
	 *
	 * Mode buttons change the canvas input semantics (draw/select/sew), so centralising
	 * creation ensures consistent styling and command wiring.
	 *
	 * @param InMode The editor mode the button will switch to.
	 * @param InLabel Human-readable label for the button.
	 * @return The constructed mode button widget.
	 */
	TSharedRef<SWidget> MakeModeButton(
		SClothDesignCanvas::EClothEditorMode InMode,
		const FText& InLabel);

	/**
	 * @brief Builds the mode-selection toolbar (draw / select / sew).
	 *
	 * Having a dedicated toolbar builder ensures it can be reused or embedded in different
	 * layouts without duplicating code.
	 *
	 * @return A Slate widget containing the mode toolbar.
	 */
	TSharedRef<SWidget> MakeModeToolbar();

	/**
	 * @brief Returns a colour to use for a mode button depending on active mode.
	 *
	 * Colour-coding mode buttons provides immediate visual feedback about the active tool,
	 * reducing user confusion.
	 *
	 * @param Mode Mode to evaluate.
	 * @return An Slate colour to apply to the corresponding button.
	 */
	FSlateColor GetModeButtonColor(SClothDesignCanvas::EClothEditorMode Mode) const;

	/**
	 * @brief Builds a small UI box that reminds the user of the current mode or hints.
	 *
	 * Centralised reminder creation ensures consistent messaging and placement.
	 *
	 * @return A Slate widget containing the mode reminder.
	 */
	TSharedRef<SWidget> MakeModeReminderBox();

	/**
	 * @brief Tab spawner callback for the 2D window tab.
	 *
	 * Encapsulates tab creation so the module can control how the 2D editor is initialised.
	 *
	 * @param Args Spawn arguments provided by the docking system.
	 * @return The newly created dock tab for the 2D editor.
	 */
	TSharedRef<SDockTab> OnSpawn2DWindowTab(const FSpawnTabArgs& Args);

	/**
	 * @brief Called when the user clicks "Generate Mesh".
	 *
	 * Triggers mesh generation for the current canvas shapes. Returns an FReply
	 * so it can be bound directly to Slate buttons.
	 *
	 * @return FReply indicating whether the click was handled.
	 */
	FReply OnGenerateMeshClicked();

	/**
	 * @brief Called when the user clicks "Sewing".
	 *
	 * Initiates the sewing workflow from the UI.
	 *
	 * @return FReply indicating whether the click was handled.
	 */
	FReply OnSewingClicked();

	/**
	 * @brief Called when the user clicks "Merge Meshes".
	 *
	 * Begins the merge operation for sewn pattern pieces.
	 *
	 * @return FReply indicating whether the click was handled.
	 */
	FReply OnMergeMeshesClicked();

	/**
	 * @brief Called when the user clicks "Save".
	 *
	 * Commits the current canvas state to an asset or storage location.
	 *
	 * @return FReply indicating whether the click was handled.
	 */
	FReply OnSaveClicked();

	/**
	 * @brief Called when the user clicks "Clear".
	 *
	 * Clears the current canvas shapes; UI should confirm or undo should be possible.
	 *
	 * @return FReply indicating whether the click was handled.
	 */
	FReply OnClearClicked();

	/**
	 * @brief Called when the user clicks "Clear Sewing".
	 *
	 * Removes all sewing definitions and previews from the canvas.
	 *
	 * @return FReply indicating whether the click was handled.
	 */
	FReply OnClearSewingClicked();

	/**
	 * @brief Utility to check for the presence of a required skeleton asset.
	 *
	 * Used before operations that require a skeleton (e.g. creating a skeletal mesh)
	 * to avoid presenting options that would fail.
	 *
	 * @return true if the expected skeleton asset exists.
	 */
	static bool CheckSkeletonAssetExists();

	friend class FClothDesignModuleButtonTest; /**< Allows the test fixture access to internals for unit tests. */
};


#endif
