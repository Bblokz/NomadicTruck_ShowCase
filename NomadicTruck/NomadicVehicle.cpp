// Copyright Bas Blokzijl - All rights reserved.
#include "NomadicVehicle.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "RTS_Survival/Buildings/BuildingExpansion/BuildingExpOwnerComp/BuildingExpansionOwnerComp.h"
#include "RTS_Survival/GameUI/MainGameUI.h"
#include "RTS_Survival/Player/CPPController.h"

#include "Components/AudioComponent.h"
#include "RTS_Survival/GameUI/TrainingUI/TrainerComponent/TrainerComponent.h"
#include "RTS_Survival/RTSComponents/HealthComponent.h"
#include "RTS_Survival/RTSComponents/SelectionComponent.h"
#include "Sound/SoundCue.h"
#include "RTS_Survival/RTSComponents/TimeProgressBarWidget.h"

ANomadicVehicle::ANomadicVehicle()
	: AChaosTank(),
	  M_PreviewMesh(NULL),
	  M_BuildingMesh(NULL),
	  M_NomadStatus(ENomadStatus::Truck),
	  M_StaticPreviewMesh(NULL),
	  M_NomadicAIController(NULL),
	  M_BuildingTransform(FTransform::Identity),
	  m_ConstructionAnimationMaterial(NULL),
	  M_ConstructionMontageTime(20.f),
	  M_MeshAnimationTime(20.f),
	  M_ConstructionFrames(30.f),
	  M_ConvertToVehicleTime(20.f),
	  M_MaterialIndex(0),
	  M_AmountSmokesCovertToVehicle(0),
	  M_ConversionSmokeRadius(100.f),
	  M_DesiredTruckUIOffset(FVector::ZeroVector)
{
	M_NomadStatus = ENomadStatus::Truck;
	BuildingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMeshComponent"));
	if (BuildingMeshComponent)
	{
		BuildingMeshComponent->SetupAttachment(RootComponent);
		BuildingMeshComponent->SetVisibility(false);
		BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	BuildingExpansionComponent = CreateDefaultSubobject<
		UBuildingExpansionOwnerComp>(TEXT("BuildingExpansionComponent"));

	TrainerComponent = CreateDefaultSubobject<UTrainerComponent>(TEXT("TrainerComponent"));
}

void ANomadicVehicle::SetStaticPreviewMesh(AStaticPreviewMesh* NewStaticPreviewMesh)
{
	if (IsValid(M_StaticPreviewMesh))
	{
		M_StaticPreviewMesh->Destroy();
	}
	M_StaticPreviewMesh = NewStaticPreviewMesh;
}

UBuildingExpansionOwnerComp& ANomadicVehicle::GetBuildingExpansionData() const
{
	return *BuildingExpansionComponent;
}

bool ANomadicVehicle::IsBuildingAbleToExpand() const
{
	if (M_NomadStatus == ENomadStatus::Building)
	{
		return true;
	}
	return false;
}

void ANomadicVehicle::InitNomadicVehicle(
	UStaticMesh* NewPreviewMesh,
	UStaticMesh* NewBuildingMesh,
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
	const FSelectionDecalSettings NewSelectionDecalSettings)
{
	M_PreviewMesh = NewPreviewMesh;
	M_BuildingMesh = NewBuildingMesh;
	m_ConstructionAnimationMaterial = NewConstructionAnimationMaterial;
	M_SmokeSystems = SmokeSystems;
	if (NewConstructionFrames <= 0.f)
	{
		M_ConstructionFrames = 30.f;
	}
	else
	{
		M_ConstructionFrames = NewConstructionFrames;
	}
	if (NewConstructionMontageTime <= 0.f)
	{
		M_ConstructionMontageTime = 20.f;
	}
	else
	{
		M_ConstructionMontageTime = NewConstructionMontageTime;
	}
	if (NewMeshAnimationTime <= 0.f)
	{
		M_MeshAnimationTime = 30.f;
	}
	else
	{
		M_MeshAnimationTime = NewMeshAnimationTime;
	}
	if (NewConvertToVehicleTime <= 0.f)
	{
		M_ConvertToVehicleTime = 20.f;
	}
	else
	{
		M_ConvertToVehicleTime = NewConvertToVehicleTime;
	}
	M_AmountSmokesCovertToVehicle = NewAmountSmokesCovertToVehicle;
	M_AttachmentsToSpawn = NewAttachmentsToSpawn;
	M_NiagaraAttachmentsToSpawn = NewNiagaraAttachmentsToSpawn;
	M_SoundAttachmentsToSpawn = NewSoundAttachmentsToSpawn;
	if (NewConversionSmokeRadius <= 0.f)
	{
		M_ConversionSmokeRadius = 100.f;
	}
	else
	{
		M_ConversionSmokeRadius = NewConversionSmokeRadius;
	}
	M_ConversionProgressBar = NewConversionProgressBar;
	M_DesiredTruckUIOffset = NewTruckUIOffset;
	M_SelectionDecalSettings = NewSelectionDecalSettings;
}

void ANomadicVehicle::BeginPlay()
{
	Super::BeginPlay();
	// set m_NomadicAIController.
	M_NomadicAIController = Cast<AAINomadicVehicle>(GetController());
}

void ANomadicVehicle::ExecuteMoveCommand(const FVector MoveToLocation)
{
	if (M_NomadStatus == ENomadStatus::Truck)
	{
		// Executes move command in bp and sets gears in ChaosVehicle.
		// Sets turrets to autoEngage enemies in TankMaster.
		Super::ExecuteMoveCommand(MoveToLocation);
	}
	else
	{
		// todo warning unit not able to move.
		DoneExecutingCommand(EAbilityID::IdMove);
	}
}

void ANomadicVehicle::TerminateMoveCommand()
{
	Super::TerminateMoveCommand();
}

void ANomadicVehicle::ExecuteAttackCommand(AActor* TargetActor)
{
	Super::ExecuteAttackCommand(TargetActor);
}

void ANomadicVehicle::ExecuteCreateBuildingCommand(const FVector BuildingLocation, const FRotator BuildingRotation)
{
	CreateBuildingAtLocationBP(BuildingLocation, BuildingRotation);
}

// Step 1
void ANomadicVehicle::StartBuildingConstruction()
{
	M_NomadStatus = ENomadStatus::CreatingBuildingRotating;
	// Save the preview transform.
	M_BuildingTransform = M_StaticPreviewMesh->GetTransform();
	M_StaticPreviewMesh->Destroy();
	M_NomadicAIController->StopBehaviourTree();
	// teleport to the building location.
	FHitResult HitResult;
	DrawDebugSphere(GetWorld(), M_BuildingTransform.GetLocation(), 100.f, 12, FColor::Red, false, 5.f);
	ChaosVehicleMesh->SetWorldLocation(M_BuildingTransform.GetLocation(), false, &HitResult,
									   ETeleportType::TeleportPhysics);
	// Standalone rotation without command queue.
	ExecuteRotateTowardsCommand(M_BuildingTransform.Rotator(), false);
}

// Step 2
void ANomadicVehicle::OnFinishedStandaloneRotation()
{
	RTSFunctionLibrary::PrintString("Finished standalone rotation");
	M_NomadStatus = ENomadStatus::CreatingBuildingTruckAnim;
	M_ConversionProgressBar->StartProgressBar(GetTotalConstructionTime());

	CreateSmokeForVehicleConversion();

	// Prevents any further commands from being executed unit the cancel command is called.
	// We call the cancel command at OnBuildingFinished.
	StopCommandsUntilCancel();
	// Start the full timer of montage and building animation.
	// Propagate start building to blueprint.
	BeginBuildingTruckAnimationMontage();
}


// Step 3
void ANomadicVehicle::OnTruckMontageFinished()
{
	const UWorld* World = GetWorld();
	if (World)
	{
		// Hide the vehicle mesh
		ChaosVehicleMesh->SetVisibility(false);
		ChaosVehicleMesh->SetGenerateOverlapEvents(false);
		M_NomadStatus = ENomadStatus::CreatingBuildingMeshAnim;

		// Set transform of the building mesh to the transform of the preview mesh.
		// BuildingMeshComponent->SetWorldTransform(m_BuildingTransform);
		BuildingMeshComponent->SetWorldRotation(M_BuildingTransform.Rotator());

		// Show the building mesh.
		BuildingMeshComponent->SetStaticMesh(M_BuildingMesh);
		BuildingMeshComponent->SetVisibility(true);
		BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// Offset the truck UI.
		MoveTruckUIWithLocalOffsets(true);

		// Cache original materials
		CacheOriginalMaterials();
		// Init smoke locations for material animation.
		InitSmokeLocations();

		// Set decal to buildig mode
		AdjustSelectionDecalToConversion(true);

		// apply construction materials and calculate amount materials to exclude.
		const uint32 AmountMaterialsToExclude = ApplyConstructionMaterial(false);
		RTSFunctionLibrary::PrintString(
			"\n\nMaterials to exclude::" + FString::FromInt(AmountMaterialsToExclude) + "\n\n");

		// Start timer to reapply original materials
		const float Interval = M_MeshAnimationTime / (M_CachedOriginalMaterials.Num() - AmountMaterialsToExclude);
		SetAnimationTimer(Interval);
		BPOnStartMeshAnimation();
	}
}

// Step 4
void ANomadicVehicle::OnBuildingFinished()
{
	M_NomadStatus = ENomadStatus::Building;
	M_ConversionProgressBar->StopProgressBar();
	// Open command queue by calling cancel.
	CancelCommand();
	CreateBuildingAttachments();
	PlayerController->TruckConverted(this, true);
}


void ANomadicVehicle::TerminateCreateBuildingCommand()
{
	Super::TerminateCreateBuildingCommand();
	M_ConversionProgressBar->StopProgressBar();
	switch (M_NomadStatus)
	{
	case ENomadStatus::Truck:
		RTSFunctionLibrary::PrintString("terminate building command as truck");
		M_NomadicAIController->StopBehaviourTree();
		M_StaticPreviewMesh->Destroy();
		break;
	case ENomadStatus::CreatingBuildingRotating:
		// Note that BT and static preview are already stopped/destroyed.
		StopRotating();
		RTSFunctionLibrary::PrintString("terminate building command as ROTATING creating building");
		M_NomadStatus = ENomadStatus::Truck;
		break;
	case ENomadStatus::CreatingBuildingTruckAnim:
		BPStopTruckAnimationMontage();
		RTSFunctionLibrary::PrintString("terminate building command as MONTAGE");
		M_NomadStatus = ENomadStatus::Truck;
		break;
	case ENomadStatus::CreatingBuildingMeshAnim:
		RTSFunctionLibrary::PrintString("terminate building command as MESH ANIMATION");
		CancelBuildingMeshAnimation();
		// Set decal to truck mode
		AdjustSelectionDecalToConversion(false);
		M_NomadStatus = ENomadStatus::Truck;
		break;
	case ENomadStatus::Building:
		break;
	default:
		RTSFunctionLibrary::PrintString("cancel building command but is not building!");
	}

	// Only show conversion to construct building if the vehicle did cancel the building command
	// NOT if the vehicle ended the construction command and is now a building.
	if (PlayerController && PlayerController->GetMainMenuUI() && M_NomadStatus == ENomadStatus::Truck)
	{
		// Only show the construct building button if this unit is the primary selected unit.
		PlayerController->GetMainMenuUI()->RequestShowConstructBuilding(this);
	}
}

void ANomadicVehicle::ExecuteConvertToVehicleCommand()
{
	M_NomadStatus = ENomadStatus::CreatingTruck;

	// Starts animations for packing up expansions.
	StartPackUpAllExpansions(M_ConvertToVehicleTime);

	// Prevents any further commands from being added to the queue unit the cancel command is called.
	// We call the cancel command at on finished converting to vehicle.
	StopCommandsUntilCancel();

	RTSFunctionLibrary::PrintString("Converting back to vehicle");

	// Save the building materials to reapply to the mesh after deconstruction is complete.
	CacheOriginalMaterials();

	CreateSmokeForVehicleConversion();
	DestroyAllBuildingAttachments();

	// check if smoke systems are initiated
	uint8 zeroCounter = 0;
	for (auto smokeLocation : M_CreateSmokeTransforms)
	{
		if (smokeLocation.Equals(FTransform::Identity))
		{
			zeroCounter++;
		}
		if (zeroCounter > 1)
		{
			// Init smoke locations for material animation.
			InitSmokeLocations();
			break;
		}
	}
	// don't apply construction materials but only calculate the amount of materials to exclude.
	const uint32 AmountMaterialsToExclude = ApplyConstructionMaterial(true);
	RTSFunctionLibrary::PrintString("\n\nMaterials to exclude::" + FString::FromInt(AmountMaterialsToExclude) + "\n\n");

	// We now reapply construction materials from the top of the array.
	M_MaterialIndex = M_CachedOriginalMaterials.Num() - 1;

	M_ConversionProgressBar->StartProgressBar(M_ConvertToVehicleTime);

	// Start timer to reapply construction materials, m_NomadStatus is used to determine if we are creating the truck or the building.
	const float Interval = M_ConvertToVehicleTime / (M_CachedOriginalMaterials.Num() - AmountMaterialsToExclude);
	SetAnimationTimer(Interval);
	if (PlayerController && PlayerController->GetMainMenuUI())
	{
		PlayerController->GetMainMenuUI()->RequestShowCancelVehicleConversion(this);
	}
}

void ANomadicVehicle::TerminateConvertToVehicleCommand()
{
	if (M_NomadStatus != ENomadStatus::CreatingTruck)
	{
		return;
	}
	Super::TerminateConvertToVehicleCommand();

	M_NomadStatus = ENomadStatus::Building;
	M_ConversionProgressBar->StopProgressBar();
	const UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(MaterialReapplyTimerHandle);
	}
	// set building materials to original materials before construction materials were applied.
	SetAllBuildingMaterialsToCache();
	// Reset index and cache.
	ResetCachedMaterials();

	if (PlayerController && PlayerController->GetMainMenuUI())
	{
		// Only show the convert to vehicle button if this unit is the primary selected unit.
		PlayerController->GetMainMenuUI()->RequestShowConvertToVehicle(this);
	}
	// Propagate to possible bxps to cancel the packing.
	CancelPackUpExpansions();
	// Create the building attachments again.
	CreateBuildingAttachments();

}

