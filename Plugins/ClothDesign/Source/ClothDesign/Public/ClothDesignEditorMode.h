#pragma once
// Using #pragma once here because this header contains UCLASS macros
// UnrealHeaderTool (UHT) requires that reflected types are NOT inside #ifndef/#define include guards

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

	// UEdMode interface
	virtual void Enter() override;
	virtual void ActorSelectionChangeNotify() override;
	virtual void CreateToolkit() override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;

 	// this enables the transform widgets at the top of the editor mode which would otherwise be disabled
 	virtual bool UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const override { return true; }
 	virtual bool ShouldDrawWidget() const override { return true; }
 	virtual bool ShowModeWidgets() const override { return true; }
 	virtual bool CanCycleWidgetMode() const override { return true; }
};
