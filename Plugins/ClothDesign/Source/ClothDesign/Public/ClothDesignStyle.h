#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"

/**
 * @class FClothDesignStyle
 * @brief Manages the custom Slate style set used for the Cloth Design editor UI.
 *
 * This class is responsible for registering, initializing, and providing access to
 * a centralized style set for Slate widgets within the Cloth Design editor module.
 * The style set contains brushes, colors, fonts, and other visual resources.
 *
 * The style is implemented as a singleton-like static instance to ensure it is
 * only loaded and registered once during the module's lifetime.
 *
 * Typical usage:
 * @code
 * FClothDesignStyle::Initialize();
 * const ISlateStyle& Style = FClothDesignStyle::Get();
 * FClothDesignStyle::Shutdown();
 * @endcode
 */
class FClothDesignStyle
{
public:

	/**
	 * @brief Initializes the style set if it has not been created already.
	 * 
	 * Registers the style set with the Slate style registry, making its
	 * resources available to Slate widgets.
	 */
	static void Initialize();

	/**
	 * @brief Shuts down the style set and unregisters it from the Slate style registry.
	 *
	 * This should be called during module shutdown to free resources.
	 */
	static void Shutdown();

	/**
	 * @brief Retrieves the current Slate style instance.
	 * @return A constant reference to the active style set.
	 * @note This should only be called after Initialize().
	 */
	static const ISlateStyle& Get();

	/**
	 * @brief Gets the FName identifier of this style set.
	 * @return The name used to register and look up the style set.
	 */
	static FName GetStyleSetName();

private:

	// /**
	//  * @brief Creates the Slate style set with all custom brushes, fonts, and colors.
	//  * @return A shared reference to the newly created style set.
	//  */
	// static TSharedRef<class FSlateStyleSet> Create();

	/** 
	 * @brief Shared pointer to the Slate style set instance used for the Cloth Design editor.
	 */
	static TSharedPtr<class FSlateStyleSet> StyleInstance;
};
