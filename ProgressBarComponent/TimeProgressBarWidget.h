// Copyright Bas Blokzijl - All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#include "TimeProgressBarWidget.generated.h"

/**
 * @class UTimeProgressBarWidget
 * 
 * @brief A widget component to display progress over time with an optional text display of progress percentage.
 * the m_ProgressText and m_ProgressBar are bound to the respective widgets in the UMG editor automatically
 * by giving the elements the same name as the variables.
 */
UCLASS()
class RTS_SURVIVAL_API UTimeProgressBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief Initializes the time progress component with the camera sphere to orient the progress bar towards.
	 * @param NewCameraSphere The camera sphere to orient the progress bar towards.
	 * @param NewBarAsMeshRef The reference to the bar widget as component on the owner of the TimeProgressBarWidget.
	 */
	UFUNCTION(BlueprintCallable, Category= "Initialization")
	void InitTimeProgressComponent(UStaticMeshComponent* NewCameraSphere,
		UMeshComponent* NewBarAsMeshRef);

	/** Starts the progress bar for the specified duration. */
	UFUNCTION(BlueprintCallable, Category= "Progress Bar Control")
	void StartProgressBar(float Time);

	/** Stops the progress bar and hides it. */
	UFUNCTION(BlueprintCallable, Category= "Progress Bar Control")
	void StopProgressBar();

	/** @return How long the progressbar has been running. */
	float GetTimeElapsed() const;

	/** @brief Sets the local location of the progress bar. */
	inline void SetLocalLocation(const FVector& NewLocation) const {M_BarAsMeshRef->SetRelativeLocation(NewLocation);};

	inline FVector GetLocalLocation() const {return M_BarAsMeshRef->GetRelativeLocation();};


protected:
	// Called when the game starts or the widget is created
	virtual void NativeConstruct() override;

private:
	// Bound automatically to the UMG widget by name.
	UPROPERTY(meta = (BindWidget))
	UProgressBar* M_ProgressBar;

	// Bound automatically to the UMG widget by name.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* M_ProgressText;
	
	// Camera component to orient the progress bar towards
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Settings", meta=(AllowPrivateAccess="true"))
	UStaticMeshComponent* M_CameraSphere;

	// Total time for the progress bar to complete
	float M_TotalTime;

	// Timer handle for progress bar update
	FTimerHandle M_ProgressUpdateHandle;

	// To update the progress bar's fill percentage and text display
	void UpdateProgressBar();

	UPROPERTY()
	// World spawned in
	UWorld* M_World;

	float M_StartTime;

	/** @brief Orients the progressbar to the Player controller. */
	void OrientProgressBar();
	
	// Used to set timer for the rotation of the healthBar.
	FTimerHandle M_BarRotationHandle;
	// Used to set timer for the rotation of the healthBar.
	FTimerDelegate M_BarRotationDel;

	// Reference to the bar widget as component on the owner of the TimeProgressBarWidget.
	// This is used for rotation adjustments.
	UPROPERTY()
	UMeshComponent* M_BarAsMeshRef;
};
