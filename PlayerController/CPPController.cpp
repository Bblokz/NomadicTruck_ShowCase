// Copyright (C) 2020-2023 Bas Blokzijl - All rights reserved.

#include "CPPController.h"

#include "Abilities.h"
#include "PlacementEffects.h"
#include "AsyncRTSAssetsSpawner/RTSAsyncSpawner.h"
#include "Camera/CameraPawn.h"
#include "Camera/RTSCamera.h"
#include "HUD/CPPHUD.h"
#include "PlayerResourceManager/PlayerResourceManager.h"
#include "RTS_Survival/Buildings/CPPBuildingMaster.h"
#include "RTS_Survival/Buildings/BuildingExpansion/BuildingExpansion.h"
#include "RTS_Survival/Game/Resources/CPPResourceMaster.h"
#include "RTS_Survival/GameUI/MainGameUI.h"
#include "RTS_Survival/MasterObjects/HealthBase/HPActorObjectsMaster.h"
#include "RTS_Survival/MasterObjects/HealthBase/HpCharacterObjectsMaster.h"
#include "RTS_Survival/MasterObjects/Items/ItemsMaster.h"
#include "RTS_Survival/MasterObjects/SelectableBase/SelectableActorObjectsMaster.h"
#include "RTS_Survival/MasterObjects/SelectableBase/SelectablePawnMaster.h"
#include "RTS_Survival/RTSComponents/RTSComponent.h"
#include "RTS_Survival/RTSComponents/SelectionComponent.h"
#include "RTS_Survival/Units/CPP_UnitMaster.h"
#include "RTS_Survival/Units/SquadController.h"
#include "RTS_Survival/Units/Enums/Enum_UnitType.h"
#include "RTS_Survival/Units/Tanks/WheeledTank/BaseTruck/NomadicVehicle.h"
#include "RTS_Survival/Utils/HFunctionLibary.h"
#include "RTS_Survival/Utils/Navigator/RTSNavigator.h"

//...

void ACPPController::ConstructBuilding(AActor* RequestingActor)
{
	if (RequestingActor && RequestingActor->IsA(ANomadicVehicle::StaticClass()))
	{
		if (ANomadicVehicle* NomadicVehicle = Cast<ANomadicVehicle>(RequestingActor))
		{
			NomadicVehicle->SetUnitToIdle();
			if (DeveloperSettings::Debugging::GBuilding_Mode_Compile_DebugSymbols)
			{
				RTSFunctionLibrary::PrintString("ConstructBuilding: " + NomadicVehicle->GetName());
			}
			
			CPPConstructionPreviewRef->StartBuildingPreview(NomadicVehicle->GetPreviewMesh());
			m_IsBuildingPreviewModeActive = EBuildingPreviewMode::NomadicPreviewMode;
			m_ActiveAbility = EAbilityID::IdCreateBuilding;
			// Note that we do not call the MainGameUI to show the cancel button as this is already
			// completed by MainGameUI itself since this function is called from the MainGameUI.
		}
	}
}

void ACPPController::ConvertBackToVehicle(AActor* RequestingActor)
{
	if (RequestingActor)
	{
		if (ANomadicVehicle* NomadicVehicle = Cast<ANomadicVehicle>(RequestingActor))
		{
			NomadicVehicle->ConvertToVehicle(true);
		}
	}
}

void ACPPController::CancelVehicleConversion(AActor* RequestingActor)
{
	if (RequestingActor)
	{
		if (ANomadicVehicle* NomadicVehicle = Cast<ANomadicVehicle>(RequestingActor))
		{
			NomadicVehicle->SetUnitToIdle();
		}
	}
}

void ACPPController::TruckConverted(
	ANomadicVehicle* ConvertedTruck,
	const bool bConvertedToBuilding) const
{
	M_MainGameUI->OnTruckConverted(ConvertedTruck, bConvertedToBuilding);
}


