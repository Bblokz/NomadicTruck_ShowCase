// Copyright Bas Blokzijl - All rights reserved.


#include "CPPConstructionPreview.h"

#include "Blueprint/UserWidget.h"
#include "PreviewWidget/W_PreviewStats.h"
#include "RTS_Survival/Player/CPPController.h"
#include "RTS_Survival/Utils/HFunctionLibary.h"
#include "StaticMeshPreview/StaticPreviewMesh.h"
#include "RTS_Survival/DeveloperSettings.h"
#include "RTS_Survival/RTSCollisionTraceChannels.h"
#include "RTS_Survival/Player/Camera/CameraPawn.h"
FName ACPPConstructionPreview::PreviewMeshComponentName(TEXT("PreviewMesh"));


ACPPConstructionPreview::ACPPConstructionPreview()
	: bM_BHasActivePreview(false),
	  M_ConstructionPreviewMaterial(NULL),
	  bM_IsValidCursorLocation(false),
	  RotationDegrees(0),
	  bM_IsValidBuildingLocation(false),
	  M_HostLocation(FVector::Zero()),
	  M_BuildRadius(0),
	  M_SlopeAngle(0),
	  M_PreviewStatsWidget(nullptr),
	  M_DynamicMaterialPool()
{
	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize the preview mesh component
	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(PreviewMeshComponentName);
	PreviewMesh->BodyInstance.bSimulatePhysics = false;
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PreviewMesh->SetCollisionObjectType(ECollisionChannel::COLLISION_OBJ_BUILDING_PLACEMENT);
	PreviewMesh->SetCollisionResponseToChannel(
		ECollisionChannel::COLLISION_OBJ_BUILDING_PLACEMENT, ECollisionResponse::ECR_Overlap);
	PreviewMesh->SetCollisionResponseToChannel(
		ECollisionChannel::COLLISION_OBJ_ENEMY, ECollisionResponse::ECR_Overlap);
	PreviewMesh->SetCollisionResponseToChannel(
		ECollisionChannel::COLLISION_TRACE_ENEMY, ECollisionResponse::ECR_Overlap);
	PreviewMesh->SetGenerateOverlapEvents(true);
	PreviewMesh->SetCanEverAffectNavigation(false);
	PreviewMesh->SetCastShadow(false);
	RootComponent = PreviewMesh;

	// Initialize rotation degrees
	RotationDegrees = 10.f;
}


// Called when the game starts or when spawned
void ACPPConstructionPreview::BeginPlay()
{
	Super::BeginPlay();
}

FVector ACPPConstructionPreview::GetGridSnapAdjusted(
	const FVector& MouseWorldPosition,
	const float ExtraHeight) const
{
	constexpr float GridSize = DeveloperSettings::GamePlay::Construction::GridSnapSize;
	const float X = FMath::RoundToFloat(MouseWorldPosition.X / GridSize) * GridSize;
	const float Y = FMath::RoundToFloat(MouseWorldPosition.Y / GridSize) * GridSize;
	const float Z = MouseWorldPosition.Z + ExtraHeight;
	return FVector(X, Y, Z);
}

bool ACPPConstructionPreview::IsOverlapping() const
{
	TArray<AActor*> OverlappingActors;
	// m_CollisionBox->GetOverlappingActors(OverlappingActors);
	PreviewMesh->GetOverlappingActors(OverlappingActors);
	if (DeveloperSettings::Debugging::GConstruction_Preview_Compile_DebugSymbols)
	{
		RTSFunctionLibrary::PrintString("Number of overlapping actors: " + FString::FromInt(OverlappingActors.Num()),
		                                FColor::Red);
	}

	return OverlappingActors.Num() >= 1;
}


