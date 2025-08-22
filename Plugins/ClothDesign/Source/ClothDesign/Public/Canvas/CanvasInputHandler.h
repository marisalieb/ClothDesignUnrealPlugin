#ifndef FCanvasInputHandler_H
#define FCanvasInputHandler_H

#include "CoreMinimal.h"
#include "Input/Reply.h"

// /*
//  * Thesis reference:
//  * See Chapter 4.5 (HandleDraw), 4.7 (HandleSew) and 4.9.2 (HandleSelect) for details.
//  */

/**
 * 
 * @note Thesis reference:
 * See Chapter 4.5 (HandleDraw), 4.7 (HandleSew) and 4.9.2 (HandleSelect) for details.
 */


/**
 * @brief Forward declaration of the cloth design canvas.
 */
class SClothDesignCanvas;


/**
 * @brief Handles user input for the canvas.
 * 
 * This class exists to translate user interactions into meaningful actions on the canvas,
 * ensuring that input is interpreted correctly according to the design logic.
 */
class FCanvasInputHandler
{
	
public:
	/**
	 * @brief Constructs an input handler associated with a canvas.
	 * @param InCanvas Pointer to the canvas to associate with.
	 * 
	 * This initialises the handler so that it can manage inputs specifically for this canvas,
	 * maintaining a clear link between user actions and canvas updates.
	 */
	explicit FCanvasInputHandler(SClothDesignCanvas* InCanvas);

	/**
	 * @brief Handles panning of the canvas.
	 * @param MouseEvent The pointer event representing user input.
	 * @return FReply indicating whether the event was handled.
	 * 
	 * Panning is handled to allow the user to reposition their view, 
	 * giving them control over how they examine or interact with the cloth.
	 */
	FReply HandlePan(const FPointerEvent& MouseEvent);

	/**
	 * @brief Handles drawing input on the canvas.
	 * @param Geo The geometry of the canvas.
	 * @param MouseEvent The pointer event representing user input.
	 * @return FReply indicating whether the event was handled.
	 * 
	 * Drawing is interpreted here to enable the user to define shapes or patterns,
	 * translating physical gestures into design elements.
	 */
	FReply HandleDraw(const FGeometry& Geo, const FPointerEvent& MouseEvent);

	/**
	 * @brief Handles sewing actions at a clicked location on the canvas.
	 * @param CanvasClick The 2D position of the click on the canvas.
	 * @return FReply indicating whether the event was handled.
	 * 
	 * Sewing is processed to facilitate connecting design segments, 
	 * reflecting how the user intends pieces of cloth to interact.
	 */
	FReply HandleSew(const FVector2D& CanvasClick);

	/**
	 * @brief Handles selection actions at a clicked location on the canvas.
	 * @param CanvasClick The 2D position of the click on the canvas.
	 * @return FReply indicating whether the event was handled.
	 * 
	 * Selection exists so the user can choose specific elements for modification,
	 * supporting precise control over the design.
	 */
	FReply HandleSelect(const FVector2D& CanvasClick);

private:
	SClothDesignCanvas* Canvas; /**< The canvas being controlled, to maintain context for input handling. */
	bool bIsSeamReady = false; /**< Indicates if a seam is ready, helping to prevent incomplete operations. */

	/**
	 * @brief Calculates squared distance from a point to a line segment.
	 * @param P The point to measure from.
	 * @param A One endpoint of the line segment.
	 * @param B The other endpoint of the line segment.
	 * @return Squared distance from point P to segment AB.
	 * 
	 * This utility exists to determine proximity efficiently, supporting actions like snapping or selection 
	 * without unnecessary computation.
	 */
	static float DistPointToSegmentSq(
		const FVector2D& P,
		const FVector2D& A,
		const FVector2D& B);
};

#endif