void ANomadicVehicle::CreateSmokeForVehicleConversion() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Center = GetActorLocation();
	const FVector BaseScale = FVector(1.f);
	const float Z = Center.Z; // Use the Z coordinate of the actor's location for all smoke effects

	for (int32 i = 0; i < M_AmountSmokesCovertToVehicle; ++i)
	{
		// Calculate angle for this smoke effect
		const float Angle = (360.f / M_AmountSmokesCovertToVehicle) * i;
		const float Radians = FMath::DegreesToRadians(Angle);

		// Calculate x and y position for smoke effect in the circle
		const float X = Center.X + M_ConversionSmokeRadius * FMath::Cos(Radians);
		const float Y = Center.Y + M_ConversionSmokeRadius * FMath::Sin(Radians);
		const FVector SmokeLocation = FVector(X, Y, Z);
		// 20% variability in scale.
		const FVector SmokeScale = BaseScale + FVector(FMath::RandRange(-0.2f, 0.2f),
		                                               FMath::RandRange(-0.2f, 0.2f),
		                                               FMath::RandRange(-0.2f, 0.2f));

		// Spawn smoke effect at this location
		CreateRandomSmokeSystemAtTransform(FTransform(FRotator::ZeroRotator, SmokeLocation, SmokeScale));
	}
}

