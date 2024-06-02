// Copyright Bas Blokzijl - All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RTS_Survival/MasterObjects/ActorObjectsMaster.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"


#include "CPPConstructionPreview.generated.h"

class UW_PreviewStats;
class RTS_SURVIVAL_API ACPPController;
// Forward declarations.
class RTS_SURVIVAL_API AStaticPreviewMesh;

/**
 * @brief The construction preview of a building that is moved along a grid at the cursor location.
 * Start and stop the preview with StartBuildingPreview and StopBuildingPreview.
 * Needs to be initialized with InitConstructionPreview.
 * On the second click of building a static preview mesh is spawned which is saved at the constructor unit.
 */
UCLASS()
class RTS_SURVIVAL_API ACPPConstructionPreview : public AActorObjectsMaster
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACPPConstructionPreview();

	/** @return If the building preview is overlapping with something. */
	bool GetIsBuildingPreviewBlocked() const;

	AStaticPreviewMesh* CreateStaticMeshActor(const FRotator& Rotation) const;

	/**
	 * @brief Starts the static mesh preview of the building of the provided mesh.
	 * @param NewPreviewMesh The mesh that will be displayed.
	 * @param HostLocation The location of the expanding building that wants to place an expansion.
	 * @param BuildRadius How far the building can be placed from the host location.
	 * @note Do not call directly but use the CppController::StartBuildingPreview.
	 */
	UFUNCTION(BlueprintCallable)
	void StartBuildingPreview(
		UStaticMesh* NewPreviewMesh,
		const FVector HostLocation = FVector::ZeroVector,
		const float BuildRadius = 0);

	inline UStaticMesh* GetPreviewMesh() const { return PreviewMesh->GetStaticMesh(); }

	/**
	 * @brief Stops the static mesh preview of the building by setting the preview mesh to nullptr.
	 * And letting the tick function know that there is no active preview.
	 */
	UFUNCTION(BlueprintCallable)
	void StopBuildingPreview();

	/**
	 * @brief Rotates the building preview clockwise by m_RotationDegrees.
	 *
	 * This function updates the rotation of the preview mesh, rotating it clockwise around its up axis
	 * by the angle specified in m_RotationDegrees.
	 */
	UFUNCTION()
	void RotatePreviewClockwise() const;

	/**
	 * @brief Rotates the building preview counterclockwise by m_RotationDegrees.
	 *
	 * This function updates the rotation of the preview mesh, rotating it counterclockwise around its up axis
	 * by the angle specified in m_RotationDegrees.
	 */
	UFUNCTION()
	void RotatePreviewCounterclockwise() const;

	FRotator GetPreviewRotation() const;

protected:
	/**
	 * Call in begin play.
	 * @param NewPlayerController The playercontroller.
	 * @param ConstructionMaterial The material to use for the preview mesh.
	 * @param PreviewStatsWidget The widget that shows building instructions and current stats of the construction placement.
	 * @param PreviewStatsWidgetComponent The widget component that shows the preview stats widget.
	 */
	UFUNCTION(BlueprintCallable, Category="ReferenceCasts")
	void InitConstructionPreview(
		ACPPController* NewPlayerController,
		UMaterialInstance* ConstructionMaterial,
		UW_PreviewStats* PreviewStatsWidget,
		UWidgetComponent* PreviewStatsWidgetComponent);

	UPROPERTY(BlueprintReadOnly, Category="Reference")
	ACPPController* PlayerController;

	/**
	 * Calculates the mouse position on the landscape and snaps it to the grid if there is an active preview.
	 * Displays the preview mesh at the cursor location with the valid or invalid material depending on the overlap.
	 * @param DeltaTime 
	 */
	virtual void Tick(float DeltaTime) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Name for the preview mesh component.
	static FName PreviewMeshComponentName;

	/** The static mesh on which the preview is placed. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient)
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY(BlueprintReadOnly)
	FVector CursorWorldPosition;

	// Degrees to rotate the building preview
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Building Rotation")
	float RotationDegrees;

private:
	FVector GetGridSnapAdjusted(
		const FVector& MouseWorldPosition,
		const float ExtraHeight = 0) const;

	// Whether there is a preview active.
	bool bM_BHasActivePreview;

	/** @return Whether the construction preview overlaps with something. */
	bool IsOverlapping() const;

	// The Displayed building Material.
	// Safe non-owning reference using GC system.
	UPROPERTY()
	UMaterialInstance* M_ConstructionPreviewMaterial;

	bool bM_IsValidCursorLocation;

	bool bM_IsValidBuildingLocation;

	// Location of the expanding building that wants to place an expansion.
	FVector M_HostLocation;

	// How far the building can be placed from the host location.
	float M_BuildRadius;

	// The angle in degrees that the slope of the landscape has to the preview mesh.
	float M_SlopeAngle;

	// Widget that shows building instructions and current stats of the construction placement.
	UPROPERTY()
	UW_PreviewStats* M_PreviewStatsWidget;

	// Reference to the stats widget as component.
	UPROPERTY()
	UWidgetComponent* M_PreviewStatsWidgetComponent;

	/**
	 * @brief Updates the preview widget with the data of the preview.
	 * @param bIsInclineValid Whether the preview's incline results in valid placement.
	 */
	void UpdatePreviewStatsWidget(const bool bIsInclineValid) const;

	void RotatePreviewStatsToCamera() const;

	/** @return Whether the location is within the build radius of the host location. */
	bool IsWithinBuildRadius(const FVector& Location) const;

	/**
	 * @brief Checks if the slope at a given location is within the allowed angle for building placement.
	 *
	 * This function performs a line trace at the specified location to determine the surface normal.
	 * It then calculates the angle between this normal and the world's up vector. If the angle is less than
	 * or equal to m_DegreesAllowedOnHill, the function returns true, indicating the slope is valid for building.
	 *
	 * @param Location The world location where the slope's validity is to be checked.
	 * @return bool True if the slope angle at the location is within the allowed limit, false otherwise.
	 *
	 * @note Attemps to trace from the preview mesh at the predetermined socket points ["FL", "FR", "RL", "RR"].
	 * If these are not found we revert back to the box extend of the preview mesh.
	 */
	bool IsSlopeValid(const FVector& Location);

	/**
	 * @brief Adds the box extent to the provided trace points.
	 * @param OutTracePoints The collection of trace points to add the box extent to.
	 * @param Location The pivot location.
	 * @param BoxExtent The box extend of the preview mesh.
	 */
	void AddBoxExtentToTracePoints(
		TArray<FVector>& OutTracePoints,
		const FVector& Location,
		const FVector& BoxExtent) const;

	// Pool to store dynamic material instances for each material slot.
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> M_DynamicMaterialPool;

	/**
	 * @brief Initializes the dynamic material pool with the provided base material.
	 * @param BaseMaterial The base material to use for the dynamic material instances.
	 */
	void InitializeDynamicMaterialPool(UMaterialInstance* BaseMaterial);

	/**
	 * @brief Updates the material of the preview mesh to the valid or invalid material.
	 * @param bIsValidLocation Whether the location is valid.
	 */
	void UpdatePreviewMaterial(const bool bIsValidLocation);

	/** Moves the construction widget to the height of the preview mesh. */
	void MoveWidgetToMeshHeight() const;

	/**
	 * @brief Sets the Cursor location.
	 * @param CursorLocation The to landscape snapped position of the cursor.
	 */
	void SetCursorPosition(const FVector& CursorLocation);
};
