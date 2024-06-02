// Copyright (C) Bas Blokzijl - All rights reserved.

/**
 * This class is cleaned of almost all of the project's original code.
 */
#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameUI_Utils/GameUIController.h"
#include "Formation/FormationMovement.h"
#include "Kismet/KismetMathLibrary.h"
#include "ConstructionPreview/CPPConstructionPreview.h"
#include "RTS_Survival/Buildings/BuildingExpansion/BuildingExpansion.h"
#include "RTS_Survival/Units/Enums/Enum_UnitType.h"
#include "RTS_Survival/Player/Abilities.h"
#include "RTS_Survival/Player/PlacementEffects.h"

#include "CPPController.generated.h"

class UPlayerResourceManager;
class ARTSAsyncSpawner;
class ARTSNavigator;
class ACameraPawn;
struct FActionUIParameters;
class ABuildingExpansion;
class IBuildingExpansionOwner;
enum class EBuildingExpansionType : uint8;
class UMainGameUI;
class ANomadicVehicle;
class RTS_SURVIVAL_API ACPP_UnitMaster;
class RTS_SURVIVAL_API ASquadController;
class RTS_SURVIVAL_API ACPPBuildingMaster;
class RTS_SURVIVAL_API ACPPHUD;
class RTS_SURVIVAL_API ACPPGameState;
class RTS_SURVIVAL_API ACPPResourceMaster;
class RTS_SURVIVAL_API ASelectablePawnMaster;
class RTS_SURVIVAL_API ASelectableActorObjectsMaster;

// ...

UCLASS()
class RTS_SURVIVAL_API ACPPController : public APlayerController
{
	GENERATED_BODY()

	friend class RTS_SURVIVAL_API AActionUIController;

	//...

public:
	//...

	/**
	 * Cancels the building construction on the requesting actor if it is of the NomadicVehicle Type.
	 * @param RequestingActor The actor that requested the building to be cancelled.
	 */
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category = "MainGameUI")
	void CancelBuilding(AActor *RequestingActor);

	/**
	 * Constructs the building on the requesting actor if it is of the NomadicVehicle Type.
	 * @param RequestingActor The actor that requested the building to be constructed.
	 */
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category = "MainGameUI")
	void ConstructBuilding(AActor *RequestingActor);

	/**
	 * @brief Instructs the actor to convert to vehicle if it is a nomadic vehicle.
	 * @param RequestingActor The vehicle that requested the conversion.
	 */
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category = "MainGameUI")
	void ConvertBackToVehicle(AActor *RequestingActor);

	/**
	 * @brief Instructs the actor to stop the conversion to vehicle.
	 * @param RequestingActor the vehicle that wants to stop converting.
	 */
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category = "MainGameUI")
	void CancelVehicleConversion(AActor *RequestingActor);

	/**
	 * @brief Called when a truck finished converting to either a building or a vehicle.
	 * @param ConvertedTruck the truck that was converted.
	 * @param bConvertedToBuilding Whether the truck was converted to a building or not.
	 * @post the main game UI is updated accordingly.
	 */
	void TruckConverted(
		ANomadicVehicle *ConvertedTruck,
		const bool bConvertedToBuilding) const;

	/**
	 * @brief adds the specified expansion type at the index provided to the array of expansions saved in the
	 * expansion component of the expansion owner.
	 * @param BuildingExpansionType The type of expansion to add.
	 * @param BuildingExpansionOwner The owner of the expansion.
	 * @param ExpansionSlotIndex The index in the array of expansions to add the expansion to.
	 * @param bIsUnpackedExpansion Whether the expansion is unpacked or not.
	 * @note Is called from MainGameUI after the user clicks on a building expansion widget.
	 */
	void ExpandBuildingWithType(
		EBuildingExpansionType BuildingExpansionType,
		IBuildingExpansionOwner *BuildingExpansionOwner,
		const int ExpansionSlotIndex,
		const bool bIsUnpackedExpansion);

	/** @brief function called by the Async spawner when the building expansion is spawned.
	 * @param bSuccessfulExpansionSpawn: Whether the expansion was spawned successfully.
	 * @param SpawnedBxp: The spawned building expansion.
	 * @param BxpOwner: The owner of the building expansion.
	 * @param BuildingExpansionType: The type of building expansion.
	 * @param ExpansionSlotIndex: The index in the array of expansions to add the expansion to.
	 * @param bIsUnpackedExpansion: Whether the expansion is unpacked or build for the first time.
	 */
	void OnBxpSpawnedAsync(
		const bool bSuccessfulExpansionSpawn,
		ABuildingExpansion *SpawnedBxp,
		IBuildingExpansionOwner *BxpOwner,
		const EBuildingExpansionType BuildingExpansionType,
		const int ExpansionSlotIndex,
		const bool bIsUnpackedExpansion);

	//...

	/**
	 * Stop the building expansion placement and stop building mode.
	 * @param BxpOwner If the bxp was already spawned we destroy it on the owner.
	 * else we cancel the request to spawn the bxp on the RTSAsyncSpawner.
	 * @param bIsCancelledPackedExpansion Whether the building expansion is packed up or not.
	 * If it is, we destroy the expansion but make sure to save the type of expansion as well as set the status to packed up.
	 * @note Is public so it can be called by Main Game UI when the cancel button is pressed.
	 * @post The building mode is stopped and the preview on the ConstructionPreview is destroyed.
	 */
	void StopBxpPreviewPlacement(IBuildingExpansionOwner *BxpOwner, const bool bIsCancelledPackedExpansion);

	/**
	 * @brief Stops the building expansion's construction animation and destroys it.
	 * @param BxpOwner The owner of the building expansion, which will destroy it.
	 * @param BuildingExpansion The building expansion to destroy.
	 * @param bIsCancelledPackedBxp Whether the building expansion is packed up or not.
	 * @note if the bxp is packed we set the status back to packed and make sure to save the type of expansion.
	 */
	void CancelBuildingExpansionConstruction(IBuildingExpansionOwner *BxpOwner,
											 ABuildingExpansion *BuildingExpansion,
											 const bool bIsCancelledPackedBxp) const;

