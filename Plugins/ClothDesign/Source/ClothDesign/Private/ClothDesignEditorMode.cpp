// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClothDesignEditorMode.h"
#include "ClothDesignToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "ClothDesignCommands.h"
#include "ClothDesignStyle.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// #include "Tools/ClothDesignSimpleTool.h"
// #include "Tools/ClothDesignInteractiveTool.h"

// step 2: register a ToolBuilder in FClothDesignEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "ClothDesignEditorMode"

const FEditorModeID UClothDesignEditorMode::EM_ClothDesignEditorModeId = TEXT("EM_ClothDesignEditorMode");

// FString UClothDesignEditorMode::SimpleToolName = TEXT("ClothDesign_ActorInfoTool");
// FString UClothDesignEditorMode::InteractiveToolName = TEXT("ClothDesign_MeasureDistanceTool");


UClothDesignEditorMode::UClothDesignEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(
		UClothDesignEditorMode::EM_ClothDesignEditorModeId,
		LOCTEXT("ModeName", "ClothDesign"),
		FSlateIcon(),
		true);
}

UClothDesignEditorMode::~UClothDesignEditorMode()
{
}

void UClothDesignEditorMode::ActorSelectionChangeNotify()
{
}

void UClothDesignEditorMode::Enter()
{
	UEdMode::Enter();
	
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	
	// const FClothDesignCommands& SampleToolCommands = FClothDesignCommands::Get();

	// RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UClothDesignSimpleToolBuilder>(this));
	// RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UClothDesignInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	// GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UClothDesignEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FClothDesignToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UClothDesignEditorMode::GetModeCommands() const
{
	return FClothDesignCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
