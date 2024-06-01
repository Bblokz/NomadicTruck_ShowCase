// Copyright Bas Blokzijl - All rights reserved.

#include "RTSAsyncSpawner.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "RTS_Survival/Buildings/BuildingExpansion/Interface/BuildingExpansionOwner.h"
#include "RTS_Survival/Player/CPPController.h"
#include "RTS_Survival/Utils/HFunctionLibary.h"


ARTSAsyncSpawner::ARTSAsyncSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARTSAsyncSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void ARTSAsyncSpawner::InitRTSAsyncSpawner(ACPPController* PlayerController)
{
	if (PlayerController)
	{
		M_PlayerController = PlayerController;
	}
	else
	{
		RTSFunctionLibrary::ReportError(
			"PlayerController is nullptr in InitRTSAsyncSpawner. \n At function InitRTSAsyncSpawner in RTSAsyncSpawner.cpp"
			"Class name: ARTSAsyncSpawner. \n failed to set PlayerController reference on init?.");
	}
}

UStaticMesh* ARTSAsyncSpawner::SyncGetBuildingExpansionPreviewMesh(EBuildingExpansionType BuildingExpansionType)
{
	if (BxpPreviewMeshMap.Contains(BuildingExpansionType))
	{
		return BxpPreviewMeshMap[BuildingExpansionType];
	}
	else
	{
		RTSFunctionLibrary::ReportError(
			"Building expansion type not found in the map! \n At function SyncGetBuildingExpansionPreviewMesh in RTSAsyncSpawner.cpp"
			"number of BuildingExpansionType: " + FString::FromInt((int32)BuildingExpansionType));
		return nullptr;
	}
}

void ARTSAsyncSpawner::AsyncSpawnBuildingExpansion(
	EBuildingExpansionType BuildingExpansionType,
	IBuildingExpansionOwner* BuildingExpansionOwner,
	const int ExpansionSlotIndex,
	const bool bIsUnpackedExpansion)
{
	if (BuildingExpansionType == EBuildingExpansionType::BXT_Invalid)
	{
		RTSFunctionLibrary::ReportError(
			"Attempt to spawn invalid building expansion type! \n At function AsyncSpawnBuildingExpansion in RTSAsyncSpawner.cpp"
			"\n the function will return without spawning the expansion.");
		return;
	}
	// Check if the map contains the specified building expansion type
	if (BuildingExpansionMap.Contains(BuildingExpansionType))
	{
		// Get the soft class reference for the specified building expansion type
		const TSoftClassPtr<ABuildingExpansion> AssetClass = BuildingExpansionMap[BuildingExpansionType];

		// If the asset is already loaded, handle it immediately (possible if recently used)
		if (AssetClass.IsValid())
		{
			HandleAsyncBxpLoadComplete(AssetClass.ToSoftObjectPath(),
			                           BuildingExpansionType,
			                           BuildingExpansionOwner,
			                           ExpansionSlotIndex,
			                           bIsUnpackedExpansion);
		}
		else
		{
			// If the asset is not loaded, request asynchronous loading
			StreamableManager.RequestAsyncLoad(
				AssetClass.ToSoftObjectPath(),
				FStreamableDelegate::CreateUObject(
					this,
					&ARTSAsyncSpawner::HandleAsyncBxpLoadComplete,
					AssetClass.ToSoftObjectPath(),
					BuildingExpansionType,
					BuildingExpansionOwner,
					ExpansionSlotIndex,
					bIsUnpackedExpansion
				)
			);
		}
	}
	else
	{
		RTSFunctionLibrary::ReportError(
			"Building expansion type not found in the map! \n At function AsyncSpawnBuildingExpansion in RTSAsyncSpawner.cpp"
			"number of BuildingExpansionType: " + FString::FromInt((int32)BuildingExpansionType));
	}
}

void ARTSAsyncSpawner::HandleAsyncBxpLoadComplete(
	FSoftObjectPath AssetPath,
	const EBuildingExpansionType BuildingExpansionType,
	IBuildingExpansionOwner* BuildingExpansionOwner,
	const int ExpansionSlotIndex,
	const bool bIsUnpackedExpansion)
{
	// Resolve the loaded asset
	UObject* LoadedAsset = AssetPath.ResolveObject();
	if (LoadedAsset)
	{
		// Cast the loaded asset to a UClass to obtain actor class
		UClass* AssetClass = Cast<UClass>(LoadedAsset);
		if (AssetClass)
		{
			// Spawn the building expansion actor at the current location of this spawner
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(AssetClass, GetActorLocation(), FRotator::ZeroRotator,
			                                                      SpawnParams);

			// If the actor was spawned successfully, call the blueprint-implementable event
			if (SpawnedActor)
			{
				OnBuildingExpansionSpawned(SpawnedActor, BuildingExpansionOwner, BuildingExpansionType,
				                           ExpansionSlotIndex, bIsUnpackedExpansion);
			}
			else
			{
				RTSFunctionLibrary::ReportError(
					"Failed to spawn building expansion of type " + FString::FromInt((int32)BuildingExpansionType) +
					". \n At function HandleAsyncBxpLoadComplete in RTSAsyncSpawner.cpp"
					"Class name: ARTSAsyncSpawner. \n result: No callback to playercontroller is made.");
			}
		}
	}
}

void ARTSAsyncSpawner::OnBuildingExpansionSpawned(
	AActor* SpawnedActor,
	IBuildingExpansionOwner* BuildingExpansionOwner,
	const EBuildingExpansionType BuildingExpansionType,
	const int ExpansionSlotIndex,
	const bool bIsUnpackedExpansion
)
{
	if (M_PlayerController)
	{
		if (ABuildingExpansion* BuildingExpansion = Cast<ABuildingExpansion>(SpawnedActor))
		{
			M_PlayerController->OnBxpSpawnedAsync(BuildingExpansion, BuildingExpansionOwner, BuildingExpansionType,
			                                      ExpansionSlotIndex, bIsUnpackedExpansion);
		}
		else
		{
			RTSFunctionLibrary::ReportError(
				"Builing expansion cast failed! \n At function OnBuildingExpansionSpawned in RTSAsyncSpawner.cpp"
				"Class name: ARTSAsyncSpawner. \n Check the BuildingExpansion reference."
				"\n name of spawned actor: " + SpawnedActor->GetName());
		}
	}
}
