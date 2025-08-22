#ifndef SClothDesignCanvas_H
#define SClothDesignCanvas_H

#include "Widgets/SCompoundWidget.h"
#include "Math/InterpCurve.h"
#include "PatternMesh.h"
#include "Misc/ScopeLock.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "PatternCreation/PatternAssets.h"
#include "PatternCreation/PatternSewing.h"

/*
 * Thesis reference:
 * See Chapter 4.2 and 4.4 for detailed explanations.
 */


/**
 * @brief Primary Slate widget for the cloth design canvas.
 *
 * This widget encapsulates the entire interactive canvas used to draw shapes,
 * define seams, preview sewing, and generate meshes. It centralises input handling,
 * rendering, and state management so the editor UI and underlying geometry remain
 * consistent and testable.
 *
 * @note Thesis reference: See Chapter 4.x for the canvas interaction and rendering rationale.
 */
class SClothDesignCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClothDesignCanvas) {}
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the canvas widget with Slate arguments.
	 *
	 * The construct function initialises the widget, binds callbacks and prepares
	 * internal state so the canvas is ready to render and receive input.
	 *
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * @brief Indicates this widget can receive keyboard focus.
	 *
	 * Required so keyboard shortcuts and key-driven interactions work reliably.
	 *
	 * @return true to allow keyboard focus.
	 */
	virtual bool SupportsKeyboardFocus() const override { return true; }

	/**
	 * @brief Transforms a canvas-space point into screen/widget space.
	 *
	 * Marked virtual to allow tests (and painters) to override coordinate transforms,
	 * ensuring rendering and hit-testing can be validated independently.
	 *
	 * @param Point The point in canvas (logical) coordinates.
	 * @return Transformed point in screen/widget coordinates.
	 */
	virtual FVector2D TransformPoint(const FVector2D& Point) const; // virtual for canvaspaint tests

	/**
	 * @brief Inverse of TransformPoint: converts screen space to canvas space.
	 *
	 * Used for hit-testing, mouse-to-canvas conversions, and consistent input handling.
	 *
	 * @param ScreenPoint Point in screen/widget coordinates.
	 * @return Corresponding point in canvas (logical) coordinates.
	 */
	virtual FVector2D InverseTransformPoint(const FVector2D& ScreenPoint) const;

	// --- Drawing logic ---

	/**
	 * @brief Main Slate paint entry for the widget.
	 *
	 * Separates background, grid, shapes and seam drawing so that rendering is predictable
	 * and testable. Returns the next available rendering layer to allow callers to stack
	 * additional UI on top.
	 *
	 * @param Args Painting arguments passed from Slate.
	 * @param AllottedGeometry Geometry of the widget area to paint into.
	 * @param CullingRect Rectangle used for culling draw operations.
	 * @param OutDrawElements The element list to append draw commands to.
	 * @param LayerId The initial layer id to draw at.
	 * @param InWidgetStyle Widget style information.
	 * @param bParentEnabled Whether the parent widget is enabled.
	 * @return The next free layer id after drawing.
	 */
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
						  const FSlateRect& CullingRect, FSlateWindowElementList& OutDrawElements,
						  int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// --- Mouse and key handling ---

	/**
	 * @brief Handles mouse wheel events for zooming or other context-sensitive actions.
	 *
	 * Zooming is handled here to keep zoom logic local to the canvas and consistent
	 * across input sources.
	 *
	 * @param Geometry Widget geometry at the time of the event.
	 * @param MouseEvent The mouse wheel event details.
	 * @return FReply indicating whether the event was handled.
	 */
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles mouse button down events (start of click/drag operations).
	 *
	 * Centralises hit-testing and prepares the canvas for dragging, point selection,
	 * or initiating seam workflows.
	 *
	 * @param Geometry Widget geometry at the time of the event.
	 * @param MouseEvent Details of the mouse button event.
	 * @return FReply indicating whether the event was handled.
	 */
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles mouse move events for dragging, previewing and cursor updates.
	 *
	 * Keeps live previews and drag state consistent with current input and transform.
	 *
	 * @param Geometry Widget geometry at the time of the event.
	 * @param MouseEvent Details of the pointer movement.
	 * @return FReply indicating whether the event was handled.
	 */
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles mouse button up events to finalise clicks or drags.
	 *
	 * Completes interactions started in OnMouseButtonDown and ensures state is consistent.
	 *
	 * @param Geometry Widget geometry at the time of the event.
	 * @param MouseEvent Details of the mouse button release.
	 * @return FReply indicating whether the event was handled.
	 */
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles key down events for shortcuts and keyboard control.
	 *
	 * Keeping keyboard handling local allows context-sensitive shortcuts and editing
	 * operations to be implemented adjacent to canvas state.
	 *
	 * @param Geometry Widget geometry at time of key event.
	 * @param InKeyEvent The key event details.
	 * @return FReply indicating whether the event was handled.
	 */
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& InKeyEvent) override;

	/**
	 * @brief Handles key up events.
	 *
	 * Useful for completion logic for modifier-based interactions (e.g. Shift+drag).
	 *
	 * @param Geometry Widget geometry at time of key event.
	 * @param InKeyEvent The key event details.
	 * @return FReply indicating whether the event was handled.
	 */
	virtual FReply OnKeyUp(const FGeometry& Geometry, const FKeyEvent& InKeyEvent) override;

	/**
	 * @brief Called when the widget receives keyboard focus.
	 *
	 * Used to update visual focus state and ensure keyboard-driven workflows
	 * behave consistently when focus changes.
	 *
	 * @param Geometry Widget geometry at focus time.
	 * @param InFocusEvent Focus event details.
	 * @return FReply indicating whether the event was handled.
	 */
	virtual FReply OnFocusReceived(const FGeometry& Geometry, const FFocusEvent& InFocusEvent) override;


	/**
	 * @brief Editor modes supported by the canvas.
	 *
	 * Modes define high-level input semantics (drawing, selecting/editing, sewing),
	 * which control how input events are interpreted.
	 */
	enum class EClothEditorMode
	{
		Draw,   /**< Drawing new shapes and editing current curve. */
		Select, /**< Selection/edit mode (labelled "edit" in the UI). */
		Sew     /**< Sewing mode for defining seams between shapes. */
	};

	/**
	 * @brief Switches the canvas to a different editor mode.
	 *
	 * Mode changes alter input interpretation and UI feedback; this function
	 * centralises that transition so state changes remain consistent.
	 *
	 * @param NewMode The mode to switch to.
	 * @return FReply suitable for UI binding (typically handled).
	 */
	FReply OnModeButtonClicked(EClothEditorMode NewMode);

	// --- Undo/Redo stacks ---

	/** Undo history for canvas states; used to revert user actions. */
	TArray<FCanvasState> UndoStack; /**< Maintains past states for undo operations. */

	/** Redo history for canvas states; used to reapply undone actions. */
	TArray<FCanvasState> RedoStack; /**< Maintains states that can be redone. */

	/**
	 * @brief Returns a copy of the current canvas state.
	 *
	 * Used by undo/redo code and by save/load operations so callers obtain a
	 * serialisable snapshot rather than relying on scattered member reads.
	 *
	 * @return A copy of the current FCanvasState.
	 */
	FCanvasState GetCurrentCanvasState() const;

	// Tangent handle selection for point editing
	/**
	 * @brief Which tangent handle is active when editing a point's Bezier handles.
	 *
	 * Allows the editor to support separate arrival/leave handles or linked tangents.
	 */
	enum class ETangentHandle
	{
		None,   /**< No tangent handle selected. */
		Arrive, /**< The incoming tangent handle. */
		Leave   /**< The outgoing tangent handle. */
	};

	/** Whether tangents are handled independently for arrival/leave. */
	bool bSeparateTangents = false; /**< If true, arrival and leave tangents are edited separately. */

	/** Currently selected tangent handle for editing. */
	ETangentHandle SelectedTangentHandle = ETangentHandle::None; /**< Determines which handle moves on drag. */

	// --- Background texture --

	/** Selected background texture (optional). */
	TWeakObjectPtr<UTexture2D> BackgroundTexture; /**< Holds a weak reference to the background image asset. */

	/**
	 * @brief Returns the file path of the selected background texture.
	 *
	 * Useful for displaying the currently-loaded texture in UI.
	 *
	 * @return Path string for the selected texture, or empty if none.
	 */
	FString GetSelectedTexturePath() const;

	/**
	 * @brief Callback invoked when a background texture is chosen by the user.
	 *
	 * Keeps texture selection and canvas rendering in sync.
	 *
	 * @param AssetData Asset metadata for the chosen texture.
	 */
	void OnBackgroundTextureSelected(const FAssetData& AssetData);

	/**
	 * @brief Returns the current scale applied to the background image.
	 *
	 * Used for rendering and for persisting image scale with the canvas state.
	 *
	 * @return Optional float containing the scale; empty optional if not applicable.
	 */
	TOptional<float> GetBackgroundImageScale() const;

	/**
	 * @brief Changes the background image scale.
	 *
	 * Adjusts visual presentation of the background without altering geometry.
	 *
	 * @param NewScale New scale factor to apply to the background image.
	 */
	void OnBackgroundImageScaleChanged(float NewScale);

	// --- Save / Load shapes and assets ---

	/**
	 * @brief Returns the path of the selected shape asset.
	 *
	 * Used by save/load UI to display or persist the selected asset location.
	 *
	 * @return Path string of the selected shape asset.
	 */
	FString GetSelectedShapeAssetPath() const;

	/**
	 * @brief Invoked when a shape asset is selected from the content browser.
	 *
	 * Triggers loading of the canvas state or preview data associated with the asset.
	 *
	 * @param AssetData Asset metadata for the selected shape.
	 */
	void OnShapeAssetSelected(const FAssetData& AssetData);

	/**
	 * @brief Clears all shape-related data from the canvas.
	 *
	 * Resets the canvas to an empty drawing state without removing user preferences.
	 */
	void ClearAllShapeData();

	// --- Sewing / merging / mesh generation UI callbacks ---

	/** Initiates the seam click workflow (UI button callback). */
	void SewingClick();

	/** Merges selected sewn pieces (UI button callback). */
	void MergeClick();

	/** Clears all sewing data (UI button callback). */
	void ClearAllSewing();

	/** Generates meshes for completed shapes (UI button callback). */
	void GenerateMeshesClick();

	/**
	 * @brief Saves the current canvas state under the given name.
	 *
	 * @param SaveName Name to use for the saved asset/state.
	 * @return FReply indicating whether the save action was handled (for UI binding).
	 */
	FReply SaveClick(const FString& SaveName);

	// UI accessors

	/**
	 * @brief Returns the editor's current mode (draw/select/sew).
	 *
	 * Useful for UI elements that need to reflect the active mode.
	 *
	 * @return Current EClothEditorMode.
	 */
	EClothEditorMode GetCurrentMode() const { return CurrentMode; }

	/** Access to the sewing manager for programmatic control. */
	FPatternSewing& GetSewingManager() { return SewingManager; }
	const FPatternSewing& GetSewingManager() const { return SewingManager; }

	// --- Core canvas state (user-editable) ---

	/** Current zoom factor used to map canvas units to screen units. */
	float ZoomFactor = 5.0f; /**< Controls visual scale for drawing and hit-testing. */

	/** Current pan offset in canvas units. */
	FVector2D PanOffset = FVector2D::ZeroVector; /**< Enables camera-like panning without changing geometry. */

	/** Index of the currently selected point (mutable to allow const paint operations to query). */
	mutable int32 SelectedPointIndex = INDEX_NONE; /**< Used for highlighting and editing a point. */

	/** Currently selected shape index (mutable to allow read-only operations to report selection). */
	mutable int32 SelectedShapeIndex = INDEX_NONE; /**< Used to focus operations on a single shape. */

	/** Last recorded mouse position in widget/screen coordinates. */
	FVector2D LastMousePos = FVector2D::ZeroVector; /**< Used to compute drags and deltas. */

	/** Whether a point is currently being dragged by the user. */
	bool bIsDraggingPoint = false; /**< Guards point-drag logic and prevents accidental edits. */

	/** Whether a whole shape is being dragged. */
	bool bIsDraggingShape = false; /**< Used to move entire shapes. */

	/** Whether a tangent handle is being dragged. */
	bool bIsDraggingTangent = false; /**< Used to manipulate Bezier tangents. */

	/** Whether any shape is currently selected. */
	bool bIsShapeSelected = false; /**< Affects UI affordances and operation availability. */

	/** Mutable flag used by paint/hit test code to indicate a drag is active. */
	mutable bool bIsDragging = false; /**< Allows const painting methods to know drag state. */

	/** Whether the canvas is currently panning (middle mouse drag etc.). */
	bool bIsPanning = false; /**< Panning separates camera movement from geometry edits. */

	/** Scale applied to the background image for rendering convenience. */
	float BackgroundImageScale = 1.0f; /**< Independent of ZoomFactor so image fits desired visual size. */

	/** Current working curve (points being drawn). */
	FInterpCurve<FVector2D> CurvePoints; /**< Stores control points for the active curve being edited. */

	/** Shapes that have been completed (committed) by the user. */
	TArray<FInterpCurve<FVector2D>> CompletedShapes; /**< Persistent curves used for mesh generation and sewing. */

	/** Flags indicating per-point Bezier usage for the active curve. */
	TArray<bool> bUseBezierPerPoint; /**< Allows mixed handle usage within a single curve. */

	/** Flags indicating per-point Bezier usage for completed shapes. */
	TArray<TArray<bool>> CompletedBezierFlags; /**< Preserves original curve topology for completed shapes. */

	/** Global toggle for whether to create Bezier points by default when adding new points. */
	bool bUseBezierPoints = true; /**< Gives a sensible default for new points while allowing per-point overrides. */

	/** Optional little text block reminding users of current mode or hints. */
	TSharedPtr<STextBlock> ModeReminderText; /**< UI element to show ephemeral instructions or mode reminders. */

	// --- Misc / workflow helpers ---

	/**
	 * @brief Finalises the current shape, optionally generating its mesh immediately.
	 *
	 * Moving the finalise logic into a single function ensures finalisation is consistent
	 * whether triggered by UI or programmatic flows.
	 *
	 * @param bGenerateNow If true, immediately generate meshes from the finalised shape.
	 * @param OutSpawnedActors Optional pointer to receive spawned pattern actors from generation.
	 * @return Index of the finalised shape within CompletedShapes, or INDEX_NONE on failure.
	 */
	int32 FinaliseCurrentShape(bool bGenerateNow = false, TArray<TWeakObjectPtr<APatternMesh>>* OutSpawnedActors = nullptr);

	/** Mapping of shape index -> set of sewn point indices for the shape. */
	TMap<int32, TSet<int32>> SewnPointIndicesPerShape; /**< Maintained to support seam previews and merging. */

	/**
	 * @brief Recomputes sewn point sets from seam definitions and current shape data.
	 *
	 * Keeps SewnPointIndicesPerShape in sync with seam definitions so merging and preview logic
	 * can operate on a precomputed data structure.
	 */
	void UpdateSewnPointSets();

	/** Index of the seam currently selected by the user (or INDEX_NONE). */
	int32 SelectedSeamIndex = INDEX_NONE; /**< Used for seam editing and highlight operations. */