void ANomadicVehicle::SetAllBuildingMaterialsToCache()
{
	for (int Index = 0; Index < M_CachedOriginalMaterials.Num(); ++Index)
	{
		BuildingMeshComponent->SetMaterial(Index, M_CachedOriginalMaterials[Index]);
	}
}

void ANomadicVehicle::SetAnimationTimer(const float Interval)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MaterialReapplyTimerHandle);
		World->GetTimerManager().SetTimer(MaterialReapplyTimerHandle, this, &ANomadicVehicle::ReapplyOriginalMaterial,
		                                  Interval, true);
	}
}

void ANomadicVehicle::MoveTruckUIWithLocalOffsets(const bool bMoveToBuildingPosition)
{
	if(M_DesiredTruckUIOffset.Equals(FVector::ZeroVector))
	{
		return;
	}
	if (bMoveToBuildingPosition)
	{
		// Calculate the offset to apply to the UI components based on the building mesh
		const FVector DesiredLocation = BuildingMeshComponent->GetRelativeLocation() + M_DesiredTruckUIOffset;
		// Save components original locations
		M_TruckUIElementLocations.Key = HealthComponent->GetLocalLocation();
		M_TruckUIElementLocations.Value = M_ConversionProgressBar->GetLocalLocation();
		const float ZDifference = M_TruckUIElementLocations.Key.Z - M_TruckUIElementLocations.Value.Z;
		HealthComponent->SetLocalLocation(DesiredLocation);
		M_ConversionProgressBar->SetLocalLocation(DesiredLocation + FVector(0.f, 0.f, ZDifference));
	}
	else
	{
		HealthComponent->SetLocalLocation(M_TruckUIElementLocations.Key);
		M_ConversionProgressBar->SetLocalLocation(M_TruckUIElementLocations.Value);
	}
}

