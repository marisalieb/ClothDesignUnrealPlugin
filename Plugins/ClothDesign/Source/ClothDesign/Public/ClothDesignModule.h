#ifndef FClothDesignModule_H
#define FClothDesignModule_H

#include "Modules/ModuleManager.h"
#include "ClothDesignCanvas.h"

/*
 * Thesis reference:
 * See Chapter 4.2 and 4.3 for detailed explanations.
 */

class FClothDesignModule : public IModuleInterface
{
public:

	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	static const FName TwoDTabName;
	void Spawn2DWindow();


private:
	TSharedPtr<FUICommandList> PluginCommands;
	
	TSharedPtr<SClothDesignCanvas> CanvasWidget;
	FString CurrentSaveName = TEXT("ShapeName");
	
	void OnTabActivated(TSharedPtr<SDockTab> Tab, ETabActivationCause ActivationCause);
	
	TSharedRef<SWidget> MakeObjectPicker(
			const FText& LabelText,
			const UClass* AllowedClass,
			TFunction<FString()> GetPath,
			TFunction<void(const FAssetData&)> OnChanged);
	TSharedRef<SWidget> MakeBackgroundControls();
	TSharedRef<SWidget> MakeLoadSavePanel();
	TSharedRef<SWidget> MakeActionButtons();
	TSharedRef<SWidget> MakeClearButtons();
	TSharedRef<SWidget> MakeModeButton(
			SClothDesignCanvas::EClothEditorMode InMode,
			const FText& InLabel);
	TSharedRef<SWidget> MakeModeToolbar();
	FSlateColor GetModeButtonColor(SClothDesignCanvas::EClothEditorMode Mode) const;
	TSharedRef<SWidget> MakeModeReminderBox();

	TSharedRef<SDockTab> OnSpawn2DWindowTab(const FSpawnTabArgs& Args);
	FReply OnGenerateMeshClicked();
	FReply OnSewingClicked();
	FReply OnMergeMeshesClicked();
	FReply OnSaveClicked();
	FReply OnClearClicked(); 
	FReply OnClearSewingClicked(); 

	static bool CheckSkeletonAssetExists();

	
	friend class FClothDesignModuleButtonTest;

};

#endif
