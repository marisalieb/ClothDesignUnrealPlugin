// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"

/*
 This class contains info about the full set of commands used in this editor mode.
 */
class FClothDesignEditorModeCommands : public TCommands<FClothDesignEditorModeCommands>
{
public:
	FClothDesignEditorModeCommands();

	virtual void RegisterCommands() override;
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();

	TSharedPtr<FUICommandInfo> SimpleTool;
	TSharedPtr<FUICommandInfo> InteractiveTool;

	TSharedPtr<FUICommandInfo> Open2DWindow;

protected:
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;
};