void ANomadicVehicle::AdjustSelectionDecalToConversion(const bool bSetToBuildingPosition) const
{
	const TPair<UMaterialInterface*, UMaterialInterface*> Materials = SelectionComponent->GetMaterials();
	// Update with the other set of matierals.
	SelectionComponent->UpdateSelectionMaterials(M_SelectionDecalSettings.State2_SelectionDecalMat, M_SelectionDecalSettings.Sate2_DeselectionDecalMat);
	// Save the previous set of materials.
	M_SelectionDecalSettings.State2_SelectionDecalMat = Materials.Key;
	M_SelectionDecalSettings.Sate2_DeselectionDecalMat = Materials.Value;
	if (bSetToBuildingPosition)
	{
		SelectionComponent->UpdateDecalScale(M_SelectionDecalSettings.State2_SelectionDecalSize);
		SelectionComponent->SetDecalRelativeLocation(M_SelectionDecalSettings.State2_DecalPosition);
		SelectionComponent->UpdateSelectionArea(
			M_SelectionDecalSettings.State2_AreaSize,
			M_SelectionDecalSettings.State2_AreaPosition);
	}
	else
	{
		SelectionComponent->UpdateDecalScale(M_SelectionDecalSettings.State1_SelectionDecalSize);
		SelectionComponent->SetDecalRelativeLocation(M_SelectionDecalSettings.State1_DecalPosition);
		SelectionComponent->UpdateSelectionArea(
			M_SelectionDecalSettings.State1_AreaSize,
			M_SelectionDecalSettings.State1_AreaPosition);
	}
}

