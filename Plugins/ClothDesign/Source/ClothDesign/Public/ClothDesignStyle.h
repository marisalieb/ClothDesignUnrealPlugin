#ifndef FClothDesignStyle_H
#define FClothDesignStyle_H

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"

/*
 * Thesis reference:
 * See Chapter 4.2 for details.
 */



/**
 * @brief Manages the visual style set for the Cloth Design editor.
 * 
 * This class provides a centralized Slate style for UI elements in the cloth design editor,
 * including icons, colors, and fonts. It allows initialization and shutdown of the style set,
 * as well as access to the shared style instance.
 */
class FClothDesignStyle
{
public:
	/**
	 * @brief Initializes the style set and registers it with the Slate style registry.
	 */
	static void Initialize();

	/**
	 * @brief Shuts down the style set and unregisters it from the Slate style registry.
	 */
	static void Shutdown();

	/**
	 * @brief Retrieves the shared Slate style instance.
	 * 
	 * @return Reference to the ISlateStyle interface representing the current style.
	 */
	static const ISlateStyle& Get();

	/**
	 * @brief Retrieves the FName identifier for this style set.
	 * 
	 * @return Name of the style set.
	 */
	static FName GetStyleSetName();

private:
	/** Internal shared pointer to the style set instance */
	static TSharedPtr<FSlateStyleSet> StyleInstance;
};

#endif
