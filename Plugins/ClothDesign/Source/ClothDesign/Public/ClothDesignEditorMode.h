
#pragma once

// #include "Tools/UEdMode.h"
#include "UnrealWidgetFwd.h"

#include "Tools/LegacyEdModeWidgetHelpers.h"
#include "ClothDesignEditorMode.generated.h"

 UCLASS()

class CLOTHDESIGN_API UClothDesignEditorMode : public UBaseLegacyWidgetEdMode

{
	GENERATED_BODY()

public:
	const static FEditorModeID EM_ClothDesignEditorModeId;
 	
	UClothDesignEditorMode();
	// virtual ~UClothDesignEditorMode() override;

	/** UEdMode interface */
	virtual void Enter() override;
	virtual void ActorSelectionChangeNotify() override;
	virtual void CreateToolkit() override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override; // took out override

 	virtual bool UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const override { return true; }
 	virtual bool ShouldDrawWidget() const override { return true; }
 	virtual bool ShowModeWidgets() const override { return true; }
 	virtual bool CanCycleWidgetMode() const override { return true; }
};
