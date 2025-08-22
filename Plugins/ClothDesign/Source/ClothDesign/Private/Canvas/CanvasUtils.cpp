#include "Canvas/CanvasUtils.h"

void FCanvasUtils::SaveStateForUndo(
	TArray<FCanvasState>& UndoStack,
	TArray<FCanvasState>& RedoStack,
	const FCanvasState& CurrentState)
{
	UndoStack.Add(CurrentState);
	RedoStack.Empty();
}


bool FCanvasUtils::Undo(
	TArray<FCanvasState>& UndoStack,
	TArray<FCanvasState>& RedoStack,
	FCanvasState& CurrentState)
{
	if (UndoStack.Num() > 0)
	{
		RedoStack.Add(CurrentState);
		CurrentState = UndoStack.Pop();
		return true;
	}
	return false;
}

bool FCanvasUtils::Redo(
	TArray<FCanvasState>& UndoStack,
	TArray<FCanvasState>& RedoStack,
	FCanvasState& CurrentState)
{
	if (RedoStack.Num() > 0)
	{
		UndoStack.Add(CurrentState);
		CurrentState = RedoStack.Pop();
		return true;
	}
	return false;
}


void FCanvasUtils::RecalculateNTangents(
	FInterpCurve<FVector2D>& Curve,
	const TArray<bool>&      bBezierFlags)
{
	int32 Num = Curve.Points.Num();
	if (Num < 2) return;

	for (int32 i = 0; i < Num; ++i)
	{
		// Only operate on N‑points/ linear points
		if (bBezierFlags[i]) continue;

		// Prev
		if (i > 0)
		{
			FVector2D Prev = Curve.Points[i-1].OutVal;
			FVector2D Curr = Curve.Points[i  ].OutVal;
			Curve.Points[i].ArriveTangent = (Curr - Prev) * 0.5f;
		}
		else
		{
			Curve.Points[i].ArriveTangent = FVector2D::ZeroVector;
		}

		// Next
		if (i < Num - 1)
		{
			FVector2D Curr = Curve.Points[i  ].OutVal;
			FVector2D Next = Curve.Points[i+1].OutVal;
			Curve.Points[i].LeaveTangent = (Next - Curr) * 0.5f;
		}
		else
		{
			Curve.Points[i].LeaveTangent = FVector2D::ZeroVector;
		}
	}
}


FVector3d FCanvasUtils::ComputeAreaWeightedCentroid(const UE::Geometry::FDynamicMesh3& Mesh)
{
	double totalArea = 0.0;
	FVector3d accum(0,0,0);

	for (int tid : Mesh.TriangleIndicesItr())
	{
		UE::Geometry::FIndex3i T = Mesh.GetTriangle(tid);
		FVector3d A = Mesh.GetVertex(T.A);
		FVector3d B = Mesh.GetVertex(T.B);
		FVector3d C = Mesh.GetVertex(T.C);

		FVector3d e1 = B - A;
		FVector3d e2 = C - A;
		double triArea = 0.5 * FVector3d::CrossProduct(e1, e2).Length();

		if (triArea > KINDA_SMALL_NUMBER)
		{
			FVector3d triCentroid = (A + B + C) / 3.0;
			accum += triCentroid * triArea;
			totalArea += triArea;
		}
	}

	if (totalArea > KINDA_SMALL_NUMBER) return accum / totalArea;

	// Fallback: simple average of vertices
	int nv = 0;
	FVector3d sum(0,0,0);
	for (int vid : Mesh.VertexIndicesItr())
	{
		sum += Mesh.GetVertex(vid);
		++nv;
	}
	if (nv > 0) return sum / static_cast<double>(nv);
	return FVector3d::Zero();
}

// Subtract pivotFrom (centroid) from all vertices to move pivot to origin.
void FCanvasUtils::CenterMeshVerticesToOrigin(TArray<FVector>& Vertices, const FVector& PivotFrom)
{
    for (FVector& V : Vertices)
    {
        V -= PivotFrom;
    }
}

void FCanvasUtils::TranslateDynamicMeshBy(UE::Geometry::FDynamicMesh3& Mesh, const FVector3d& Offset)
{
	for (int vid : Mesh.VertexIndicesItr())
	{
		FVector3d p = Mesh.GetVertex(vid);
		p -= Offset; // subtract offset so mesh becomes local around origin
		Mesh.SetVertex(vid, p);
	}
}


