#ifndef FClothDesignCommands_H
#define FClothDesignCommands_H

#include "Framework/Commands/Commands.h"

/*
 * Thesis reference:
 * See Chapter 4.2 for details.
 */


/**
 * @class FClothDesignCommands
 * @brief Handles the registration and management of commands for the Cloth Design Editor Mode.
 * 
 * This class defines and manages UI commands used in the Cloth Design Editor Mode.
 * 
 * @note For implementation details, see Chapter 4.2 of the thesis.
 */
class FClothDesignCommands : public TCommands<FClothDesignCommands>
{
public:
	/**
	 * @brief Constructor for the FClothDesignCommands class.
	 * 
	 * Initializes the command context for the Cloth Design Editor Mode.
	 */
	FClothDesignCommands();

	/**
	 * @brief Registers all commands for the Cloth Design Editor Mode.
	 * 
	 * This method defines and binds UI commands, such as the "Open 2D Window" command.
	 */
	virtual void RegisterCommands() override;

	/**
	 * @brief Retrieves all registered commands.
	 * 
	 * @return A map containing the registered commands, grouped by their names.
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
