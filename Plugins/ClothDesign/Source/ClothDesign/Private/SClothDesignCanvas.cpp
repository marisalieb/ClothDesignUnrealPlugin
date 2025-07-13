#include "SClothDesignCanvas.h"


// #include "SClothShapeCanvas.h"
#include "Rendering/DrawElements.h"

void SClothDesignCanvas::Construct(const FArguments& InArgs)
{
	// Optional: set focusable, mouse events, etc.
}

FReply SClothDesignCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D LocalClickPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		Points.Add(LocalClickPos);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

int32 SClothDesignCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
								 const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
								 int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// Draw lines between points
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2D>({ Points[i], Points[i + 1] }),
			ESlateDrawEffect::None,
			FLinearColor::Gray,
			true,
			2.0f
		);
	}

	// Draw closing line if shape is closed
	if (Points.Num() > 2)
	{
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2D>({ Points.Last(), Points[0] }),
			ESlateDrawEffect::None,
			FLinearColor::Gray,
			true,
			2.0f
		);
	}

	return LayerId;
}
