#include "Misc/AutomationTest.h"
#include "Canvas/CanvasMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "CoreMinimal.h"


#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasMeshTests, "CanvasMesh.UnitTests", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasMeshTests::RunTest(const FString& Parameters)
{
    // ---------------------------
    // 1) IsPointInPolygon
    // ---------------------------
    {
        TArray<FVector2f> Poly = { {0,0}, {0,5}, {5,5}, {5,0} };
        FVector2f Inside{2,2};
        FVector2f Outside{6,6};

        TestTrue("Inside point", FCanvasMesh::IsPointInPolygon(Inside, Poly));
        TestFalse("Outside point", FCanvasMesh::IsPointInPolygon(Outside, Poly));
    }

    // ---------------------------
    // 2) SampleShapeCurve
    // ---------------------------
    {
        FInterpCurve<FVector2D> Curve;
        Curve.AddPoint(0, {0,0});
        Curve.AddPoint(1, {1,1});
        Curve.AddPoint(2, {2,0});

        TArray<FVector2f> PolyVerts;
        TArray<int32> SeamVertexIDs;
        TArray<int32> VertexIDs;
        FDynamicMesh3 Mesh;

        FCanvasMesh::SampleShapeCurve(Curve, true, 0, 2, 2, PolyVerts, SeamVertexIDs, VertexIDs, Mesh);
        TestTrue("Curve should produce vertices", PolyVerts.Num() > 0);
    }

    // ---------------------------
    // 3) AddGridInteriorPoints
    // ---------------------------
    {
        TArray<FVector2f> PolyVerts = { {0,0}, {0,4}, {4,4}, {4,0} };
        TArray<int32> VertexIDs;
        FDynamicMesh3 Mesh;

        FCanvasMesh::AddGridInteriorPoints(PolyVerts, PolyVerts.Num(), VertexIDs, Mesh);

        TestTrue("AddGridInteriorPoints adds vertices", VertexIDs.Num() > 0);
    }

    // ---------------------------
    // 4) BuildBoundaryEdges
    // ---------------------------
    {
        TArray<UE::Geometry::FIndex2i> BoundaryEdges;
        FCanvasMesh::BuildBoundaryEdges(4, BoundaryEdges);
        TestEqual("BoundaryEdges count should match", BoundaryEdges.Num(), 4);
    }
    

    // ---------------------------
    // 6) ConvertCDTToDynamicMesh
    // ---------------------------
    {
        TArray<FVector2f> PolyVerts = { {0,0}, {0,4}, {4,4}, {4,0} };
        TArray<UE::Geometry::FIndex2i> BoundaryEdges;
        FCanvasMesh::BuildBoundaryEdges(PolyVerts.Num(), BoundaryEdges);

        UE::Geometry::TConstrainedDelaunay2<float> CDT;
        FCanvasMesh::RunConstrainedDelaunay(PolyVerts, BoundaryEdges, CDT);

        FDynamicMesh3 Mesh;
        TArray<int32> PolyIndexToVID;
        FCanvasMesh::ConvertCDTToDynamicMesh(CDT, Mesh, PolyIndexToVID);

        TestTrue("Mesh should have triangles (CDT internal data not directly testable)", Mesh.TriangleCount() > 0);
        TestTrue("Mesh should have vertices (CDT internal data not directly testable)", Mesh.VertexCount() > 0);
        TestTrue("PolyIndexToVID mapping", PolyIndexToVID.Num() > 0);
    }

    // ---------------------------
    // 7) ExtractVerticesAndIndices
    // ---------------------------
    {
        FDynamicMesh3 Mesh;
        // make a simple triangle
        int V0 = Mesh.AppendVertex(FVector3d(0,0,0));
        int V1 = Mesh.AppendVertex(FVector3d(1,0,0));
        int V2 = Mesh.AppendVertex(FVector3d(0,1,0));
        Mesh.AppendTriangle(V0,V1,V2);

        TArray<FVector> Vertices;
        TArray<int32> Indices;
        FCanvasMesh::ExtractVerticesAndIndices(Mesh, Vertices, Indices);

        TestEqual("Vertices count", Vertices.Num(), 3);
        TestEqual("Indices count", Indices.Num(), 3);
    }

    // ---------------------------
    // 8) TriangulateAndBuildMesh
    // ---------------------------
    {
        FInterpCurve<FVector2D> Curve;
        Curve.AddPoint(0, {0,0});
        Curve.AddPoint(1, {1,1});
        Curve.AddPoint(2, {2,0});

        TArray<int32> LastSeamVertexIDs;
        FDynamicMesh3 LastMesh;
        TArray<int32> LastBuiltSeamVertexIDs;
        TArray<TWeakObjectPtr<APatternMesh>> SpawnedActors;

        FCanvasMesh::TriangulateAndBuildMesh(Curve, true, 0, 2, LastSeamVertexIDs, LastMesh, LastBuiltSeamVertexIDs, SpawnedActors);

        TestTrue("TriangulateAndBuildMesh produces vertices", LastMesh.VertexCount() > 0);
    }

    return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanvasMeshBuildAllMeshesTest, "CanvasMesh.BuildAllMeshes",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanvasMeshBuildAllMeshesTest::RunTest(const FString& Parameters)
{
    // 1. Prepare test input: a simple square shape as a curve
    FInterpCurve<FVector2D> CurvePoints;
    CurvePoints.AddPoint(0.0f, FVector2D(0, 0));
    CurvePoints.AddPoint(1.0f, FVector2D(100, 0));
    CurvePoints.AddPoint(2.0f, FVector2D(100, 100));
    CurvePoints.AddPoint(3.0f, FVector2D(0, 100));
    CurvePoints.AddPoint(4.0f, FVector2D(0, 0)); // Close the shape

    TArray<FInterpCurve<FVector2D>> CompletedShapes;
    CompletedShapes.Add(CurvePoints); // Only one shape for this test

    // 2. Prepare output arrays
    TArray<FDynamicMesh3> OutMeshes;
    TArray<TWeakObjectPtr<APatternMesh>> OutSpawnedActors;

    // 3. Call the function under test
    FCanvasMesh::TriangulateAndBuildAllMeshes(CompletedShapes, CurvePoints, OutMeshes, OutSpawnedActors);

    // 4. Validate results
    TestTrue(TEXT("At least one mesh should be created"), OutMeshes.Num() > 0);

    if (OutMeshes.Num() > 0)
    {
        // Each mesh should have vertices
        for (const FDynamicMesh3& Mesh : OutMeshes)
        {
            TestTrue(TEXT("Each mesh should have vertices"), Mesh.VertexCount() > 0);
            TestTrue(TEXT("Each mesh should have triangles"), Mesh.TriangleCount() > 0);
        }
    }

    // 5. Check spawned actors array
    TestTrue(TEXT("Spawned actors array should match number of meshes"), OutSpawnedActors.Num() == OutMeshes.Num());

    return true;
}
