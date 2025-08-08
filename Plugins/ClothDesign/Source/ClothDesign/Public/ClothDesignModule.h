
#pragma once

#include "Modules/ModuleManager.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "ClothDesignCanvas.h"
#include "ClothShapeAsset.h"
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
	//TSharedRef<class SDockTab> OnSpawn2DWindowTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawn2DWindowTab(const FSpawnTabArgs& Args);
	TSharedRef<SWidget> MakeBackgroundControls();
	TSharedRef<SWidget> MakeLoadSavePanel();


	
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

	
	TSharedRef<SWidget> MakeObjectPicker(
	const FText& LabelText,
	UClass* AllowedClass,
	TFunction<FString()> GetPath,
	TFunction<void(const FAssetData&)> OnChanged);

	TSharedRef<SWidget> MakeActionButtons();
	TSharedRef<SWidget> MakeClearAllButton();
	TSharedRef<SWidget> MakeModeToolbar();
	TSharedRef<SWidget> MakeModeButton(
						SClothDesignCanvas::EClothEditorMode InMode,
						const FText& InLabel);

};
