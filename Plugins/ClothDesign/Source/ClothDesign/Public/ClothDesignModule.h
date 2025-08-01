// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "SClothDesignCanvas.h"
#include "UClothShapeAsset.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"


/*
 This is the module definition for the editor mode. You can implement custom functionality
 as your plugin module starts up and shuts down. See IModuleInterface for more extensibility options.
 */
class FClothDesignModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	void Spawn2DWindow();
	TSharedRef<class SDockTab> OnSpawn2DWindowTab(const class FSpawnTabArgs& SpawnTabArgs);

	static const FName TwoDTabName;
	void OnTabActivated(TSharedPtr<SDockTab> Tab, ETabActivationCause ActivationCause);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	
	TSharedPtr<class SClothDesignCanvas> CanvasWidget;

	FReply OnGenerateMeshClicked();  // Declare the handler

	FReply OnSewingClicked();  // Declare the handler
	
	FSlateColor GetModeButtonColor(SClothDesignCanvas::EClothEditorMode Mode) const;

	void SaveCurrentShapesToAsset();

	
	FReply OnSaveClicked();  // Declare the handler
	// FReply OnSaveAsClicked();  // Declare the handler
	FReply OnClearClicked();  // Declare the handler

	FString CurrentSaveName = TEXT("ShapeName");
	
};
