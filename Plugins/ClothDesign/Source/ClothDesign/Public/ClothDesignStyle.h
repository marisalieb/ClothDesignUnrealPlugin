#ifndef FClothDesignStyle_H
#define FClothDesignStyle_H

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"

/*
 * Thesis reference:
 * See Chapter 4.2 for details.
 */


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

#endif
