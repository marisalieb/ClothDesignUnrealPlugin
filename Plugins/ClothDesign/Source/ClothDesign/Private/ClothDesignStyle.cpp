#include "ClothDesignStyle.h"

#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/SlateGameResources.h"

TSharedPtr<FSlateStyleSet> FClothDesignStyle::StyleInstance = nullptr;

// void FClothDesignStyle::Initialize()
// {
// 	if (!StyleInstance.IsValid())
// 	{
// 		StyleInstance = Create();
// 		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
// 	}
// }

void FClothDesignStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

		// Set plugin content root (adjust path to your plugin folder)
		FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("ClothDesign"))->GetBaseDir() / TEXT("Resources");
		StyleInstance->SetContentRoot(ContentDir);

		// Register your icon (use the correct filename here)
		StyleInstance->Set("ClothDesignIcon", new FSlateImageBrush(
			StyleInstance->RootToContentDir(TEXT("cloth_icon40.png")), FVector2D(40, 40)));

		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance.Get());
	}
}


void FClothDesignStyle::Shutdown()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance.Get());
		ensure(StyleInstance.IsUnique());
		StyleInstance.Reset();
	}
}

FName FClothDesignStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ClothDesignEditorModeStyle"));
	return StyleSetName;
}

const ISlateStyle& FClothDesignStyle::Get()
{
	return *StyleInstance;
}
//
// TSharedRef<FSlateStyleSet> FClothDesignStyle::Create()
// {
// 	TSharedRef<FSlateStyleSet> StyleRef = MakeShareable(new FSlateStyleSet("ClothDesignEditorModeStyle"));
//
// 	// FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("ClothDesign"))->GetBaseDir() / TEXT("Resources");
// 	// StyleRef->SetContentRoot(ContentDir);
//
// 	const FVector2D Icon40(40.0f, 40.0f);
// 	const FVector2D Icon128(128.0f, 128.0f);
// 	
// 	// StyleRef->Set("ClothDesignMode.Icon128", new FSlateImageBrush(StyleRef->RootToContentDir(TEXT("cloth_icon.png")), Icon128));
// 	// StyleRef->Set("ClothDesignMode.Icon40", new FSlateImageBrush(StyleRef->RootToContentDir(TEXT("cloth_icon40.png")), Icon40));
//
// 	return StyleRef;
// }
