#ifndef FClothDesignCommands_H
#define FClothDesignCommands_H

#include "Framework/Commands/Commands.h"

/*
 * Thesis reference:
 * See Chapter 4.2 for details.
 */

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

#endif
