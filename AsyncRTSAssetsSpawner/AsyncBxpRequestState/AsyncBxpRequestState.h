#pragma once

#include "CoreMinimal.h"

#include "AsyncBxpRequestState.generated.h"

class IBuildingExpansionOwner;
class ABuildingExpansion;

enum class EAsyncBxpStatus
{
	Async_NoRequest,
	// The async manager is loading a bxp.
	Async_SpawnedPreview_WaitForBuildingMesh,
	// The asset is loaded and the bxp is spawned.
	Async_BxpIsSpawned
};

static FString AsyncBxpStatusToString(EAsyncBxpStatus Status)
{
	switch (Status)
	{
	case EAsyncBxpStatus::Async_NoRequest:
		return "Async_NoRequest";
	case EAsyncBxpStatus::Async_SpawnedPreview_WaitForBuildingMesh:
		return "Async_SpawnedPreview_WaitForConstructBxp";
	case EAsyncBxpStatus::Async_BxpIsSpawned:
		return "Async_LoadingComplete";
	default:
		return "Unknown";
	}
}

USTRUCT()
struct FAsyncBxpRequestState
{
	GENERATED_BODY()

	// The building expansion that is spawned asynchronously, can be null.
	// Non-owning pointer saveguarded with GC.
	UPROPERTY()
	TObjectPtr<ABuildingExpansion> SpawnedBuildingExpansion;

	EAsyncBxpStatus Status = EAsyncBxpStatus::Async_NoRequest;

	// Slot index in the array of bxps with widgets the bxp owner has.
	int ExpansionSlotIndex = INDEX_NONE;

	IBuildingExpansionOwner* BuildingExpansionOwner = nullptr;

	bool bIsPackedExpansion = false;

	void Reset()
	{
		SpawnedBuildingExpansion = nullptr;
		Status = EAsyncBxpStatus::Async_NoRequest;
		ExpansionSlotIndex = INDEX_NONE;
		BuildingExpansionOwner = nullptr;
		bIsPackedExpansion = false;
	}

	void InitSuccessfulRequest(
		const EAsyncBxpStatus InitStatus,
		const int InitExpansionIndex,
		IBuildingExpansionOwner* Owner,
		const bool bInitIsPackedExpansion)
	{
		Status = InitStatus;
		ExpansionSlotIndex = InitExpansionIndex;
		BuildingExpansionOwner = Owner;
		bIsPackedExpansion = bInitIsPackedExpansion;
	}
};
