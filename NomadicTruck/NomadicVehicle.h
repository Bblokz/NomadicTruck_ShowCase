﻿// Copyright Bas Blokzijl - All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "AINomadicVehicle.h"
#include "RTS_Survival/Buildings/BuildingExpansion/Interface/BuildingExpansionOwner.h"
#include "RTS_Survival/Player/ConstructionPreview/StaticMeshPreview/StaticPreviewMesh.h"
#include "RTS_Survival/Units/Tanks/WheeledTank/ChaosTank.h"
#include "RTS_Survival/Buildings/BuildingAttachments/BuildingAttachments.h"
#include "RTS_Survival/GameUI/TrainingUI/Interface/Trainer.h"
#include "RTS_Survival/RTSComponents/SelectionDecalSettings/SelectionDecalSettings.h"

#include "NomadicVehicle.generated.h"

class RTS_SURVIVAL_API UBuildingExpansionOwnerComp;
// Forward declarations.
class RTS_SURVIVAL_API UTimeProgressBarWidget;

UENUM()
enum class ENomadStatus
{
	Truck,
	Building,
	// Rotating to align with the building location.
	CreatingBuildingRotating,
	// Animating the truck expanding.
	CreatingBuildingTruckAnim,
	// Animating the building mesh construction.
	CreatingBuildingMeshAnim,
	CreatingTruck
};


/**
 * @brief A vehicle that can deploy into a building and pack it up back into a vehicle.
 * @note Set "Allow CPUAccess" in the static mesh editor for the building mesh to allow for smoke effects.
 * @note ---------------------------------------------------------------
 * @note Follow the conversion path:
 * @note 1) StartBuildingConstruction BP: OnMoveTruckToBuildingLocationBP
 * @note The truck is now moved to the building location and the chasis teleported
 * @note 2) OnFinishedStandaloneRotation
 * @note The truck is now aligned with the building location.
 * @note 3) BP: BeginBuildingTruckAnimationMontage
 * @note This starts the montage animation in the blueprint.
 * @note 4) OnTruckMontageFinished BP: BPOnStartMeshAnimation
 * @note Now starts the material animation using timers.
 * @note 5) OnBuildingFinished.
 * Creates building attachments and opens command queue again.
 * @note ---------------------------------------------------------------
 * @note Keeps the EAbilityID::IdCreateBuilding in the queue during the whole process.
 * @note Sets truck to StopCommandsUntilCancel after rotating in place for the corrrect building angle.
 * @note the building process is always cancelled with the terminate command that
 * @note is associated with the EAbilityID::IdCreateBuilding.
 * @note ---------------------------------------------------------------
 * @note for building acceptance radius see AAINomadicVehicle::ConstructionAcceptanceRad.
 */
UCLASS()
class RTS_SURVIVAL_API ANomadicVehicle : public AChaosTank, public IBuildingExpansionOwner, public ITrainer
{
	GENERATED_BODY()

public:
	ANomadicVehicle();

	/** @brief Starts constructing the building.
	 * @note called by Task_CreateBuilding Upon completing movement.
	 */
	void StartBuildingConstruction();

	/** @return Whether this truck is movable and not a stationary base. */
	inline bool GetIsInTruckMode() const { return M_NomadStatus == ENomadStatus::Truck; }

	inline ENomadStatus GetNomadicStatus() const { return M_NomadStatus; }
	
	/** @return The preview mesh of the building that this unit constructs. */
	inline UStaticMesh* GetPreviewMesh() const { return M_PreviewMesh; }

	void SetStaticPreviewMesh(AStaticPreviewMesh* NewStaticPreviewMesh);

	float GetTotalConstructionTime() const { return M_ConstructionMontageTime + M_MeshAnimationTime; }

	float GetTotalConvertToVehicleTime() const { return M_ConvertToVehicleTime; }