private:
	/** Last geometry passed to OnPaint/on-input; cached for hit-testing and coordinate transforms. */
	FGeometry LastGeometry; /**< Cached geometry to avoid repeatedly querying Slate during input handling. */

	/** Manages save/load of canvas state tied to assets. */
	FPatternAssetManager AssetManager; /**< Centralises asset I/O so code does not duplicate load/save logic. */

	/** Sewing helper that encapsulates seam definition, validation and merging. */
	FPatternSewing SewingManager; /**< Separates seam workflow from canvas rendering and input logic. */

	/** Current editor mode (draw/select/sew). */
	EClothEditorMode CurrentMode = EClothEditorMode::Draw; /**< Default to draw mode on initialisation. */

	/**
	 * @brief Focuses the viewport on currently selected points or shapes.
	 *
	 * Used after operations that reposition geometry so the user remains oriented.
	 */
	void FocusViewportOnPoints();

	/**
	 * @brief Restores the canvas to the supplied state snapshot.
	 *
	 * Used for undo/redo and for loading saved canvas states.
	 *
	 * @param State The canvas state to restore.
	 */
	void RestoreCanvasState(const FCanvasState& State);

	/**
	 * @brief Deletes any procedural cloth meshes previously spawned in the scene.
	 *
	 * Ensures duplicate or stale actors are removed before generating or spawning new ones.
	 */
	void DeleteOldClothMeshesFromScene();

	/**
	 * @brief Returns whether at least two cloth mesh actors exist in the scene.
	 *
	 * Used to decide if merging or multi-mesh operations are applicable.
	 *
	 * @return true if two or more cloth meshes are present.
	 */
	bool AreAtLeastTwoClothMeshesInScene() const;

	/** Unit test access for state roundtrip tests. */
	friend class FClothCanvas_StateRoundtripTest;
};


#endif