void ACPPConstructionPreview::InitConstructionPreview(
	ACPPController* NewPlayerController,
	UMaterialInstance* ConstructionMaterial,
	UW_PreviewStats* PreviewStatsWidget,
	UWidgetComponent* PreviewStatsWidgetComponent)
{
	PlayerController = NewPlayerController;
	M_ConstructionPreviewMaterial = ConstructionMaterial;
	M_PreviewStatsWidget = PreviewStatsWidget;
	PreviewStatsWidget->InitW_PreviewStats();
	M_PreviewStatsWidgetComponent = PreviewStatsWidgetComponent;

	// Initialize the pool of dynamic material instances
	InitializeDynamicMaterialPool(ConstructionMaterial);
}


void ACPPConstructionPreview::Tick(float DeltaTime)
{
	bool bIsSlopeValid = false;
	Super::Tick(DeltaTime);
	if (bM_BHasActivePreview)
	{
		SetCursorPosition(
			PlayerController->GetCursorWorldPosition(DeveloperSettings::UIUX::SightDistanceMouse,
			                                         bM_IsValidCursorLocation));
		if (bM_IsValidCursorLocation)
		{
			// Move preview along grid.
			SetActorLocation(GetGridSnapAdjusted(CursorWorldPosition));
			// Check if the slope is valid for the current cursor position.
			bIsSlopeValid = IsSlopeValid(CursorWorldPosition);
			if (!bIsSlopeValid || IsOverlapping())
			{
				// Invalid location due to slope or overlap.
				if (DeveloperSettings::Debugging::GConstruction_Preview_Compile_DebugSymbols)
				{
					RTSFunctionLibrary::PrintString("Cannot place building here", FColor::Red);
				}
				UpdatePreviewMaterial(false);
				bM_IsValidBuildingLocation = false;
			}
			else
			{
				// Valid location.
				UpdatePreviewMaterial(true);
				bM_IsValidBuildingLocation = true;
			}
		}
		else
		{
			// Location outside of view.
			if (DeveloperSettings::Debugging::GConstruction_Preview_Compile_DebugSymbols)
			{
				RTSFunctionLibrary::PrintString("location outside of view", FColor::Red);
			}
			bM_IsValidBuildingLocation = false;
		}
		UpdatePreviewStatsWidget(bIsSlopeValid);
	}
}

void ACPPConstructionPreview::SetCursorPosition(const FVector& CursorLocation)
{
	CursorWorldPosition = CursorLocation;
}

bool ACPPConstructionPreview::GetIsBuildingPreviewBlocked() const
{
	return !bM_IsValidBuildingLocation;
}

AStaticPreviewMesh* ACPPConstructionPreview::CreateStaticMeshActor(const FRotator& Rotation) const
{
	// create a staticPreview Mesh and set the preview mesh to the preview mesh of this actor.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// at location CursorWorldPosition.
	AStaticPreviewMesh* StaticPreviewMesh = GetWorld()->SpawnActor<AStaticPreviewMesh>(
		CursorWorldPosition, Rotation, SpawnParams);
	StaticPreviewMesh->SetPreviewMesh(PreviewMesh->GetStaticMesh(), M_ConstructionPreviewMaterial);
	return StaticPreviewMesh;
}

void ACPPConstructionPreview::StartBuildingPreview(
	UStaticMesh* NewPreviewMesh,
	const FVector HostLocation,
	const float BuildRadius)
{
	bM_IsValidBuildingLocation = false;
	if (NewPreviewMesh)
	{
		PreviewMesh->SetStaticMesh(NewPreviewMesh);
		bM_BHasActivePreview = true;
		M_PreviewStatsWidget->SetVisibility(ESlateVisibility::Visible);
		MoveWidgetToMeshHeight();

		// Apply a dynamic material from the pool to each material slot.
		for (int i = 0; i < PreviewMesh->GetNumMaterials(); ++i)
		{
			if (M_DynamicMaterialPool.IsValidIndex(i))
			{
				PreviewMesh->SetMaterial(i, M_DynamicMaterialPool[i]);
			}
		}
	}
	else
	{
		RTSFunctionLibrary::ReportError("Attempt to start building preview with null mesh!"
								  "\n at function StartBuildingPreview in CPPConstructionPreview.cpp"
								  "\n Actor: " + GetName());
	}
}

