#ifndef FCanvasState_H
#define FCanvasState_H

# include "Canvas/CanvasSewing.h"

/*
 * Thesis reference:
 * See Chapter 4.4 and 4.9.1 for detailed explanations.
 */

/**
 * @brief Represents the complete state of the cloth design canvas at a given moment.
 *
 * This struct encapsulates all relevant information required to render the canvas,
 * manage user interactions, undo/redo operations, and handle sewing operations.
 * The intent is to provide a single, serialisable snapshot of the canvas state
 * so that operations such as undo, redo, and temporary previews can be performed
 * consistently without relying on scattered state.
 */
struct FCanvasState
{
    /** 
     * @brief The curve currently being drawn by the user.
     *
     * Stores the control points for the active curve. Maintaining this separately
     * allows the system to provide live feedback and undo functionality while
     * preserving the original shapes.
     */
    FInterpCurve<FVector2D> CurvePoints;

    /**
     * @brief All shapes that have been completed on the canvas.
     *
     * Each shape is stored as a curve, allowing the system to maintain
     * persistent geometry and provide operations such as mesh generation
     * and sewing alignment.
     */
    TArray<FInterpCurve<FVector2D>> CompletedShapes;

    /**
     * @brief The index of the currently selected point on the active curve.
     *
     * Tracking the selected point is essential for move, delete, or edit
     * operations and ensures the UI responds correctly to user input.
     */
    int32 SelectedPointIndex;

    /**
     * @brief The current pan offset of the canvas in screen coordinates.
     *
     * Separating the pan offset from the shapes allows smooth camera-like
     * movement without modifying the underlying geometry.
     */
    FVector2D PanOffset;

    /**
     * @brief The current zoom factor of the canvas.
     *
     * Used to scale the canvas and provide consistent UI feedback, enabling
     * precise user interactions across different zoom levels.
     */
    float ZoomFactor;

    /**
     * @brief Bezier usage flags for each point of the active curve.
     *
     * Determines whether individual points of the current curve should
     * use Bezier handles. This allows mixed curve types within the same canvas.
     */
    TArray<bool> bUseBezierPerPoint;

    /**
     * @brief Bezier usage flags for each point of completed shapes.
     *
     * Stores which points of completed shapes used Bezier handles, preserving
     * the original geometry for undo/redo and rendering.
     */
    TArray<TArray<bool>> CompletedBezierFlags;

    // --- Sewing data ---

    /**
     * @brief Full seam definitions that have been added to the canvas.
     *
     * Contains the start and end indices of sewn edges between shapes.
     * Keeping this allows the system to persist sewing operations and
     * regenerate meshes or previews without redoing calculations.
     */
    TArray<FSeamDefinition> SeamDefinitions;

    /**
     * @brief Transient preview points shown while sewing.
     *
     * Maps shape indices to sets of points that are temporarily highlighted
     * during user interaction, allowing the user to visualise proposed seams
     * before committing.
     */
    TMap<int32, TSet<int32>> SeamPreviewPoints;

    /**
     * @brief Stores the current state of the seam click process.
     *
     * Represents the sequential steps of selecting start and end points for
     * two shapes. Stored as an integer for efficient comparison and
     * serialisation.
     */
    int32 SeamClickState = 0;

    /** Shape and point indices for the start and end targets of the first seam side. */
    FIntPoint AStartTarget = FIntPoint(INDEX_NONE, INDEX_NONE); /**< ShapeIndex, PointIndex */
    FIntPoint AEndTarget   = FIntPoint(INDEX_NONE, INDEX_NONE); /**< ShapeIndex, PointIndex */

    /** Shape and point indices for the start and end targets of the second seam side. */
    FIntPoint BStartTarget = FIntPoint(INDEX_NONE, INDEX_NONE); /**< ShapeIndex, PointIndex */
    FIntPoint BEndTarget   = FIntPoint(INDEX_NONE, INDEX_NONE); /**< ShapeIndex, PointIndex */

    /**
     * @brief The index of the currently selected seam.
     *
     * Allows the system to highlight or modify a specific seam
     * while preserving the rest of the canvas state.
     */
    int32 SelectedSeamIndex = INDEX_NONE;

    /**
     * @brief Equality operator.
     *
     * Compares curve points, Bezier flags, completed shapes, selection indices,
     * pan offset, and zoom factor to determine if two canvas states are equivalent.
     * Excludes sewing preview points and seam definitions for efficiency.
     *
     * @param Other The canvas state to compare against.
     * @return true if the states are effectively identical, false otherwise.
     */
    bool operator==(const FCanvasState& Other) const
    {
        return  CurvePoints == Other.CurvePoints &&
                bUseBezierPerPoint == Other.bUseBezierPerPoint && 
                CompletedShapes == Other.CompletedShapes &&
                CompletedBezierFlags == Other.CompletedBezierFlags &&
                SelectedPointIndex == Other.SelectedPointIndex &&
                PanOffset == Other.PanOffset &&
                FMath::IsNearlyEqual(ZoomFactor, Other.ZoomFactor);
    }
};


#endif

