// Copyright Bas Blokzijl - All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "RTS_Survival/MasterObjects/ActorObjectsMaster.h"
#include "RTSAsyncSpawner.generated.h"

class IBuildingExpansionOwner;
class ACPPController;
enum class EBuildingExpansionType : uint8;
class ABuildingExpansion;

UCLASS()
class RTS_SURVIVAL_API ARTSAsyncSpawner : public AActorObjectsMaster
{
	GENERATED_BODY()

public:
	ARTSAsyncSpawner();

	/**
	 * @brief Asynchronously spawns a building expansion of the specified type.
	 * @param BuildingExpansionType The type of building expansion to spawn.
	 * @param BuildingExpansionOwner The owner of the building expansion.
	 * @param ExpansionSlotIndex The index of the expansion slot to spawn the expansion in.
	 * @param bIsUnpackedExpansion Whether the expansion is an unpacked expansion or not.
	 * @pre The BuildingExpansionType is set to the correct mapping in the BuildingExpansionMap.
	 */
	void AsyncSpawnBuildingExpansion(
		EBuildingExpansionType BuildingExpansionType,
		IBuildingExpansionOwner* BuildingExpansionOwner,
		const int ExpansionSlotIndex,
		const bool bIsUnpackedExpansion);

	UFUNCTION(BlueprintCallable, Category = "ReferenceCasts")
	void InitRTSAsyncSpawner(ACPPController* PlayerController);

	/**
	 * Syncronously gets the preview mesh of the building expansion type.
	 * @param BuildingExpansionType The type of building expansion to get the preview mesh of.
	 * @return The static mesh pointer of the preview mesh.
	 * @pre The ExpansionType mapping is set to the correct preview mesh.
	 */
	UStaticMesh* SyncGetBuildingExpansionPreviewMesh(EBuildingExpansionType BuildingExpansionType);


protected:
	virtual void BeginPlay() override;
	
	// Associates the building expansion type with the class to spawn using a hashmap.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Async Spawning")
	TMap<EBuildingExpansionType, TSoftClassPtr<ABuildingExpansion>> BuildingExpansionMap;

	// Associates the building expansion type with the associated preview mesh using a hashmap.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Async Spawning")
	TMap<EBuildingExpansionType, UStaticMesh*> BxpPreviewMeshMap;

private:
	// Used to load assets asynchronously.
	FStreamableManager StreamableManager;

	// Safe refeence using GC system.
	UPROPERTY()
	ACPPController* M_PlayerController;

	/**
	 * @brief Handles the loaded hard reference to a bxp.
	 * will attempt to spawn the bxp and propagate to the player controller using OnBuildingExpansionSpawned.
	 * @param AssetPath Path to the asset to load.
	 * @param BuildingExpansionType The type of building expansion to spawn.
	 * @param BuildingExpansionOwner The owner of the building expansion.
	 * @param ExpansionSlotIndex The index of the expansion slot to spawn the expansion in.
	 * @param bIsUnpackedExpansion The expansion is an unpacked expansion or not.
	 * @note Makes no callback when the assset fails to spawn.
	 */
	void HandleAsyncBxpLoadComplete(
		FSoftObjectPath AssetPath,
		const EBuildingExpansionType BuildingExpansionType,
		IBuildingExpansionOwner* BuildingExpansionOwner,
		const int ExpansionSlotIndex,
	const bool bIsUnpackedExpansion);

	/** @brief Notifies the playercontroller that the building expansion was spawned. */
	void OnBuildingExpansionSpawned(
		AActor* SpawnedActor,
		IBuildingExpansionOwner* BuildingExpansionOwner,
		const EBuildingExpansionType BuildingExpansionType,
		const int ExpansionSlotIndex,
		const bool bIsUnpackedExpansion);
};