	/**
	 * @brief overwrite from IBuildingExpansionOwner.
	 * @return Whether the building is in a state in which expanding is possible.
	 */
	virtual bool IsBuildingAbleToExpand() const override final;

protected:
	/**
	 * @brief Initializes the NomadicVehicle.
	 * @param NewPreviewMesh The mesh used to choose the building location.
	 * @param NewBuildingMesh The actual high poly mesh of the building.
	 * @param NewConstructionAnimationMaterial The material used to animate the building construction.
	 * @param SmokeSystems The smoke systems to spawn when the building is being constructed.
	 * @param NewConstructionFrames The amount of frames in the construction animation.
	 * @param NewConstructionMontageTime The amount of time spend on montage.
	 * @param NewMeshAnimationTime The amount of time spend on mesh animation.
	 * @param NewConvertToVehicleTime How long it takes to convert from a building back to a vehicle.
	 * @param NewAmountSmokesCovertToVehicle How many smokes to spawn when converting to a vehicle.
	 * @param NewAttachmentsToSpawn The attachments to spawn once the building is constructed.
	 * @param NewNiagaraAttachmentsToSpawn The niagara attachments to spawn once the building is constructed.
	 * @param NewSoundAttachmentsToSpawn The sound attachments to spawn once the building is constructed.
	 * @param NewConversionSmokeRadius In how big of a circle surrounding the vehicle we create smokes when starting conversion.
	 * @param NewConversionProgressBar The progress bar to display the conversion progress which is a derived blueprint
	 * of which the cpp class is UTimeProgressBarWidget.
	 * @param NewTruckUIOffset The offset to apply to the truck UI elements when converting to a vehicle.
	 * @param NewSelectionDecalSettings To define decal size, offset and materials when switching between truck and building.
	 * @note Truck selection materials are set on the selection component.
	 */
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category = "ReferenceCasts")
	void InitNomadicVehicle(UStaticMesh* NewPreviewMesh, UStaticMesh* NewBuildingMesh,
	                        UMaterialInstance* NewConstructionAnimationMaterial,
	                        TArray<UNiagaraSystem*> SmokeSystems,
	                        const float NewConstructionFrames,
	                        const float NewConstructionMontageTime,
	                        const float NewMeshAnimationTime,
	                        const float NewConvertToVehicleTime,
	                        const int NewAmountSmokesCovertToVehicle,
	                        TArray<FBuildingAttachment> NewAttachmentsToSpawn,
	                        TArray<FBuildingNiagaraAttachment> NewNiagaraAttachmentsToSpawn,
	                        TArray<FBuildingSoundAttachment> NewSoundAttachmentsToSpawn,
	                        const float NewConversionSmokeRadius,
	                        UTimeProgressBarWidget* NewConversionProgressBar,
	                        const FVector NewTruckUIOffset,
	                        const FSelectionDecalSettings NewSelectionDecalSettings);


	virtual void BeginPlay() override;

	// From IBuildingExpansionOwner.
	virtual UBuildingExpansionOwnerComp& GetBuildingExpansionData() const override final;

	virtual void ExecuteMoveCommand(const FVector MoveToLocation) override;
	virtual void TerminateMoveCommand() override;
	virtual void ExecuteAttackCommand(AActor* TargetActor) override;

	// Static Mesh for the building representation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* BuildingMeshComponent;

	/**
	 * @brief Creates a building with the provided rotation at the provided location.
	 * @param BuildingLocation The location to place the building.
	 * @param BuildingRotation The Rotation to place the building with.
	 */
	virtual void ExecuteCreateBuildingCommand(
		const FVector BuildingLocation,
		const FRotator BuildingRotation) override final;


	/** @brief Stops the building creation command and associated logic. */
	virtual void TerminateCreateBuildingCommand() override final;

	virtual void ExecuteConvertToVehicleCommand() override final;

	virtual void TerminateConvertToVehicleCommand() override final;

	/**
	 * @brief Executes the create building command in the blueprint by activating BT.
	 * @param BuildingLocation The location to place the building.
	 * @param BuildingRotation The Rotation to place the building with.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void CreateBuildingAtLocationBP(
		const FVector BuildingLocation,
		const FRotator BuildingRotation);

	/**
	 * @brief Called when the truck has completed moving to the building location and is now getting ready to rotate towards the
	 * direction of the building. (before montage is played)
	 * @param BuildingLocation The location where the building will be placed.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnMoveTruckToBuildingLocationBP(const FVector BuildingLocation);

	/** @brief Signals the start of building construction in the blueprint. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "BuildingInConstruction")
	void BeginBuildingTruckAnimationMontage();

	// Stops the montage of the truck expanding.
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "BuildingInConstruction")
	void BPStopTruckAnimationMontage();

	// Called from blueprint when the animation is finished to start applying the construction material.
	UFUNCTION(BlueprintCallable, NotBlueprintable, Category = "BuildingInConstruction")
	void OnTruckMontageFinished();

	// Called When the truck starts the material animation
	UFUNCTION(BlueprintImplementableEvent, Category = "BuildingInConstruction")
	void BPOnStartMeshAnimation();

	// Finished rotating on building placement location.
	virtual void OnFinishedStandaloneRotation() override final;

	/**
	 * @brief Handles logic when the mesh animation was stopped due to cancel building.
	 * Sets truck attachments to visible if needed.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Truck")
	void OnMeshAnimationCancelled();

	// The play rate on the montage.
	UFUNCTION(BlueprintCallable, NotBlueprintable, BlueprintPure, Category = "BuildingInConstruction")
	float GetConstructionTimeInPlayRate() const;

	UFUNCTION(BlueprintImplementableEvent, Category="ConvertToTruck")
	void BP_OnFinishedConvertingToVehicle();

	UPROPERTY(BlueprintReadOnly)
	UBuildingExpansionOwnerComp* BuildingExpansionComponent;

	UPROPERTY(Category = ChaosVehicle, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UTrainerComponent* TrainerComponent;

	// Trainer functions

	/** @return the Trainer component of this nomadic vehicle. */
	virtual UTrainerComponent* GetTrainerComponent() override;

	/**
	 * @brief Called when training of an option is started.
	 * @param StartedOption What is being trained.
	 */
	virtual void OnTrainingStarted(const ETrainingOptions StartedOption) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Trainer")
	void BpOnTrainingStarted(const ETrainingOptions StartedOption);

	/**
	 * @brief Called when training of an option is completed.
	 * @param CompletedOption What is completed.
	 */
	virtual void OnTrainingComplete(const ETrainingOptions CompletedOption) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Trainer")
	void BPOnTrainingComplete(const ETrainingOptions CompletedOption);

	/**
	 * @brief Called when the training of an option is cancelled.
	 * @param CancelledOption The option that was cancelled.
	 */
	virtual void OnTrainingCancelled(const ETrainingOptions CancelledOption) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Trainer")
	void BPOnTrainingCancelled(const ETrainingOptions CancelledOption);

