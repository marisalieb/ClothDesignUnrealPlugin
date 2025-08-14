
#include "ClothDesignCommands.h"

// This file was started using the Unreal Engine 5.5 Editor Mode C++ template.
// This template can be created directly in Unreal Engine in the plugins section.
//
// The template code has been modified and extended for this project.
// The unmodified template code is included at the bottom of the file for reference.


#define LOCTEXT_NAMESPACE "ClothDesignEditorModeCommands"

FClothDesignCommands::FClothDesignCommands()
	: TCommands("ClothDesignEditorMode",
		NSLOCTEXT("ClothDesignEditorMode", "ClothDesignEditorModeCommands", "ClothDesign Editor Mode"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FClothDesignCommands::RegisterCommands()
{
	// TArray <TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);
	
	UI_COMMAND(Open2DWindow, "Open 2D Window", "Opens the custom 2D editor window", EUserInterfaceActionType::Button, FInputChord());
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FClothDesignCommands::GetCommands()
{
	return Get().Commands;
}

#undef LOCTEXT_NAMESPACE


/*
------------------------------------------
Original Unreal Engine Template Code
------------------------------------------
// Copyright Epic Games, Inc. All Rights Reserved.

#include "NewEditorModeEditorModeCommands.h"
#include "NewEditorModeEditorMode.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "NewEditorModeEditorModeCommands"

FNewEditorModeEditorModeCommands::FNewEditorModeEditorModeCommands()
	: TCommands<FNewEditorModeEditorModeCommands>("NewEditorModeEditorMode",
		NSLOCTEXT("NewEditorModeEditorMode", "NewEditorModeEditorModeCommands", "NewEditorMode Editor Mode"),
		NAME_None,
		FEditorStyle::GetStyleSetName())
{
}

void FNewEditorModeEditorModeCommands::RegisterCommands()
{
	TArray <TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);

	UI_COMMAND(SimpleTool, "Show Actor Info", "Opens message box with info about a clicked actor", EUserInterfaceActionType::Button, FInputChord());
	ToolCommands.Add(SimpleTool);

	UI_COMMAND(InteractiveTool, "Measure Distance", "Measures distance between 2 points (click to set origin, shift-click to set end point)", EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(InteractiveTool);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FNewEditorModeEditorModeCommands::GetCommands()
{
	return FNewEditorModeEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE

*/