void ANomadicVehicle::OnFinishedConvertingToVehicle()
{
	M_NomadStatus = ENomadStatus::Truck;
	// Pack up all building expansions.
	FinishPackUpAllExpansions();

	M_ConversionProgressBar->StopProgressBar();
	GetWorld()->GetTimerManager().ClearTimer(MaterialReapplyTimerHandle);
	// Show the vehicle mesh
	ChaosVehicleMesh->SetVisibility(true);
	ChaosVehicleMesh->SetGenerateOverlapEvents(true);
	ChaosVehicleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ChaosVehicleMesh->SetSimulatePhysics(true);

	// hide the building mesh.
	BuildingMeshComponent->SetVisibility(false);
	BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// set building materials to original materials before construction materials were applied.
	SetAllBuildingMaterialsToCache();
	// Reset index and cache.
	ResetCachedMaterials();

	// free up the command queue, important to call this after the status is set to truck as otherwise the
	// terminate conversion command will trigger.
	CancelCommand();

	if (PlayerController && PlayerController->GetMainMenuUI())
	{
		// Update Game UI.
		PlayerController->TruckConverted(this, false);
	}

	// Move Truck UI back in place.
	MoveTruckUIWithLocalOffsets(false);

	// Set decal to truck mode.
	AdjustSelectionDecalToConversion(false);

	BP_OnFinishedConvertingToVehicle();
}




float ANomadicVehicle::GetConstructionTimeInPlayRate() const
{
	// Assume AnimationFPS is the frame rate the animation was authored at.
	constexpr float AnimationFPS = 30.f;
	const float MontageDuration = M_ConstructionFrames / AnimationFPS;

	// Calculate the play rate required to play the montage over m_ConstructionTime seconds.
	float PlayRate = MontageDuration / M_ConstructionMontageTime;
	RTSFunctionLibrary::PrintString("Playrate::" + FString::SanitizeFloat(PlayRate));
	return PlayRate;
}

UTrainerComponent* ANomadicVehicle::GetTrainerComponent()
{
	return TrainerComponent;
}

void ANomadicVehicle::OnTrainingStarted(const ETrainingOptions StartedOption)
{
	BpOnTrainingStarted(StartedOption);
}

void ANomadicVehicle::OnTrainingComplete(const ETrainingOptions CompletedOption)
{
	BPOnTrainingComplete(CompletedOption);
}

void ANomadicVehicle::OnTrainingCancelled(const ETrainingOptions CancelledOption)
{
	BPOnTrainingCancelled(CancelledOption);
}

void ANomadicVehicle::CacheOriginalMaterials()
{
	ResetCachedMaterials();
	if (BuildingMeshComponent)
	{
		for (int32 i = 0; i < BuildingMeshComponent->GetNumMaterials(); ++i)
		{
			M_CachedOriginalMaterials.Add(BuildingMeshComponent->GetMaterial(i));
		}
	}
}

uint32 ANomadicVehicle::ApplyConstructionMaterial(const bool bOnlyCalculateExcludedMaterials) const
{
	uint32 AmountBuildingTruckMaterials = 0;
	if (BuildingMeshComponent && m_ConstructionAnimationMaterial)
	{
		for (int32 i = 0; i < BuildingMeshComponent->GetNumMaterials(); ++i)
		{
			// Don't apply construction material to materials that are part of the truck.
			if (!ChaosVehicleMesh->GetMaterials().Contains(BuildingMeshComponent->GetMaterial(i)))
			{
				if (!bOnlyCalculateExcludedMaterials)
				{
					BuildingMeshComponent->SetMaterial(i, m_ConstructionAnimationMaterial);
				}
			}
			else
			{
				// Count the amount of materials that are part of the truck and the building.
				AmountBuildingTruckMaterials++;
			}
		}
	}
	return AmountBuildingTruckMaterials;
}

