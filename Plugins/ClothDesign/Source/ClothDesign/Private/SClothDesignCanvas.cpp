#include "SClothDesignCanvas.h"
#include "AClothPatternMeshActor.h"


#include "CompGeom/PolygonTriangulation.h"



// #include "SClothShapeCanvas.h"
#include "Rendering/DrawElements.h"

// using namespace UE::Geometry;


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


void SClothDesignCanvas::TriangulateAndBuildMesh()
{
	if (Points.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Need at least 3 points to triangulate"));
		return;
	}

	// Step 1: Convert FVector2D 
	TArray<PolygonTriangulation::TVector2<float>> PolygonVerts;
	
	// Convert from your Points (FVector2D) to PolygonVerts
	for (const FVector2D& P : Points)
	{
		PolygonVerts.Add(PolygonTriangulation::TVector2<float>(P.X, P.Y));
	}

	// Step 2: Triangulate using GeometryProcessing
	TArray<UE::Geometry::FIndex3i> Triangles;


	PolygonTriangulation::TriangulateSimplePolygon<float>(PolygonVerts, Triangles, false);

	if (Triangles.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Triangulation failed"));
		return;
	}
	// Step 3: Convert results to Unreal-friendly format
	TArray<FVector> Vertices;
	TArray<int32> Indices;

	for (const PolygonTriangulation::TVector2<float>& V : PolygonVerts)
	{
		Vertices.Add(FVector(V.X, V.Y, 0.f));  // Z=0 since it’s flat
	}

	
	for (const UE::Geometry::FIndex3i& Tri : Triangles)
	{
		Indices.Add(Tri.C);
		Indices.Add(Tri.B);
		Indices.Add(Tri.A);
	}

	// Step 4: Spawn procedural mesh (next step)
	CreateProceduralMesh(Vertices, Indices);
}



void SClothDesignCanvas::CreateProceduralMesh(const TArray<FVector>& Vertices, const TArray<int32>& Indices)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	AClothPatternMeshActor* MeshActor = World->SpawnActor<AClothPatternMeshActor>();
	if (!MeshActor) return;

	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Normals.Add(FVector::UpVector);
		UV0.Add(FVector2D(Vertices[i].X * 0.01f, Vertices[i].Y * 0.01f));
		VertexColors.Add(FLinearColor::White);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
	}

	MeshActor->MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UV0, VertexColors, Tangents, true);
}

