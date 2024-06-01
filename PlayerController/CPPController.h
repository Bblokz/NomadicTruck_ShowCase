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
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category="MainGameUI")
	void CancelBuilding(AActor* RequestingActor);

	/**
	 * Constructs the building on the requesting actor if it is of the NomadicVehicle Type.
	 * @param RequestingActor The actor that requested the building to be constructed.
	 */
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category="MainGameUI")
	void ConstructBuilding(AActor* RequestingActor);

	/**
	 * @brief Instructs the actor to convert to vehicle if it is a nomadic vehicle.
	 * @param RequestingActor The vehicle that requested the conversion.
	 */
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category="MainGameUI")
	void ConvertBackToVehicle(AActor* RequestingActor);

	/**
	 * @brief Instructs the actor to stop the conversion to vehicle.
	 * @param RequestingActor the vehicle that wants to stop converting.
	 */
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category="MainGameUI")
	void CancelVehicleConversion(AActor* RequestingActor);

	/**
	 * @brief Called when a truck finished converting to either a building or a vehicle.
	 * @param ConvertedTruck the truck that was converted.
	 * @param bConvertedToBuilding Whether the truck was converted to a building or not.
	 * @post the main game UI is updated accordingly.
	 */
	void TruckConverted(
		ANomadicVehicle* ConvertedTruck,
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
		IBuildingExpansionOwner* BuildingExpansionOwner,
		const int ExpansionSlotIndex,
		const bool bIsUnpackedExpansion);

	/** @brief function called by the Async spawner when the building expansion is spawned.
	 * @param SpawnedBxp: The spawned building expansion.
	 * @param BxpOwner: The owner of the building expansion.
	 * @param BuildingExpansionType: The type of building expansion.
	 * @param ExpansionSlotIndex: The index in the array of expansions to add the expansion to.
	 * @param bIsUnpackedExpansion: Whether the expansion is unpacked or build for the first time.
	 */
	void OnBxpSpawnedAsync(
		ABuildingExpansion* SpawnedBxp,
		IBuildingExpansionOwner* BxpOwner,
		const EBuildingExpansionType BuildingExpansionType,
		const int ExpansionSlotIndex,
	const bool bIsUnpackedExpansion);
	
	//...

	private:
	//...

	/**
	 * The buidling expansion providing the preview mesh.
	 * @note Needs to be saved because with a second click we need to propagate the valid location to this expansion.
	 */
	UPROPERTY()
	ABuildingExpansion* M_BuildingExpansionForPreview;

	/**
	 * Places the provided building expansion at the given location.
	 * @param BuildingLocation Where to place the building expansion.
	 * @param BuildingExpansion The building expansion to place.
	 */
	void PlaceExpansionBuilding(const FVector& BuildingLocation, ABuildingExpansion* BuildingExpansion) const;


	/**
	 * @brief Destroys the building expansion and stops preview mode.
	 * @note If we are previewing a bxp, make sure to cancel it wiht packed state changes if we were indeed
	 * unpacking a bxp instead of clean building it.
	 */
	void StopBuildingPreviewMode();

	/** @brief Also destroys the preview but without calling a cancel on the bxp placement.
	 * @note used in case we found a valid location to place our preview. */
	void FinishedBuildingMode();

	//...

};
