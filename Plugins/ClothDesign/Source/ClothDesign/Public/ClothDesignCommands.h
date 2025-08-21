#ifndef FClothDesignCommands_H
#define FClothDesignCommands_H

#include "Framework/Commands/Commands.h"

/*
 * Thesis reference:
 * See Chapter 4.2 for details.
 */


/**
 * @brief Defines UI commands for the Cloth Design editor mode.
 * 
 * This class registers and stores all the user interface commands that are used in the 
 * Cloth Design editor mode, such as opening windows or triggering specific tools. 
 * It derives from TCommands to integrate with Unreal's command framework.
 */
class FClothDesignCommands : public TCommands<FClothDesignCommands>
{
public:
	/** Constructor */
	FClothDesignCommands();

	/**
	 * @brief Registers all commands associated with this mode.
	 * 
	 * This is where each TSharedPtr<FUICommandInfo> is bound to its input gesture, label, 
	 * and tooltip. Called automatically when the command framework is initialized.
	 */
	virtual void RegisterCommands() override;

	/**
	 * @brief Retrieves all registered commands for this mode.
	 * 
	 * @return Map of command categories to arrays of command info shared pointers.
	 */
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();

public:
	/** Command for opening the 2D window in the cloth design editor */
	TSharedPtr<FUICommandInfo> Open2DWindow;

protected:
	/** Internal map storing registered commands by category */
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;
};

#endif
