
#ifndef FCanvasPatternMerge_H
#define FCanvasPatternMerge_H

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "UDynamicMesh.h"

/*
 * Thesis reference:
 * See Chapter 4.8.1 for detailed explanations.
 */

// Forward declarations of classes used by FCanvasPatternMerge
class APatternMesh; /**< Represents a single pattern mesh in the canvas, used for merging and sewing operations. */
struct FPatternSewingConstraint; /**< Represents a sewing constraint between pattern edges, used to determine adjacency. */

/**
 * @brief Handles merging of sewn pattern mesh groups in the canvas.
 * 
 * FCanvasPatternMerge analyses the sewing constraints between pattern meshes
 * and merges groups of meshes that are safely connected, ensuring that
 * operations on patterns remain consistent and coherent. Merging is done
 * where no external connections would be broken, preserving the integrity
 * of the cloth design.
 */
class FCanvasPatternMerge
{
public:

    /**
     * @brief Constructs a merge manager with references to caller-owned containers.
     * 
     * The constructor takes references to the arrays of spawned actors and seams
     * so that any modifications (merges, replacements, seam removals) directly
     * update the caller's data.
     * 
     * @param InSpawnedActors Reference to the canvas's spawned pattern meshes.
     * @param InAllSeams Reference to all sewing constraints currently in the canvas.
     */
    FCanvasPatternMerge(
        TArray<TWeakObjectPtr<APatternMesh>>& InSpawnedActors,
        TArray<FPatternSewingConstraint>& InAllSeams);

    /**
     * @brief Top-level function to merge sewn groups of pattern meshes.
     * 
     * Identifies connected groups of sewn pattern meshes and merges them where safe.
     * The merge preserves external edges and ensures the canvas remains consistent.
     */
    void MergeSewnGroups() const;

    /**
     * @brief Test-only constructor that binds to static test arrays.
     * 
     * Allows unit tests to operate on shared static arrays instead of
     * requiring a full canvas setup.
     */
    FCanvasPatternMerge()
        : SpawnedActorsRef(TestActors), AllSeamsRef(TestSeams) {}

    /** @brief Static array used for test-only actor references. */
    static TArray<TWeakObjectPtr<APatternMesh>> TestActors;

    /** @brief Static array used for test-only seam constraints. */
    static TArray<FPatternSewingConstraint> TestSeams;

private:

    /** @brief References to caller-owned pattern meshes for direct modification. */
    TArray<TWeakObjectPtr<APatternMesh>>& SpawnedActorsRef;

    /** @brief References to caller-owned seams for direct modification. */
    TArray<FPatternSewingConstraint>& AllSeamsRef;

    /**
     * @brief Builds a flat actor list and maps each actor to an index.
     * 
     * This mapping simplifies adjacency and connected component calculations.
     */
    void BuildActorListAndIndexMap(
        TArray<APatternMesh*>& OutActors,
        TMap<APatternMesh*, int32>& OutMap) const;

    /**
     * @brief Builds adjacency information based on sewing constraints.
     * 
     * Determines which pattern meshes are directly connected, to guide
     * safe merging of connected components.
     */
    void BuildAdjacencyFromSeams(
        const TArray<APatternMesh*>& Actors,
        const TMap<APatternMesh*, int32>& ActorToIndex,
        TArray<TArray<int32>>& OutAdj) const;

    /**
     * @brief Identifies connected components from adjacency information.
     * 
     * Each component represents a group of pattern meshes that are
     * candidates for merging without breaking external connections.
     */
    static void FindConnectedComponents(
        const TArray<TArray<int32>>& Adj,
        TArray<TArray<int32>>& OutComponents);

    /**
     * @brief Checks if a component has edges connecting externally.
     * 
     * Prevents merging of components that would break existing constraints
     * with meshes outside the component.
     */
    bool ComponentHasExternalEdges(
        const TArray<int32>& Component,
        const TMap<APatternMesh*, int32>& ActorToIndex) const;

    /**
     * @brief Merges a connected component of pattern meshes into a single dynamic mesh.
     * 
     * Converts the component's meshes into a single FDynamicMesh3 for further processing.
     */
    static bool MergeComponentToDynamicMesh(
        const TArray<int32>& Component,
        const TArray<APatternMesh*>& Actors,
        FDynamicMesh3& OutMerged);

    /**
     * @brief Spawns a new APatternMesh actor from a merged dynamic mesh.
     * 
     * Provides a procedural mesh actor to replace the individual meshes in the component.
     */
    static APatternMesh* SpawnMergedActorFromDynamicMesh(
        FDynamicMesh3&& MergedMesh);

    /**
     * @brief Replaces original actors in a component with the merged actor.
     * 
     * Ensures that the canvas now references only the new merged mesh and
     * removes redundant actors.
     */
    void ReplaceActorsWithMerged(
        const TArray<int32>& Component,
        const TArray<APatternMesh*>& Actors,
        APatternMesh* MergedActor) const;

    /**
     * @brief Removes seams that are internal to a merged component.
     * 
     * Prevents duplicate or conflicting seams from existing after merging.
     */
    void RemoveInternalSeams(
        const TArray<int32>& Component,
        const TMap<APatternMesh*, int32>& ActorToIndex) const;

    /**
     * @brief Converts a FDynamicMesh to a USkeletalMesh asset.
     * 
     * Generates a reusable skeletal mesh from the procedural dynamic mesh.
     * 
     * @param DynMesh Input dynamic mesh to convert.
     * @param AssetPathAndName Path and name for the resulting asset.
     */
    static USkeletalMesh* CreateSkeletalFromFDynamicMesh(
        UDynamicMesh* DynMesh,
        const FString& AssetPathAndName);

    /** @brief Grants test class access to private members. */
    friend class FCanvasPatternMergeTests;
};


#endif
