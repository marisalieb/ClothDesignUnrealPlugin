#ifndef FCanvasPaint_H
#define FCanvasPaint_H

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

/*
 * Thesis reference:
 * See Chapter 4.5 for detailed explanations.
 */

class SClothDesignCanvas;
struct FGeometry;
class FSlateWindowElementList;

class FCanvasPaint
{

	FCanvasPaint() = default;
	FCanvasPaint(SClothDesignCanvas* InCanvas) : Canvas(InCanvas) {}
	
	int32 DrawBackground(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer) const;

	
	void DrawGridLines(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer,
		bool bVertical,
		float Spacing,
		const FLinearColor& Color,
		bool bSkipMajor) const;

	int32 DrawGrid(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer) const;

	static void BuildShortestArcSegments(
		int32 StartIdx, int32 EndIdx,
		int32 NumPts, TSet<int32>& OutSegments);
	
	int32 DrawCompletedShapes(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer) const;
	
	int32 DrawCurrentShape(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer) const;


	int DrawFinalisedSeamLines(
		const FGeometry& Geo,
		FSlateWindowElementList& OutDraw,
		int32 Layer) const;

private:
	SClothDesignCanvas* Canvas;

	
	const float WorldGridSpacing = 100.f;
	const int32 NumSubdivisions = 10;
	const float SubGridSpacing = WorldGridSpacing / NumSubdivisions;

	// ---- UI colours ---- 
	static const FLinearColor GridColour;
	static const FLinearColor GridColourSmall;

	static const FLinearColor LineColour;
	static const FLinearColor CompletedLineColour;

	static const FLinearColor PointColour;
	static const FLinearColor PostCurrentPointColour;

	static const FLinearColor BezierHandleColour;
	static const FLinearColor CompletedBezierHandleColour;

	static const FLinearColor SewingLineColour;
	static const FLinearColor SewingPointColour;
	
};

#endif
