#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"

class SClothDesignCanvas;   // forward-decl only
struct FGeometry;
struct FPointerEvent;


struct FCanvasInputHandler
{
	
public:
	// ctor takes a raw pointer—canvas.h need not include this header
	explicit FCanvasInputHandler(SClothDesignCanvas* InCanvas);

	// Each returns FReply::Handled() or Unhandled()
	FReply HandlePan(const FPointerEvent& MouseEvent);
	FReply HandleDraw(const FGeometry& Geo, const FPointerEvent& MouseEvent);
	FReply HandleSew(const FVector2D& CanvasClick);
	FReply HandleSelect(const FVector2D& CanvasClick);

private:
	SClothDesignCanvas* Canvas;
	bool bIsSeamReady = false;

	static float DistPointToSegmentSq(const FVector2D& P,
		const FVector2D& A, const FVector2D& B);

};
