
#pragma once

#include "Framework/Commands/Commands.h"


class FClothDesignCommands : public TCommands<FClothDesignCommands>
{
public:
	FClothDesignCommands();

	virtual void RegisterCommands() override;
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();
	
	TSharedPtr<FUICommandInfo> Open2DWindow;

protected:
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;

};

