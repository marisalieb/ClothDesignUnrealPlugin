
#include "ClothDesignEditorMode.h"
#include "ClothDesignToolkit.h"
#include "ClothDesignCommands.h"



#define LOCTEXT_NAMESPACE "ClothDesignEditorMode"

const FEditorModeID UClothDesignEditorMode::EM_ClothDesignEditorModeId = TEXT("EM_ClothDesignEditorMode");


UClothDesignEditorMode::UClothDesignEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(
		UClothDesignEditorMode::EM_ClothDesignEditorModeId,
		LOCTEXT("ModeName", "ClothDesign"),
		FSlateIcon(),
		true);
}

// UClothDesignEditorMode::~UClothDesignEditorMode()
// {
// }

void UClothDesignEditorMode::ActorSelectionChangeNotify()
{
}

void UClothDesignEditorMode::Enter()
{
	UEdMode::Enter();
}

void UClothDesignEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FClothDesignToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UClothDesignEditorMode::GetModeCommands() const
{
	return FClothDesignCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