private:
	//...

	// The vehicle selected for building upon first clicking the building action button.
	UPROPERTY()
	ANomadicVehicle *m_NomadicVehicleSelectedForBuilding;

	// Whether there is currently an active building preview.
	// This is only the case if we activated a building ability but have not yet propagated a valid building
	// command with location to the construction vehicle.
	UPROPERTY()
	EBuildingPreviewMode m_IsBuildingPreviewModeActive;

	inline bool GetIsPreviewModeActive() const
	{
		return m_IsBuildingPreviewModeActive != EBuildingPreviewMode::BuildingPreviewModeOFF;
	}

	/**
	 * Tries to place the building at the clicked location.
	 * @param ClickedLocation The location to place the building at.
	 * @return True if the building was placed, false otherwise.
	 */
	bool TryPlaceBuilding(const FVector &ClickedLocation);

	/**
	 * Attempts to place the bpx if it was loaded asynchroneously.
	 * @param ClickedLocation Where the player clicked to place the building.
	 * @return True if succesfully placed the building, false otherwise.
	 * @post If true, the M_AsyncBxpRequestState is reset.
	 * @post If false, the async state is not changed.
	 */
	bool TryPlaceBxp(const FVector &ClickedLocation);

	/**
	 * @brief Finds the first available nomadic truck in selected units to convert to building at the given location.
	 * @param BuildingLocation The location to build the building at.
	 * @return Whether we found a truck for the conversion
	 * @pre Assumes that the placement location is valid.
	 * @post Either there was a valid location and an available truck was found which now has the building ability queued.
	 * Or no truck was available. In Both cases we need to cancel the preview and deactivate building preview mode
	 * as the command is propagated to a truck or cannot be executed.
	 */
	bool NomadicConvertToBuilding(const FVector &BuildingLocation);

	//-------------------------------------- BUILDING EXPANSION RELATED --------------------------------------//

	/**
	 * Places the provided building expansion at the given location.
	 * @param BuildingLocation Where to place the building expansion.
	 * @param BuildingExpansion The building expansion to place.
	 * @param BuildingRotation The rotation of the building expansion.
	 */
	void PlaceExpansionBuilding(
		const FVector &BuildingLocation,
		ABuildingExpansion *BuildingExpansion,
		const FRotator BuildingRotation) const;

	/**
	 * @brief Destroys the building expansion and stops preview mode.
	 * Goes through the switch of buidling modes and terminates the active one.
	 * @post M_AsyncBxpRequestState is Reset.
	 * @post Preview on the ConstructionPreview is destroyed.
	 * @post Building Mode is off.
	 */
	void StopBuildingPreviewMode();

	/**
	 * @brief Stop the preview mode of the nomadic building.
	 * @post m_NomadicVehicleSelectedForBuilding is reset.
	 * @post The preview on the ConstructionPreview is destroyed.
	 * @post The building mode is off.
	 */
	void StopNomadicPreviewPlacement();

	/** @brief Also destroys the preview but without calling a cancel on the bxp placement.
	 * @note used in case we found a valid location to place our preview. */
	void StopPreviewAndBuildingMode();

	// Keeps track of the asynchronous building expansion request.
	FAsyncBxpRequestState M_AsyncBxpRequestState;

	//...
};
