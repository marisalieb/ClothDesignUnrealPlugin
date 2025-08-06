#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"


class SClothDesignCanvas;
struct FGeometry;
class FSlateWindowElementList;

struct FCanvasPaint
{
	SClothDesignCanvas* Canvas;

	FCanvasPaint(SClothDesignCanvas* InCanvas) : Canvas(InCanvas) {}
	
	const float WorldGridSpacing = 100.f;
	// --- Smaller Grid Lines ---
	const int32 NumSubdivisions = 10;
	const float SubGridSpacing = WorldGridSpacing / NumSubdivisions;
	
	int32 DrawBackground(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer);

	
	void DrawGridLines(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer,
		bool bVertical,
		float Spacing,
		const FLinearColor& Color,
		bool bSkipMajor
	);

	int32 DrawGrid(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer);
	
	int32 DrawCompletedShapes(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer);
	
	int32 DrawCurrentCurve(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer);
};
