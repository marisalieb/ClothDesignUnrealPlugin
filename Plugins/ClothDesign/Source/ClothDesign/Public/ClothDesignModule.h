// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Widgets/Input/SNumericEntryBox.h"

/**
 * @class FClothDesignModule
 * @brief Main plugin module class for the Cloth Design editor mode.
 *
 * This module handles initialization and shutdown of the Cloth Design plugin,
 * as well as spawning and managing custom Slate UI for cloth pattern design.
 *
 * Responsibilities:
 * - Registers editor tabs and menus.
 * - Spawns the 2D design window and manages its lifecycle.
 * - Handles user actions such as generating meshes and sewing patterns.
 *
 * @see IModuleInterface
 */
class FClothDesignModule : public IModuleInterface
{
public:

	/** 
	 * @brief Called when the module is loaded into memory.
	 *
	 * Registers commands, menu entries, and sets up any required editor UI elements.
	 */
	virtual void StartupModule() override;

	/**
	 * @brief Called before the module is unloaded from memory.
	 *
	 * Cleans up resources, unregisters commands, and removes UI elements.
	 */
	virtual void ShutdownModule() override;
	
	/**
	 * @brief Opens the Cloth Design 2D editor window.
	 *
	 * Creates a new tab in the editor to display the design canvas.
	 */
	void Spawn2DWindow();

	/**
	 * @brief Callback used to spawn the 2D design window tab.
	 *
	 * @param SpawnTabArgs Arguments provided by the tab manager when creating the tab.
	 * @return A reference to the newly created dock tab widget.
	 */
	TSharedRef<class SDockTab> OnSpawn2DWindowTab(const class FSpawnTabArgs& SpawnTabArgs);

	/** 
	 * @brief Name identifier for the 2D design editor tab.
	 */
	static const FName TwoDTabName;

	/**
	 * @brief Called when the 2D editor tab becomes active.
	 *
	 * @param Tab The activated dock tab.
	 * @param ActivationCause The reason the tab was activated.
	 */
	void OnTabActivated(TSharedPtr<SDockTab> Tab, ETabActivationCause ActivationCause);

private:

	/** 
	 * @brief Command list for the plugin's editor actions.
	 */
	TSharedPtr<class FUICommandList> PluginCommands;
	
	/** 
	 * @brief Pointer to the cloth design canvas widget displayed in the 2D tab.
	 */
	TSharedPtr<class SClothDesignCanvas> CanvasWidget;

	/**
	 * @brief Event handler for the "Generate Mesh" button.
	 *
	 * @return An FReply indicating how the event was handled.
	 */
	FReply OnGenerateMeshClicked();

	/**
	 * @brief Event handler for the "Sewing" button.
	 *
	 * @return An FReply indicating how the event was handled.
	 */
	FReply OnSewingClicked();
};
