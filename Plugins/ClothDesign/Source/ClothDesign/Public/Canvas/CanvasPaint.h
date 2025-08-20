#ifndef FCanvasPaint_H
#define FCanvasPaint_H

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

// /*
//  * Thesis reference:
//  * See Chapter 4.5 for detailed explanations.
//  */

/** 
 * \file CanvasPaint.h
 *
 * \note Thesis reference: See Chapter 4.5 for detailed explanations.
 */

// Forward declarations of classes used by FCanvasPaint
class SClothDesignCanvas;           /**< Represents the cloth design canvas; used for querying shape data and canvas state. */
struct FGeometry;                   /**< Provides geometric information for Slate widgets (position, size, transform). */
class FSlateWindowElementList;      /**< Container for Slate draw elements, used to record rendering commands. */


/**
 * @brief Provides methods for drawing and rendering canvas elements in the cloth design editor.
 * 
 * This class encapsulates the painting logic for the canvas, including background rendering,
 * grid lines, completed shapes, current shapes, and seam lines. It ensures visual consistency 
 * and abstracts complex rendering logic from the main canvas class.
 */
class FCanvasPaint
{
public:
    /** Default constructor */
    FCanvasPaint() = default;

    /**
     * @brief Initialises the painter with a reference to the target canvas.
     * @param InCanvas The canvas instance that this painter will render.
     * 
     * Holding a reference to the canvas allows the painter to query shape data
     * and canvas state for rendering purposes.
     */
    FCanvasPaint(SClothDesignCanvas* InCanvas) : Canvas(InCanvas) {}

    /**
     * @brief Draws the canvas background.
     * @param Geo Geometry information for the canvas area.
     * @param OutDraw Slate element list to append draw commands to.
     * @param Layer The rendering layer to use.
     * @return The next available layer after drawing.
     * 
     * Provides a consistent background for all canvas rendering and separates
     * background painting from foreground elements.
     */
    int32 DrawBackground(
        const FGeometry& Geo,
        FSlateWindowElementList& OutDraw,
        int32 Layer) const;

    /**
     * @brief Draws a series of grid lines on the canvas.
     * @param Geo Geometry information for the canvas area.
     * @param OutDraw Slate element list to append draw commands to.
     * @param Layer The rendering layer to use.
     * @param bVertical Whether the lines are vertical (true) or horizontal (false).
     * @param Spacing Distance between major grid lines.
     * @param Color Colour of the lines.
     * @param bSkipMajor Whether to skip major grid lines for minor grid drawing.
     * 
     * Handles both major and minor grid lines, providing visual guides for shape placement.
     */
    void DrawGridLines(
        const FGeometry& Geo,
        FSlateWindowElementList& OutDraw,
        int32 Layer,
        bool bVertical,
        float Spacing,
        const FLinearColor& Color,
        bool bSkipMajor) const;

    /**
     * @brief Draws the complete grid over the canvas.
     * @param Geo Geometry information for the canvas area.
     * @param OutDraw Slate element list to append draw commands to.
     * @param Layer The rendering layer to use.
     * @return The next available layer after drawing.
     * 
     * Combines vertical and horizontal grid lines, including minor subdivisions,
     * to provide spatial context for drawing operations.
     */
    int32 DrawGrid(
        const FGeometry& Geo,
        FSlateWindowElementList& OutDraw,
        int32 Layer) const;

    /**
     * @brief Builds the shortest arc segments between points.
     * @param StartIdx Index of the start point.
     * @param EndIdx Index of the end point.
     * @param NumPts Total number of points in the sequence.
     * @param OutSegments Output set containing the indices of points along the shortest arc.
     * 
     * Used for optimised rendering of circular or looped shape segments.
     */
    static void BuildShortestArcSegments(
        int32 StartIdx, int32 EndIdx,
        int32 NumPts, TSet<int32>& OutSegments);

    /**
     * @brief Draws all completed shapes on the canvas.
     * @param Geo Geometry information for the canvas area.
     * @param OutDraw Slate element list to append draw commands to.
     * @param Layer The rendering layer to use.
     * @return The next available layer after drawing.
     * 
     * Provides visual feedback for previously drawn shapes while maintaining separation
     * from the shape currently being edited.
     */
    int32 DrawCompletedShapes(
        const FGeometry& Geo,
        FSlateWindowElementList& OutDraw,
        int32 Layer) const;

    /**
     * @brief Draws the shape currently being edited by the user.
     * @param Geo Geometry information for the canvas area.
     * @param OutDraw Slate element list to append draw commands to.
     * @param Layer The rendering layer to use.
     * @return The next available layer after drawing.
     * 
     * Ensures the active shape is highlighted distinctly to provide immediate visual feedback.
     */
    int32 DrawCurrentShape(
        const FGeometry& Geo,
        FSlateWindowElementList& OutDraw,
        int32 Layer) const;

    /**
     * @brief Draws seam lines for shapes that have been finalised.
     * @param Geo Geometry information for the canvas area.
     * @param OutDraw Slate element list to append draw commands to.
     * @param Layer The rendering layer to use.
     * @return The next available layer after drawing.
     * 
     * Displays sewing or connection lines for completed shapes, supporting the cloth design workflow.
     */
    int DrawFinalisedSeamLines(
        const FGeometry& Geo,
        FSlateWindowElementList& OutDraw,
        int32 Layer) const;

private:
    /** Pointer to the canvas instance to query shape data and state. */
    SClothDesignCanvas* Canvas;

    /** World spacing between major grid lines in units. */
    const float WorldGridSpacing = 100.f;

    /** Number of subdivisions per grid cell for minor lines. */
    const int32 NumSubdivisions = 10;

    /** Spacing between minor grid lines. */
    const float SubGridSpacing = WorldGridSpacing / NumSubdivisions;

    // ---- UI colours ---- 
    static const FLinearColor GridColour;                /**< Colour for major grid lines. */
    static const FLinearColor GridColourSmall;           /**< Colour for minor grid lines. */

    static const FLinearColor LineColour;                /**< Colour for the currently drawn line. */
    static const FLinearColor CompletedLineColour;       /**< Colour for completed lines. */

    static const FLinearColor PointColour;               /**< Colour for points on the canvas. */
    static const FLinearColor PostCurrentPointColour;    /**< Colour for points after current editing. */

    static const FLinearColor BezierHandleColour;        /**< Colour for active Bezier handles. */
    static const FLinearColor CompletedBezierHandleColour; /**< Colour for completed Bezier handles. */

    static const FLinearColor SewingLineColour;          /**< Colour for finalised sewing lines. */
    static const FLinearColor SewingPointColour;         /**< Colour for points on sewing lines. */
};


#endif
