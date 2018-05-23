// Copyright 2018 BruceLee, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

class AActor;
struct FKAggregateGeom;
struct FRawMesh;

enum class EPivotPreset
{
	BoundingBoxCenter,

//Center of each surface
	BoundingBoxCenterTop,
	BoundingBoxCenterBottom,
	BoundingBoxCenterLeft,
	BoundingBoxCenterRight,
	BoundingBoxCenterFront,
	BoundingBoxCenterBack,

//8 conner
	BoundingBoxCornerTop1,
	BoundingBoxCornerTop2,
	BoundingBoxCornerTop3,
	BoundingBoxCornerTop4,
	BoundingBoxCornerBottom1,
	BoundingBoxCornerBottom2,
	BoundingBoxCornerBottom3,
	BoundingBoxCornerBottom4,

//12 Edge
	BoundingBoxEdgeTop1,
	BoundingBoxEdgeTop2,
	BoundingBoxEdgeTop3,
	BoundingBoxEdgeTop4,
	BoundingBoxEdgeMid1,
	BoundingBoxEdgeMid2,
	BoundingBoxEdgeMid3,
	BoundingBoxEdgeMid4,
	BoundingBoxEdgeBottom1,
	BoundingBoxEdgeBottom2,
	BoundingBoxEdgeBottom3,
	BoundingBoxEdgeBottom4,

	MAX,
};

class FPivotToolUtil
{
public:
	static void CopyActorPivotOffset();
	static void PasteActorPivotOffset();

	static void OffsetSelectedActorsPivotOffset(const FVector& NewPivotOffset, bool bWorldSpace = false);
	static void SnapSelectedActorsPivotToVertex();

	static void UpdateSelectedActorsPivotOffset(const FVector& NewPivotOffset, bool bWorldSpace = false);
	static void UpdateSelectedActorsPivotOffset(EPivotPreset InPivotPrest, bool bAutoSave = true, bool bAsGroup = true, bool bVertexMode = false, bool bChildrenMode = false);
	static void GetPreviewPivotsOfSelectedActors(EPivotPreset InPivotPrest, bool bAutoSave, bool bAsGroup, bool bVertexMode, bool bChildrenMode, TArray<FTransform>& OutWorldPivots);

	static bool BakeStaticMeshActorsPivotOffsetToRawMesh(class AStaticMeshActor* InStaticMeshActor, bool bBakeRotation = true, bool bDuplicate = false, bool bSilentMode = false);

	static void NotifyMessage(const FText& Message, float InDuration);

	static bool HasPivotOffsetCopied() { return bPivotOffsetCopied; }

	static bool UpdateActorPivotOffset(AActor* InActor, const FVector& NewPivotOffset, bool bWorldSpace = false);

private:

	static void TransformRawMeshVertexData(const FTransform& InTransform, FRawMesh& OutRawMesh, const FVector& InBuildScale3D);
	static void TransformPhysicsGemotry(const FTransform& InTransform, FKAggregateGeom& AggGeom);

	static FVector GetActorPivot(AActor* InActor, EPivotPreset InPreset, bool bVertexMode, bool bChildrenMode, bool bLocalSpace = true);
	static bool GetActorVertexPostion(AActor* InActor, EPivotPreset InPreset, FVector& OutPosition);
	static FVector GetActorBoundingBoxPoint(AActor* InActor, EPivotPreset InPreset, bool bChildrenMode, bool bLocalSpace = true);
	static FBoxSphereBounds GetSelectionBounds(bool bChildrenMode = false);
	static FVector GetBoundingBoxPoint(const FBox& InBoundingBox, EPivotPreset InPreset);

	static FBox GetAllComponentsBoundingBox(AActor* InActor, bool bRecursive);

	static FVector CopiedPivotOffset;
	static bool bPivotOffsetCopied;
};
