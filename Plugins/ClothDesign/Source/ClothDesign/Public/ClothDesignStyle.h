#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"

class FClothDesignStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static const ISlateStyle& Get();
	static FName GetStyleSetName();

private:
	static TSharedPtr<FSlateStyleSet> StyleInstance;
};