void ACPPConstructionPreview::StopBuildingPreview()
{
	PreviewMesh->SetStaticMesh(nullptr);
	bM_BHasActivePreview = false;
	// Reset rotation.
	PreviewMesh->SetWorldRotation(FRotator::ZeroRotator);
	M_PreviewStatsWidget->SetVisibility(ESlateVisibility::Hidden);
}

void ACPPConstructionPreview::UpdatePreviewStatsWidget(const bool bIsInclineValid) const
{
	if (M_PreviewStatsWidget && bM_BHasActivePreview)
	{
		RotatePreviewStatsToCamera();
		const FVector CursorLocation = CursorWorldPosition;
		const float Distance = (CursorLocation - M_HostLocation).Size();
		const float Degrees = PreviewMesh->GetComponentRotation().Yaw;

		M_PreviewStatsWidget->UpdateInformation(Degrees, M_SlopeAngle, bIsInclineValid, M_BuildRadius > 0, Distance,
		                                        M_BuildRadius);
	}
}


void ACPPConstructionPreview::RotatePreviewStatsToCamera() const
{
	if (PlayerController && PlayerController->CameraRef)
	{
		const FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(
			M_PreviewStatsWidgetComponent->GetComponentLocation(),
			PlayerController->CameraRef->GetCameraComponent()->GetComponentLocation());
		// Apply the calculated rotation to the widget component
		M_PreviewStatsWidgetComponent->SetWorldRotation(NewRotation);
	}
}


bool ACPPConstructionPreview::IsWithinBuildRadius(const FVector& Location) const
{
	return (Location - M_HostLocation).Size() <= M_BuildRadius;
}

bool ACPPConstructionPreview::IsSlopeValid(const FVector& Location)
{
	TArray<FVector> TraceStartPoints;
	TArray<FName> SocketNames = { "FL", "FR", "RL", "RR" };
	bool bSocketsFound = true;

	// Check for sockets and add their locations to the trace points
	for (const FName& SocketName : SocketNames)
	{
		if (PreviewMesh->DoesSocketExist(SocketName))
		{
			TraceStartPoints.Add(PreviewMesh->GetSocketLocation(SocketName) + FVector(0.0f, 0.0f,
				DeveloperSettings::GamePlay::Construction::AddedHeightToTraceSlopeCheckPoint));
		}
		else
		{
			bSocketsFound = false;
			break;
		}
	}

	// The sockets are not found, we fall back to the box extend of the preview mesh.
	if (!bSocketsFound)
	{
		TraceStartPoints.Empty();
		FVector BoxExtent = PreviewMesh->GetStaticMesh()->GetBounds().GetBox().GetExtent();
		AddBoxExtentToTracePoints(TraceStartPoints, Location, BoxExtent);
	}
	else
	{
		// Add pivot location as the first trace point
		TraceStartPoints.Insert(Location + FVector(0.0f, 0.0f, 100.0f), 0);
	}

	bool bAllPointsValid = true;
	float SlopeAngle = 0;
	constexpr float EndHeightDifference = 2*DeveloperSettings::GamePlay::Construction::AddedHeightToTraceSlopeCheckPoint;
	for (const FVector& StartPoint : TraceStartPoints)
	{
		FHitResult Hit;
		// Adjust height for added z above the original point.
		FVector EndPoint = StartPoint - FVector(0.0f, 0.0f, EndHeightDifference);
		
		if (GetWorld()->LineTraceSingleByChannel(Hit, StartPoint, EndPoint, ECC_Visibility))
		{
			SlopeAngle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(Hit.Normal, FVector::UpVector)));
			if (DeveloperSettings::Debugging::GConstruction_Preview_Compile_DebugSymbols)
			{
				RTSFunctionLibrary::PrintString("Hit angle at (" + StartPoint.ToString() + "): " + FString::SanitizeFloat(SlopeAngle), FColor::Red);
			}
			
			if (SlopeAngle > DeveloperSettings::GamePlay::Construction::DegreesAllowedOnHill)
			{
				bAllPointsValid = false;
				M_SlopeAngle = SlopeAngle;
			}
		}
		else
		{
			// If any trace doesn't hit, consider it invalid
			bAllPointsValid = false;
		}
	}

	if (bAllPointsValid)
	{
		// All valid; take the last slope as the comparison.
		M_SlopeAngle = SlopeAngle;
	}

	return bAllPointsValid;
}