void ANomadicVehicle::ReapplyOriginalMaterial()
{
	// If we are creating the truck we reverse apply construction materials to the building mesh.
	if (M_NomadStatus == ENomadStatus::CreatingTruck)
	{
		if (M_MaterialIndex != 0 && BuildingMeshComponent)
		{
			if (ChaosVehicleMesh->GetMaterials().Contains(M_CachedOriginalMaterials[M_MaterialIndex]))
			{
				// This material is part of the truck, not the animation, skip it.
				M_MaterialIndex--;
				// Retry.
				ReapplyOriginalMaterial();
			}
			else
			{
				BuildingMeshComponent->SetMaterial(M_MaterialIndex, m_ConstructionAnimationMaterial);
				CreateRandomSmokeSystemAtTransform(M_CreateSmokeTransforms[M_MaterialIndex]);
				M_MaterialIndex--;
			}
		}
		else
		{
			OnFinishedConvertingToVehicle();
		}
	}
	else
	{
		if (M_MaterialIndex < M_CachedOriginalMaterials.Num())
		{
			if (BuildingMeshComponent)
			{
				if (ChaosVehicleMesh->GetMaterials().Contains(M_CachedOriginalMaterials[M_MaterialIndex]))
				{
					// This material is part of the truck, not the animation, skip it.
					M_MaterialIndex++;
					// Retry.
					ReapplyOriginalMaterial();
				}
				else
				{
					// We have an unique building material slot to apply the original material to.
					BuildingMeshComponent->SetMaterial(M_MaterialIndex, M_CachedOriginalMaterials[M_MaterialIndex]);
					CreateRandomSmokeSystemAtTransform(M_CreateSmokeTransforms[M_MaterialIndex]);
					M_MaterialIndex++;
				}
			}
		}
		else
		{
			FinishReapplyingMaterials();
		}
	}
}

void ANomadicVehicle::FinishReapplyingMaterials()
{
	GetWorld()->GetTimerManager().ClearTimer(MaterialReapplyTimerHandle);
	OnBuildingFinished();
	// Empty the cached materials and reset the index.
	ResetCachedMaterials();
}

FVector ANomadicVehicle::CalculateMeanMaterialLocation(
	const int32 MaterialIndex,
	const TArray<FVector3f>& VertexPositions,
	const FTransform& TransformOfBuildingMesh) const
{
	FVector3f MeanLocation = FVector3f::ZeroVector;
	int32 VertexCount = VertexPositions.Num();

	for (const FVector3f& Position : VertexPositions)
	{
		MeanLocation += Position;
	}

	if (VertexCount > 0)
	{
		MeanLocation /= VertexCount;
	}

	// Convert MeanLocation to FVector for returning, and transform to world space.
	return TransformOfBuildingMesh.TransformPosition((FVector)MeanLocation);
}

FVector ANomadicVehicle::CalculateMeshPartSize(const TArray<FVector3f>& VertexPositions) const
{
	FVector Min(FLT_MAX);
	FVector Max(-FLT_MAX);

	for (const FVector3f& Position : VertexPositions)
	{
		// Convert to FVector
		FVector ConvertedPosition = FVector(Position);
		Min = Min.ComponentMin(ConvertedPosition);
		Min = Min.ComponentMin(ConvertedPosition);
		Max = Max.ComponentMax(ConvertedPosition);
	}

	// Size vector of the mesh part
	return Max - Min;
}

