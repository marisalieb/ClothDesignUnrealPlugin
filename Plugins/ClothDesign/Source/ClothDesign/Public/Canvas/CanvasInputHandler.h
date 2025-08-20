#ifndef FCanvasInputHandler_H
#define FCanvasInputHandler_H

#include "CoreMinimal.h"
#include "Input/Reply.h"

class SClothDesignCanvas;

struct FCanvasInputHandler
{
	
public:
	explicit FCanvasInputHandler(SClothDesignCanvas* InCanvas);

	// Each returns FReply::Handled() or Unhandled()
	FReply HandlePan(const FPointerEvent& MouseEvent);
	FReply HandleDraw(const FGeometry& Geo, const FPointerEvent& MouseEvent);
	FReply HandleSew(const FVector2D& CanvasClick);
	FReply HandleSelect(const FVector2D& CanvasClick);

private:
	SClothDesignCanvas* Canvas;
	bool bIsSeamReady = false;

	static float DistPointToSegmentSq(
		const FVector2D& P,
		const FVector2D& A,
		const FVector2D& B);
};

#endif