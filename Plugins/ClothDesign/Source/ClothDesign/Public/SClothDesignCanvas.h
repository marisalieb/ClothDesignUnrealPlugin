#pragma once

#include "Widgets/SCompoundWidget.h"
#include "CompGeom/PolygonTriangulation.h"
#include "ProceduralMeshComponent.h"

// using namespace UE::Geometry;
#include "VectorTypes.h"

class SClothDesignCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClothDesignCanvas) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// --- Mouse handling ---
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// --- Drawing logic ---
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
						  const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
						  int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// Trigger mesh generation from drawn shape
	void TriangulateAndBuildMesh();
	
protected:
	void CreateProceduralMesh(const TArray<FVector>& Vertices, const TArray<int32>& Indices);

private:
	TArray<FVector2D> Points;
};