void ANomadicVehicle::InitSmokeLocations()
{
	if (BuildingMeshComponent && BuildingMeshComponent->GetStaticMesh())
	{
		SetSmokeLocationsToRandomInBox();
		// Save guard to check if the mesh allows CPU access otherwise it breaks shipped builds.
		if (BuildingMeshComponent->GetStaticMesh()->bAllowCPUAccess)
		{
			FStaticMeshLODResources& LODResources = BuildingMeshComponent->GetStaticMesh()->GetRenderData()->
			                                                               LODResources[0];
			FIndexArrayView Indices = LODResources.IndexBuffer.GetArrayView();
			TArray<TArray<FVector3f>> VertexPositionsByMaterial;
			VertexPositionsByMaterial.SetNum(BuildingMeshComponent->GetNumMaterials());

			TArray<float> MeshPartSizes;
			MeshPartSizes.SetNum(BuildingMeshComponent->GetNumMaterials());
			float MaxPartSize = 0;

			// Collect vertices and calculate mesh part sizes. REQUIRES "Allow CPUAccess" in the static mesh editor.
			for (int32 SectionIndex = 0; SectionIndex < LODResources.Sections.Num(); ++SectionIndex)
			{
				const FStaticMeshSection& Section = LODResources.Sections[SectionIndex];
				TArray<FVector3f> PartVertices;

				for (uint32 i = Section.FirstIndex; i < Section.FirstIndex + Section.NumTriangles * 3; i++)
				{
					PartVertices.Add(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[i]));
				}

				FVector PartSize = CalculateMeshPartSize(PartVertices);
				MeshPartSizes[Section.MaterialIndex] = PartSize.Size();

				MaxPartSize = FMath::Max(MaxPartSize, MeshPartSizes[Section.MaterialIndex]);
				VertexPositionsByMaterial[Section.MaterialIndex] = MoveTemp(PartVertices);
			}

			const FTransform ComponentTransform = BuildingMeshComponent->GetComponentTransform();
			FVector OriginLocation = ComponentTransform.GetLocation();
			RTSFunctionLibrary::PrintString("InitSmokeLocations:: Before AsyncTask");
			// A weak pointer to this object to prevent a dangling pointer in the async task.
			TWeakObjectPtr<ANomadicVehicle> WeakThis(this);
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, VertexPositionsByMaterial,
				          ComponentTransform, OriginLocation, MeshPartSizes, MaxPartSize]()
			          {
				          RTSFunctionLibrary::PrintString("InitSmokeLocations:: At async task");
				          TArray<FTransform> CalculatedTransforms;
				          for (int32 i = 0; i < VertexPositionsByMaterial.Num(); ++i)
				          {
					          FVector MeanLocation = WeakThis->CalculateMeanMaterialLocation(
						          i, VertexPositionsByMaterial[i], ComponentTransform);
					          // Normalize scale factor based on the largest part
					          const float ScaleFactor = FMath::Max(2 * (MeshPartSizes[i] / MaxPartSize), 0.33);

					          FRotator Rotation = (MeanLocation - OriginLocation).Rotation();
					          FTransform Transform(Rotation, MeanLocation,
					                               FVector(ScaleFactor, ScaleFactor, ScaleFactor));
					          CalculatedTransforms.Add(Transform);
				          }
				          AsyncTask(ENamedThreads::GameThread, [WeakThis, CalculatedTransforms]()
				          {
					          // Check if the object still exists we cannot access members, for that we need the strong pointer.
					          if (WeakThis.IsValid())
					          {
						          ANomadicVehicle* StrongThis = WeakThis.Get();
						          StrongThis->M_CreateSmokeTransforms = CalculatedTransforms;
						          RTSFunctionLibrary::PrintString("InitSmokeLocations:: Rewrite back to game thread");
					          }
				          });
			          });
		}
		else
		{
			RTSFunctionLibrary::ReportError("Mesh is not CPU Accessible, cannot calculate smoke locations.");
		}
	}
	else
	{
		RTSFunctionLibrary::ReportError("Building Mesh component or static mesh is not set on vehicle: " + GetName());
	}
}

void ANomadicVehicle::SetSmokeLocationsToRandomInBox()
{
	if (BuildingMeshComponent)
	{
		const FBoxSphereBounds MeshBounds = BuildingMeshComponent->CalcBounds(
			BuildingMeshComponent->GetComponentTransform());
		const FBox Box = MeshBounds.GetBox();
		const int32 NumMaterials = BuildingMeshComponent->GetNumMaterials();

		M_CreateSmokeTransforms.Init(FTransform::Identity, NumMaterials);
		for (FTransform& Transform : M_CreateSmokeTransforms)
		{
			Transform.SetLocation(FMath::RandPointInBox(Box));
		}
	}
}

void ANomadicVehicle::CreateRandomSmokeSystemAtTransform(const FTransform Transform) const
{
	if (M_SmokeSystems.Num() == 0)
	{
		// No smoke systems are available
		return;
	}

	// Randomly select a smoke system from the array
	UNiagaraSystem* SelectedSystem = M_SmokeSystems[FMath::RandRange(0, M_SmokeSystems.Num() - 1)];
	if (SelectedSystem)
	{
		// Spawn the Niagara system using the provided transform
		const UWorld* World = GetWorld();
		if (World)
		{
			// Spawn the system at the transform's location, using its rotation
			UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, SelectedSystem, Transform.GetLocation(), Transform.GetRotation().Rotator(),
				Transform.GetScale3D(), true, true, ENCPoolMethod::AutoRelease, true);
		}
	}
}