private:
	// The mesh used as building preview.
	UPROPERTY()
	UStaticMesh* M_PreviewMesh;

	// The complete (high poly) mesh of the building.
	UPROPERTY()
	UStaticMesh* M_BuildingMesh;

	// Whether the vehicle is a stationary base or movable.
	UPROPERTY()
	ENomadStatus M_NomadStatus;

	// A reference to the static preview that is placed when the building location is determined.
	UPROPERTY()
	AStaticPreviewMesh* M_StaticPreviewMesh;

	// The ai controller, used to stop the behaviour tree when the building is finished.
	UPROPERTY()
	AAINomadicVehicle* M_NomadicAIController;

	// The transform of the preview mesh used to place the building.
	UPROPERTY()
	FTransform M_BuildingTransform;

	// The material used to animate the building construction.
	UPROPERTY()
	UMaterialInstance* m_ConstructionAnimationMaterial;

	// Amount of time spend on montage.
	UPROPERTY()
	float M_ConstructionMontageTime;

	// Amount of time spend on mesh animation.
	UPROPERTY()
	float M_MeshAnimationTime;

	// How many frames are in the montage.
	UPROPERTY()
	float M_ConstructionFrames;

	// How long it takes to convert from a building back to a vehicle.
	UPROPERTY()
	float M_ConvertToVehicleTime;

	// When the conversion is complete
	void OnFinishedConvertingToVehicle();

	UPROPERTY()
	FTimerHandle ConvertToVehicleTimerHandle;

	// After the last material is set.
	void OnBuildingFinished();

	// Cache for original materials of the building mesh
	// Always empty by calling ResetCachedMaterials as the index also needs to be reset!
	UPROPERTY()
	TArray<UMaterialInterface*> M_CachedOriginalMaterials;

	// Timer handle for reapplying materials
	FTimerHandle MaterialReapplyTimerHandle;

	// To save original materials of the building mesh.
	void CacheOriginalMaterials();

	/**
	 * Applies construction material to all non truck material slots.
	 * @return The amount of materials that occur in both the truck and the building and are excluded.
	 * from the construction material animation.
	 * @param bOnlyCalculateExcludedMaterials Whether to only calculate the excluded materials and do not
	 * change any materials on the building mesh.
	 */
	uint32 ApplyConstructionMaterial(const bool bOnlyCalculateExcludedMaterials) const;

	// Called by timer to reapply original materials one by one.
	void ReapplyOriginalMaterial();

	// To stop reapplying materials and finish building.
	void FinishReapplyingMaterials();

	/**
	 * Calculates the mean location of the material at the provided index.
	 * @param MaterialIndex The index of the material to calculate the mean location for.
	 * @param VertexPositions The vertex positions of the building mesh for this material.
	 * @param TransformOfBuildingMesh The transform of the building mesh.
	 * @return The mean location of the material.
	 */
	FVector CalculateMeanMaterialLocation(const int32 MaterialIndex,
	                                      const TArray<FVector3f>& VertexPositions,
	                                      const FTransform& TransformOfBuildingMesh) const;

	/**
	 * Computes the size (extent) of each mesh part associated with a material index.
	 * It's used to determine the relative scale for the smoke effects.
	 * @param VertexPositions 
	 * @return The size of the mesh part.
	 */
	FVector CalculateMeshPartSize(const TArray<FVector3f>& VertexPositions) const;

	// The current material index to reapply a material to.
	UPROPERTY()
	int32 M_MaterialIndex;

	UPROPERTY()
	TArray<FTransform> M_CreateSmokeTransforms;

	/**
	 * @brief Creates an array of smoke locations for all materials, uses async to find material means.
	 * @note Requires the final mesh of the BuildingMeshComponent to be set to "Allow CPUAccess" in the static mesh editor.
	 */
	void InitSmokeLocations();

	// Calculates random locations in the box of the building mesh to spawn smoke at.
	void SetSmokeLocationsToRandomInBox();

	UPROPERTY()
	// The smoke system(s) to spawn when the building is being constructed.
	TArray<UNiagaraSystem*> M_SmokeSystems;

	// Spawns a smoke system at the provided location.
	void CreateRandomSmokeSystemAtTransform(const FTransform Transform) const;

	void CancelBuildingMeshAnimation();

	// Attaches all the attachments to the building mesh.
	void CreateBuildingAttachments();

	// Goes through the array of BuildingAttachments and spawns them.
	void CreateChildActorAttachments();

	// Goes through the array of Niagara attachments and spawns them.
	void CreateNiagaraAttachments();

	// Goes through the array of SoundCue attachments and spawns them.
	void CreateSoundAttachments();

	// Destroys all building attachments.
	void DestroyAllBuildingAttachments();

	// The spawned attachments to destroy when the building is destroyed.
	UPROPERTY()
	TArray<AActor*> M_SpawnedAttachments;

	// Array collection of attachments to spawn once the building is constructed.
	UPROPERTY()
	TArray<FBuildingAttachment> M_AttachmentsToSpawn;

	// Array collection of niagara attachments to spawn once the building is constructed.
	UPROPERTY()
	TArray<FBuildingNiagaraAttachment> M_NiagaraAttachmentsToSpawn;

	// Array collection of sound attachments to spawn once the building is constructed.
	UPROPERTY()
	TArray<FBuildingSoundAttachment> M_SoundAttachmentsToSpawn;

	// Keeps track of the currently attached niagara particle systems.
	UPROPERTY()
	TArray<UNiagaraComponent*> M_SpawnedNiagaraSystems;

	// Keeps track of the currently attached sound cues.
	UPROPERTY()
	TArray<UAudioComponent*> M_SpawnedSoundCues;

	// Also resets the class-global material index.
	void ResetCachedMaterials();

	// Creates the smoke for the vehicle conversion.
	void CreateSmokeForVehicleConversion() const;

	UPROPERTY()
	int M_AmountSmokesCovertToVehicle;

	// Instantly sets all the building materials on the building mesh to the material array saved
	// in the cache.
	void SetAllBuildingMaterialsToCache();

	// In how big of a circle surrounding the vehicle we create smokes when starting conversion.
	UPROPERTY()
	float M_ConversionSmokeRadius;

	UPROPERTY()
	UTimeProgressBarWidget* M_ConversionProgressBar;

	/**
	 * @brief Starts the mesh material animation.
	 * @param Interval The interval in seconds between updating each material slot.
	 */
	void SetAnimationTimer(const float Interval);

	/**
	 * @brief Applies local offset to the truck UI elements depending on the conversion.
	 * @param bMoveToBuildingPosition If true move the UI elements to the truck position,
	 * otherwise move them to the building position.
	 */
	void MoveTruckUIWithLocalOffsets(const bool bMoveToBuildingPosition);

	// The offset the user wants from the base of the building mesh.
	FVector M_DesiredTruckUIOffset;

	// Stores the locations of the truck UI elements.
	TPair<FVector, FVector> M_TruckUIElementLocations;

	UPROPERTY()
	FSelectionDecalSettings M_SelectionDecalSettings;

	void AdjustSelectionDecalToConversion(const bool bSetToBuildingPosition) const;
};
