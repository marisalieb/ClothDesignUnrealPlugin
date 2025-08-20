#pragma once
// Using #pragma once here because this header contains UCLASS macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

#include "UnrealWidgetFwd.h"
#include "Tools/LegacyEdModeWidgetHelpers.h"
#include "ClothDesignEditorMode.generated.h"

/*
 * Thesis reference:
 * See Chapter 4.2 for details.
 */


/**
 * @brief Custom editor mode for cloth design in the editor.
 * 
 * This class defines a specialized editor mode for manipulating cloth assets. 
 * It extends UBaseLegacyWidgetEdMode to provide interactive widgets, selection handling, 
 * and toolkit support specific to the cloth design workflow.
 */
UCLASS()
class CLOTHDESIGN_API UClothDesignEditorMode : public UBaseLegacyWidgetEdMode
{
	GENERATED_BODY()

public:
	/** Unique identifier for this editor mode. */
	const static FEditorModeID EM_ClothDesignEditorModeId;
 	
	/** Constructor */
	UClothDesignEditorMode();

	// --- UEdMode interface ---

	/**
	 * @brief Called when entering this editor mode.
	 * Use this to initialize mode-specific state and UI.
	 */
	virtual void Enter() override;

	/**
	 * @brief Called when the actor selection changes in the editor.
	 * Handles any updates necessary when selection changes.
	 */
	virtual void ActorSelectionChangeNotify() override;

	/**
	 * @brief Creates the editor toolkit for this mode.
	 * This provides custom UI panels for cloth design tools.
	 */
	virtual void CreateToolkit() override;

	/**
	 * @brief Retrieves the UI commands associated with this editor mode.
	 * 
	 * @return Map of command categories to arrays of command info shared pointers.
	 */
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;

	// --- Transform widget behavior overrides ---

	/**
	 * @brief Determines whether the transform widget should be enabled.
	 * 
	 * @param CheckMode The widget mode to check.
	 * @return true if transform widgets should be active in this mode.
	 */
	virtual bool UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const override { return true; }

	/**
	 * @brief Determines whether the transform widget should be drawn.
	 * 
	 * @return true if the widget should be visible.
	 */
	virtual bool ShouldDrawWidget() const override { return true; }

	/**
	 * @brief Determines whether mode-specific widgets should be shown.
	 * 
	 * @return true if mode widgets should be displayed.
	 */
	virtual bool ShowModeWidgets() const override { return true; }

	/**
	 * @brief Determines whether the widget mode can be cycled.
	 * 
	 * @return true if cycling between transform widget modes is allowed.
	 */
	virtual bool CanCycleWidgetMode() const override { return true; }
};