void ANomadicVehicle::CancelBuildingMeshAnimation()
{
	GetWorld()->GetTimerManager().ClearTimer(MaterialReapplyTimerHandle);
	// Reapply original materials to the building mesh for a possible later animation.
	for (int32 i = 0; i < M_CachedOriginalMaterials.Num(); ++i)
	{
		BuildingMeshComponent->SetMaterial(i, M_CachedOriginalMaterials[i]);
	}
	ResetCachedMaterials();

	// Show the vehicle mesh.
	ChaosVehicleMesh->SetVisibility(true);
	ChaosVehicleMesh->SetGenerateOverlapEvents(true);
	ChaosVehicleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// hide the building mesh.
	BuildingMeshComponent->SetVisibility(false);
	BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Move Truck UI back in place.
	MoveTruckUIWithLocalOffsets(false);

	// Add any attachments back on the vehicle.
	OnMeshAnimationCancelled();
}

void ANomadicVehicle::CreateBuildingAttachments()
{
	if (!BuildingMeshComponent)
	{
		return;
	}
	CreateChildActorAttachments();
	CreateNiagaraAttachments();
	CreateSoundAttachments();
}

void ANomadicVehicle::CreateChildActorAttachments()

{
	for (const auto& [ActorToSpawn, SocketName, Scale] : M_AttachmentsToSpawn)
	{
		if (ActorToSpawn)
		{
			// Attempt to get the transform of the specified socket
			FTransform SocketTransform;
			if (!BuildingMeshComponent->DoesSocketExist(SocketName) ||
				!(SocketTransform = BuildingMeshComponent->GetSocketTransform(SocketName,
				                                                              ERelativeTransformSpace::RTS_World))
				.IsValid())
			{
				continue;
			}

			// Spawn the actor at the socket's location and orientation
			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorToSpawn,
			                                                      SocketTransform.GetLocation(),
			                                                      SocketTransform.GetRotation().Rotator());
			if (SpawnedActor)
			{
				// Attach it to the BuildingMeshComponent at the specified socket
				SpawnedActor->AttachToComponent(BuildingMeshComponent,
				                                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				                                SocketName);
				SpawnedActor->SetActorScale3D(Scale);
				M_SpawnedAttachments.Add(SpawnedActor);
			}
		}
	}
}

void ANomadicVehicle::CreateNiagaraAttachments()
{
	// Handle Niagara system attachments
	for (const auto& [NiagaraSystemToSpawn, SocketName, Scale] : M_NiagaraAttachmentsToSpawn)
	{
		FTransform SocketTransform = BuildingMeshComponent->GetSocketTransform(
			SocketName, ERelativeTransformSpace::RTS_World);
		if (SocketTransform.IsValid())
		{
			UNiagaraComponent* SpawnedSystem = UNiagaraFunctionLibrary::SpawnSystemAttached(
				NiagaraSystemToSpawn, BuildingMeshComponent, SocketName, FVector(0), FRotator(0),
				Scale, EAttachLocation::SnapToTarget, true, ENCPoolMethod::None, true, true);
			if (SpawnedSystem)
			{
				M_SpawnedNiagaraSystems.Add(SpawnedSystem);
				SpawnedSystem->SetWorldScale3D(Scale);
			}
		}
	}
}

void ANomadicVehicle::CreateSoundAttachments()
{
	// Handle sound cue attachments
	for (const FBuildingSoundAttachment& Attachment : M_SoundAttachmentsToSpawn)
	{
		FTransform SocketTransform = BuildingMeshComponent->GetSocketTransform(
			Attachment.SocketName, ERelativeTransformSpace::RTS_World);
		if (SocketTransform.IsValid())
		{
			UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAttached(
				Attachment.SoundCueToSpawn, BuildingMeshComponent, Attachment.SocketName,
				SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(),
				EAttachLocation::SnapToTarget); // Assuming Scale.X is used for volume or range scaling.
			if (AudioComponent)
			{
				M_SpawnedSoundCues.Add(AudioComponent);
			}
		}
	}
}

void ANomadicVehicle::DestroyAllBuildingAttachments()
{
	// Iterate over all the spawned attachment actors and destroy them
	for (AActor* SpawnedActor : M_SpawnedAttachments)
	{
		if (SpawnedActor)
		{
			SpawnedActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			SpawnedActor->Destroy();
		}
	}
	M_SpawnedAttachments.Empty();

	for (const auto NiagaraSystem : M_SpawnedNiagaraSystems)
	{
		if (NiagaraSystem)
		{
			NiagaraSystem->DestroyComponent();
		}
	}
	M_SpawnedNiagaraSystems.Empty();

	for (const auto Sound : M_SpawnedSoundCues)
	{
		if (Sound)
		{
			Sound->DestroyComponent();
		}
	}
}

void ANomadicVehicle::ResetCachedMaterials()
{
	M_CachedOriginalMaterials.Empty();
	M_MaterialIndex = 0;
}
