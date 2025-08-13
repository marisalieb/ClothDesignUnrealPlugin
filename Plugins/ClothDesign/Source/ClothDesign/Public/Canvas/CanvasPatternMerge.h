
#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "UDynamicMesh.h"                                 // UDynamicMesh

class APatternMesh;
struct FPatternSewingConstraint;


struct FCanvasPatternMerge
{
public:
    // Constructor takes references so modifications are reflected to caller
    FCanvasPatternMerge(
        TArray<TWeakObjectPtr<APatternMesh>>& InSpawnedActors,
        TArray<FPatternSewingConstraint>& InAllSeams);

    // Top-level call: find sewn groups and merge them where safe.
    void MergeSewnGroups() const;

private:
    // References to caller-owned containers
    TArray<TWeakObjectPtr<APatternMesh>>& SpawnedActorsRef;
    TArray<FPatternSewingConstraint>& AllSeamsRef;

    void BuildActorListAndIndexMap(
        TArray<APatternMesh*>& OutActors,
        TMap<APatternMesh*,
        int32>& OutMap) const;
    
    void BuildAdjacencyFromSeams(
        const TArray<APatternMesh*>& Actors,
        const TMap<APatternMesh*,
        int32>& ActorToIndex,
        TArray<TArray<int32>>& OutAdj) const;
    
    static void FindConnectedComponents(
        const TArray<TArray<int32>>& Adj,
        TArray<TArray<int32>>& OutComponents);
    
    bool ComponentHasExternalEdges(
        const TArray<int32>& Component,
        const TMap<APatternMesh*,int32>& ActorToIndex) const;
    
    static bool MergeComponentToDynamicMesh(
        const TArray<int32>& Component,
        const TArray<APatternMesh*>& Actors,
        UE::Geometry::FDynamicMesh3& OutMerged);

    static APatternMesh* SpawnMergedActorFromDynamicMesh(
        UE::Geometry::FDynamicMesh3&& MergedMesh);

    void ReplaceActorsWithMerged(
        const TArray<int32>& Component,
        const TArray<APatternMesh*>& Actors,
        APatternMesh* MergedActor) const;
    
    void RemoveInternalSeams(
        const TArray<int32>& Component,
        const TMap<APatternMesh*,int32>& ActorToIndex) const;
    
    static USkeletalMesh* CreateSkeletalFromFDynamicMesh(UDynamicMesh* DynMesh, const FString& AssetPathAndName);

};