void ACPPConstructionPreview::AddBoxExtentToTracePoints(
	TArray<FVector>& OutTracePoints,
	const FVector& Location,
	const FVector& BoxExtent) const
{
	constexpr float Height = DeveloperSettings::GamePlay::Construction::AddedHeightToTraceSlopeCheckPoint;
	// Add pivot location
	OutTracePoints.Add(Location + FVector(0.0f, 0.0f,Height));
	// Add the box extend points
	OutTracePoints.Add(Location + FVector(BoxExtent.X, BoxExtent.Y, Height));
	OutTracePoints.Add(Location + FVector(-BoxExtent.X, BoxExtent.Y, Height));
	OutTracePoints.Add(Location + FVector(BoxExtent.X, -BoxExtent.Y, Height));
	OutTracePoints.Add(Location + FVector(-BoxExtent.X, -BoxExtent.Y, Height));
}


void ACPPConstructionPreview::InitializeDynamicMaterialPool(UMaterialInstance* BaseMaterial)
{
	for (int i = 0; i < DeveloperSettings::GamePlay::Construction::MaxNumberMaterialsOnPreviewMesh; ++i)
	{
		UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		M_DynamicMaterialPool.Add(DynMaterial);
	}
}

void ACPPConstructionPreview::UpdatePreviewMaterial(bool bIsValidLocation)
{
	for (UMaterialInstanceDynamic* DynMaterial : M_DynamicMaterialPool)
	{
		if (DynMaterial)
		{
			DynMaterial->SetScalarParameterValue(FName("PlacementOkay"), bIsValidLocation);
		}
	}
}

void ACPPConstructionPreview::MoveWidgetToMeshHeight() const
{
	// Calculate the position for the widget to be 100 units above the mesh.
	FVector MeshBoundsOrigin, MeshBoundsExtent;
	PreviewMesh->GetStaticMesh()->GetBounds().GetBox().GetCenterAndExtents(MeshBoundsOrigin,
	                                                                       MeshBoundsExtent);
	const float WidgetHeightAboveMesh = MeshBoundsExtent.Z + 100.0f;
	const FVector WidgetWorldPosition = PreviewMesh->GetComponentLocation() +
		FVector(0.0f, 0.0f, WidgetHeightAboveMesh);
	M_PreviewStatsWidgetComponent->SetWorldLocation(WidgetWorldPosition);
}


void ACPPConstructionPreview::RotatePreviewClockwise() const
{
	if (bM_BHasActivePreview)
	{
		FRotator CurrentRotation = PreviewMesh->GetComponentRotation();
		CurrentRotation.Yaw += RotationDegrees;
		PreviewMesh->SetWorldRotation(CurrentRotation);
	}
}

void ACPPConstructionPreview::RotatePreviewCounterclockwise() const
{
	if (bM_BHasActivePreview)
	{
		FRotator CurrentRotation = PreviewMesh->GetComponentRotation();
		CurrentRotation.Yaw -= RotationDegrees;
		PreviewMesh->SetWorldRotation(CurrentRotation);
	}
}

FRotator ACPPConstructionPreview::GetPreviewRotation() const
{
	return PreviewMesh->GetComponentToWorld().Rotator();
}