void ACPPController::ExpandBuildingWithType(
	const EBuildingExpansionType BuildingExpansionType,
	IBuildingExpansionOwner* BuildingExpansionOwner,
	const int ExpansionSlotIndex,
	const bool bIsUnpackedExpansion)
{

	// Callback to OnBxpSpawnedAsync when the loading is complete.
	M_RTSAsyncSpawner->AsyncSpawnBuildingExpansion(BuildingExpansionType, BuildingExpansionOwner, ExpansionSlotIndex,
	                                               bIsUnpackedExpansion);
	if(UStaticMesh* PreviewMesh = M_RTSAsyncSpawner->SyncGetBuildingExpansionPreviewMesh(BuildingExpansionType))
	{
		CPPConstructionPreviewRef->StartBuildingPreview(PreviewMesh);
		m_IsBuildingPreviewModeActive = EBuildingPreviewMode::ExpansionPreviewMode;
	}
	else
	{
		RTSFunctionLibrary::ReportError("Could not load preview mesh for building expansion type: " + FString::FromInt((int32)BuildingExpansionType)
			+ "\n See function ExpandBuildingWithType in CPPController.cpp");
	}
}

void ACPPController::OnBxpSpawnedAsync(
	ABuildingExpansion* SpawnedBxp,
	IBuildingExpansionOwner* BxpOwner,
	const EBuildingExpansionType BuildingExpansionType,
	const int ExpansionSlotIndex,
	const bool bIsUnpackedExpansion)
{
	M_BuildingExpansionForPreview = SpawnedBxp;
	BxpOwner->OnBuildingExpansionCreated(SpawnedBxp, ExpansionSlotIndex, BuildingExpansionType, bIsUnpackedExpansion);
}

PlaceExpansionBuilding(
	const FVector& BuildingLocation,
	ABuildingExpansion* BuildingExpansion) const
{
	const FRotator BuildingRotation = CPPConstructionPreviewRef->GetPreviewRotation();
	// Notifies owner of all state changes and owner updates MainGameUI if needed.
	// Note that this function is also used to unpack a building expansion.
	BuildingExpansion->StartExpansionConstructionAtLocation(BuildingLocation, BuildingRotation);
}

void ACPPController::StopBuildingPreviewMode()
{
	switch (m_IsBuildingPreviewModeActive)
	{
	case EBuildingPreviewMode::ExpansionPreviewMode:
		{
			// If we are previewing a bxp, make sure to cancel it and cache the packed state if we were
			// unpacking a bxp instead of clean building it so we can place the packed bxp somewhere else later.
			const bool IsCancelOfPackedExpansion = M_BuildingExpansionForPreview->GetBuildingExpansionStatus() ==
			                                       EBuildingExpansionStatus::BXS_LookingForUnpackingLocation
				                                       ? true
				                                       : false;
			// Also calls FinishedBuildingMode to stop the preview on the constructionPreview reference.
			CancelBuildingExpansionPlacement(M_BuildingExpansionForPreview->GetBuildingExpansionOwner(),
			                                 IsCancelOfPackedExpansion);
			break;
		}
	case EBuildingPreviewMode::NomadicPreviewMode:
		if (m_NomadicVehicleSelectedForBuilding)
		{
			M_MainGameUI->RequestShowCancelBuilding(m_NomadicVehicleSelectedForBuilding);
		}
		FinishedBuildingMode();
		break;
	case EBuildingPreviewMode::BuildingPreviewModeOFF:
		break;
	default:
		break;
	}
}

void ACPPController::CancelBuildingExpansionPlacement(IBuildingExpansionOwner* BxpOwner,
                                                      const bool bIsCancelledPackedExpansion)
{
	if (IsValid(M_BuildingExpansionForPreview) && BxpOwner)
	{
		// Was this bxp packed? If so we save the type and set the status to IsPackedUp on the data component of the owner.
		// This makes sure we can unpack it later at a different location.
		BxpOwner->DestroyBuildingExpansion(M_BuildingExpansionForPreview, bIsCancelledPackedExpansion);
	}
	FinishedBuildingMode();
}

void ACPPController::CancelBuildingExpansionConstruction(IBuildingExpansionOwner* BxpOwner,
                                                         ABuildingExpansion* BuildingExpansion,
                                                         const bool bIsCancelledPackedBxp) const
{
	if (IsValid(BuildingExpansion) && BxpOwner)
	{
		BxpOwner->DestroyBuildingExpansion(BuildingExpansion, bIsCancelledPackedBxp);
	}
}

//...