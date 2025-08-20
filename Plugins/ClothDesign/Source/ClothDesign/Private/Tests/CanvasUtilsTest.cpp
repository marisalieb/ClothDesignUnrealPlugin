#include "Misc/AutomationTest.h"
#include "Canvas/CanvasUtils.h"
#include "DynamicMesh/DynamicMesh3.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasUtilsUndoRedoTest, 
    "CanvasUtils.UndoRedo", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasUtilsUndoRedoTest::RunTest(const FString& Parameters)
{
    TArray<FCanvasState> UndoStack;
    TArray<FCanvasState> RedoStack;
    FCanvasState Current;

    Current.SelectedPointIndex = 5;
    Current.ZoomFactor = 1.5f;
    Current.PanOffset = FVector2D(10, 20);

    // save state
    FCanvasUtils::SaveStateForUndo(UndoStack, RedoStack, Current);

    TestEqual(TEXT("Undo stack should have 1 item"), UndoStack.Num(), 1);
    TestEqual(TEXT("Redo stack should be empty"), RedoStack.Num(), 0);

    // change current state
    Current.SelectedPointIndex = 7;

    // undo
    bool bUndid = FCanvasUtils::Undo(UndoStack, RedoStack, Current);
    TestTrue(TEXT("Undo should succeed"), bUndid);
    TestEqual(TEXT("Current state restored SelectedPointIndex"), Current.SelectedPointIndex, 5);
    TestEqual(TEXT("Redo stack should have 1 item"), RedoStack.Num(), 1);

    // redo
    bool bRedid = FCanvasUtils::Redo(UndoStack, RedoStack, Current);
    TestTrue(TEXT("Redo should succeed"), bRedid);
    TestEqual(TEXT("Current state restored SelectedPointIndex after redo"), Current.SelectedPointIndex, 7);
    TestEqual(TEXT("Undo stack should have 1 item again"), UndoStack.Num(), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasUtilsRecalculateTangentsTest, 
    "CanvasUtils.RecalculateNTangents", 
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasUtilsRecalculateTangentsTest::RunTest(const FString& Parameters)
{
    FInterpCurve<FVector2D> Curve;
    TArray<bool> bBezierFlags;

    // add points
    Curve.Points.Add(FInterpCurvePoint<FVector2D>(0.0f, FVector2D(0,0),
        FVector2D::ZeroVector, FVector2D::ZeroVector, CIM_Linear));
    Curve.Points.Add(FInterpCurvePoint<FVector2D>(1.0f, FVector2D(1,1),
        FVector2D::ZeroVector, FVector2D::ZeroVector, CIM_Linear));
    Curve.Points.Add(FInterpCurvePoint<FVector2D>(2.0f, FVector2D(2,0),
        FVector2D::ZeroVector, FVector2D::ZeroVector, CIM_Linear));

    bBezierFlags.Init(false, 3);

    FCanvasUtils::RecalculateNTangents(Curve, bBezierFlags);

    // test that tangents are computed
    TestTrue(TEXT("Point 0 ArriveTangent should be zero"), Curve.Points[0].ArriveTangent.IsNearlyZero());
    TestTrue(TEXT("Point 0 LeaveTangent should be half the vector to next"), 
        Curve.Points[0].LeaveTangent.Equals((FVector2D(1,1)-FVector2D(0,0))*0.5f));

    TestTrue(TEXT("Point 1 ArriveTangent should be half the vector from prev"), 
        Curve.Points[1].ArriveTangent.Equals((FVector2D(1,1)-FVector2D(0,0))*0.5f));
    TestTrue(TEXT("Point 1 LeaveTangent should be half the vector to next"), 
        Curve.Points[1].LeaveTangent.Equals((FVector2D(2,0)-FVector2D(1,1))*0.5f));

    TestTrue(TEXT("Point 2 LeaveTangent should be zero"), Curve.Points[2].LeaveTangent.IsNearlyZero());

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasUtilsCentroidAndTranslateTest,
    "CanvasUtils.MeshCentroidTranslate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasUtilsCentroidAndTranslateTest::RunTest(const FString& Parameters)
{
    UE::Geometry::FDynamicMesh3 Mesh;

    // create simple triangle mesh
    int v0 = Mesh.AppendVertex(FVector3d(0,0,0));
    int v1 = Mesh.AppendVertex(FVector3d(1,0,0));
    int v2 = Mesh.AppendVertex(FVector3d(0,1,0));
    Mesh.AppendTriangle(v0,v1,v2);

    FVector3d Centroid = FCanvasUtils::ComputeAreaWeightedCentroid(Mesh);
    TestTrue(TEXT("Centroid approximately correct"), Centroid.Equals(FVector3d(1.0/3, 1.0/3, 0), 1e-5));

    TArray<FVector> Vertices;
    Vertices.Add(FVector(1,1,0));
    FCanvasUtils::CenterMeshVerticesToOrigin(Vertices, FVector(1,1,0));
    TestTrue(TEXT("Vertices centered to origin"), Vertices[0].IsNearlyZero());

    // translate dynamic mesh by offset
    FCanvasUtils::TranslateDynamicMeshBy(Mesh, Centroid);
    FVector3d NewCentroid = FCanvasUtils::ComputeAreaWeightedCentroid(Mesh);
    TestTrue(TEXT("Translated mesh centroid at origin"), NewCentroid.IsNearlyZero());

    return true;
}
