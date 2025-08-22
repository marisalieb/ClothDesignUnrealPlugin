
#include "ClothDesignEditorMode.h"
#include "ClothDesignToolkit.h"
#include "ClothDesignStyle.h"

// This file was started using the Unreal Engine 5.5 Editor Mode C++ template.
// This template can be created directly in Unreal Engine in the plugins section.
//
// The template code has been modified and extended for this project.
// The unmodified template code is included at the bottom of the file for reference.


#define LOCTEXT_NAMESPACE "ClothDesignEditorMode"

const FEditorModeID UClothDesignEditorMode::EM_ClothDesignEditorModeId = TEXT("EM_ClothDesignEditorMode");


UClothDesignEditorMode::UClothDesignEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// // appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(
	FName("EM_ClothDesign"), // mode ID
	FText::FromString("Cloth Design"), // display name
	FSlateIcon(FClothDesignStyle::GetStyleSetName(), "ClothDesignIcon"),
	true // visible
);
}


void UClothDesignEditorMode::ActorSelectionChangeNotify()
{
}

void UClothDesignEditorMode::Enter()
{
	UEdMode::Enter();
}

void UClothDesignEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FClothDesignToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UClothDesignEditorMode::GetModeCommands() const
{
	return {}; 
}

#undef LOCTEXT_NAMESPACE




/*
------------------------------------------
Original Unreal Engine Template Code
------------------------------------------
// Copyright Epic Games, Inc. All Rights Reserved.

#include "NewEditorModeEditorMode.h"
#include "NewEditorModeEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "NewEditorModeEditorModeCommands.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/NewEditorModeSimpleTool.h"
#include "Tools/NewEditorModeInteractiveTool.h"

// step 2: register a ToolBuilder in FNewEditorModeEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "NewEditorModeEditorMode"

const FEditorModeID UNewEditorModeEditorMode::EM_NewEditorModeEditorModeId = TEXT("EM_NewEditorModeEditorMode");

FString UNewEditorModeEditorMode::SimpleToolName = TEXT("NewEditorMode_ActorInfoTool");
FString UNewEditorModeEditorMode::InteractiveToolName = TEXT("NewEditorMode_MeasureDistanceTool");


UNewEditorModeEditorMode::UNewEditorModeEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UNewEditorModeEditorMode::EM_NewEditorModeEditorModeId,
		LOCTEXT("ModeName", "NewEditorMode"),
		FSlateIcon(),
		true);
}


UNewEditorModeEditorMode::~UNewEditorModeEditorMode()
{
}


void UNewEditorModeEditorMode::ActorSelectionChangeNotify()
{
}

void UNewEditorModeEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FNewEditorModeEditorModeCommands& SampleToolCommands = FNewEditorModeEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UNewEditorModeSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UNewEditorModeInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UNewEditorModeEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FNewEditorModeEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UNewEditorModeEditorMode::GetModeCommands() const
{
	return FNewEditorModeEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
*/
