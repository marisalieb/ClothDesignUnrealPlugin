#include "ClothDesignStyle.h"

#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"



TSharedPtr<FSlateStyleSet> FClothDesignStyle::StyleInstance = nullptr;


void FClothDesignStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

		// Set plugin content root (adjust path to plugin folder)
		FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("ClothDesign"))->GetBaseDir() / TEXT("Resources");
		StyleInstance->SetContentRoot(ContentDir);

		// Register icon (use the correct filename here)
		StyleInstance->Set("ClothDesignIcon", new FSlateImageBrush(
			StyleInstance->RootToContentDir(TEXT("new_2.png")), FVector2D(40, 40)));

		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance.Get());
	}
}
/// ICON FROM : https://www.flaticon.com/free-icon/clothing-hanger_18409


void FClothDesignStyle::Shutdown()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance.Get());
		ensure(StyleInstance.IsUnique());
		StyleInstance.Reset();
	}
}

const ISlateStyle& FClothDesignStyle::Get()
{
	return *StyleInstance;
}

FName FClothDesignStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ClothDesignEditorModeStyle"));
	return StyleSetName;
}

