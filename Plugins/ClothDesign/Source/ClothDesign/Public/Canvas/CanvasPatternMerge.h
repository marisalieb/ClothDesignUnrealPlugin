
#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "DynamicMesh/DynamicMesh3.h"

class APatternMesh;
struct FPatternSewingConstraint;

/**
 * Small utility that merges sewn pattern-mesh actors (groups connected by seams).
 * Construct with references to the arrays owned by your sewing manager so the
 * merge operation can update them in-place.
 */
struct FCanvasPatternMerge
{
public:
    // Constructor takes references so modifications are reflected to caller
    FCanvasPatternMerge(
        TArray<TWeakObjectPtr<APatternMesh>>& InSpawnedActors,
        TArray<FPatternSewingConstraint>& InAllSeams);

    // Top-level call: find sewn groups and merge them where safe.
    void MergeSewnGroups();

private:
    // References to caller-owned containers
    TArray<TWeakObjectPtr<APatternMesh>>& SpawnedActorsRef;
    TArray<FPatternSewingConstraint>& AllSeamsRef;

    // Internal helpers (small, focused)
    void BuildActorListAndIndexMap(TArray<APatternMesh*>& OutActors, TMap<APatternMesh*, int32>& OutMap) const;
    void BuildAdjacencyFromSeams(const TArray<APatternMesh*>& Actors, const TMap<APatternMesh*,int32>& ActorToIndex, TArray<TArray<int32>>& OutAdj) const;
    void FindConnectedComponents(const TArray<TArray<int32>>& Adj, TArray<TArray<int32>>& OutComponents) const;
    bool ComponentHasExternalEdges(const TArray<int32>& Component, const TArray<APatternMesh*>& Actors, const TMap<APatternMesh*,int32>& ActorToIndex) const;
    bool MergeComponentToDynamicMesh(const TArray<int32>& Component, const TArray<APatternMesh*>& Actors, UE::Geometry::FDynamicMesh3& OutMerged) const;
    APatternMesh* SpawnMergedActorFromDynamicMesh(UE::Geometry::FDynamicMesh3&& MergedMesh) const;
    void ReplaceActorsWithMerged(const TArray<int32>& Component, const TArray<APatternMesh*>& Actors, APatternMesh* MergedActor);
    void RemoveInternalSeams(const TArray<int32>& Component, const TArray<APatternMesh*>& Actors, const TMap<APatternMesh*,int32>& ActorToIndex);
};
