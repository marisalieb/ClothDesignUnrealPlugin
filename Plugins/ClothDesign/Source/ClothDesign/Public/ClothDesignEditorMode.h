
#pragma once

#include "Tools/UEdMode.h"
#include "UnrealWidget.h"
#include "UnrealWidgetFwd.h"
#include "EdMode.h"

#include "ClothDesignEditorMode.generated.h"

/**
 * This class provides an example of how to extend a UEdMode to add some simple tools
 * using the InteractiveTools framework. The various UEdMode input event handlers (see UEdMode.h)
 * forward events to a UEdModeInteractiveToolsContext instance, which
 * has all the logic for interacting with the InputRouter, ToolManager, etc.
 * The functions provided here are the minimum to get started inserting some custom behavior.
 * Take a look at the UEdMode markup for more extensibility options.
 */
 UCLASS()

class CLOTHDESIGN_API UClothDesignEditorMode : public UEdMode

// class UClothDesignEditorMode : public FEdMode
{
	GENERATED_BODY()

public:
	const static FEditorModeID EM_ClothDesignEditorModeId;
	
	static FString SimpleToolName;
	static FString InteractiveToolName;
	
	UClothDesignEditorMode();
	virtual ~UClothDesignEditorMode() override;

	/** UEdMode interface */
	virtual void Enter() override;
	virtual void ActorSelectionChangeNotify() override;
	virtual void CreateToolkit() override; // took out override 
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const; // took out override


	virtual bool UsesTransformWidget() const { return true;}
 	
};
