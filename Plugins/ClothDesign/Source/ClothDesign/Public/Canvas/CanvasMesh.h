#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Math/InterpCurve.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "PatternSewingConstraint.h"
#include "ClothPatternMeshActor.h"
#include "Misc/ScopeLock.h"
#include "ClothShapeAsset.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "MeshOpPreviewHelpers.h" 
#include "SClothDesignCanvas.h"
#include "ClothPatternMeshActor.h"
#include "CompGeom/Delaunay2.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshEditor.h"
#include "CoreMinimal.h"
#include "Math/MathFwd.h"
#include "UObject/Class.h"
#include "Rendering/DrawElements.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Engine/SkeletalMesh.h"
#include "ClothingSimulationInteractor.h"
#include "ClothingSimulationFactory.h"
#include "ChaosCloth/ChaosClothingSimulationInteractor.h"
#include "ClothingSimulationFactory.h"
#include "Widgets/SCompoundWidget.h"
#include "ProceduralMeshComponent.h"
#include "Math/InterpCurve.h"
#include "PatternSewingConstraint.h"
#include "Curve/DynamicGraph.h"
#include "ConstrainedDelaunay2.h"
#include "Misc/ScopeLock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "ClothShapeAsset.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "ClothPatternMeshActor.h"


namespace CanvasMesh
{
	inline TArray<AClothPatternMeshActor*> SpawnedPatternActors;

	
	static bool IsPointInPolygon(
	const FVector2f& Test, 
	const TArray<FVector2f>& Poly);

	void CreateProceduralMesh(
	const TArray<FVector>& Vertices,
	const TArray<int32>& Indices,
	FDynamicMesh3&& DynamicMesh,
	TArray<int32>&& SeamVertexIDs,
	TArray<AClothPatternMeshActor*>& OutSpawnedActors);
	
	
	void TriangulateAndBuildMesh(
		const FInterpCurve<FVector2D>& Shape,
		bool bRecordSeam,
		int32 StartPointIdx2D,
		int32 EndPointIdx2D,
		/* out */ TArray<int32>& LastSeamVertexIDs,
		/* out */ FDynamicMesh3& LastBuiltMesh,
		/* optional */ TArray<int32> LastBuiltSeamVertexIDs);
		
	void TriangulateAndBuildAllMeshes(
		const TArray<FInterpCurve<FVector2D>>& CompletedShapes,
		const FInterpCurve<FVector2D>& CurvePoints,
		TArray<FDynamicMesh3>& OutMeshes);
}




