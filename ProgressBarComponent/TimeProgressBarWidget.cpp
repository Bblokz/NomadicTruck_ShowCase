// Copyright Bas Blokzijl - All rights reserved.
#include "TimeProgressBarWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "RTS_Survival/DeveloperSettings.h"
#include "RTS_Survival/Utils/HFunctionLibary.h"


void UTimeProgressBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UTimeProgressBarWidget::InitTimeProgressComponent(
	UStaticMeshComponent* NewCameraSphere,
	UMeshComponent* NewBarAsMeshRef)
{
	M_CameraSphere = NewCameraSphere;
	M_ProgressBar->SetVisibility(ESlateVisibility::Hidden);
	M_ProgressBar->SetPercent(0.0f);
	M_ProgressText->SetVisibility(ESlateVisibility::Hidden);
	M_BarRotationDel.BindUObject(this, &UTimeProgressBarWidget::OrientProgressBar);
	M_BarAsMeshRef = NewBarAsMeshRef;
}

void UTimeProgressBarWidget::StartProgressBar(float Time)
{
	M_TotalTime = Time;
	M_ProgressBar->SetVisibility(ESlateVisibility::Visible);
	M_ProgressBar->SetPercent(0.0f);
	M_ProgressText->SetVisibility(ESlateVisibility::Visible);

	M_World = GetWorld();
	if(M_World)
	{
		M_StartTime = M_World->GetTimeSeconds();
		M_World->GetTimerManager().SetTimer(M_ProgressUpdateHandle, this, &UTimeProgressBarWidget::UpdateProgressBar,
			DeveloperSettings::Optimisation::UpdateIntervalProgressBar, true);
		M_World->GetTimerManager().SetTimer(M_BarRotationHandle, M_BarRotationDel,
	DeveloperSettings::Optimisation::OrientProgressBarInterval, true);
	}
}

void UTimeProgressBarWidget::StopProgressBar()
{
	if (M_World)
	{
		M_World->GetTimerManager().ClearTimer(M_ProgressUpdateHandle);
		M_World->GetTimerManager().ClearTimer(M_BarRotationHandle);
	}
	else
	{
		M_World = GetWorld();
		if(M_World)
		{
			M_World->GetTimerManager().ClearTimer(M_ProgressUpdateHandle);
			M_World->GetTimerManager().ClearTimer(M_BarRotationHandle);
		}
	}
	M_ProgressBar->SetVisibility(ESlateVisibility::Hidden);
	M_ProgressText->SetVisibility(ESlateVisibility::Hidden);
}

float UTimeProgressBarWidget::GetTimeElapsed() const
{
	if(M_World)
	{
		return M_World->GetTimeSeconds() - M_StartTime;
	}
	return 0.0f;
}

void UTimeProgressBarWidget::UpdateProgressBar()
{
	if(M_World)
	{
		// Calculate elapsed time and progress.
		const float CurrentTime = M_World->GetTimeSeconds() - M_StartTime;
		const float Progress = FMath::Clamp(CurrentTime / M_TotalTime, 0.0f, 1.0f);
		M_ProgressBar->SetPercent(Progress);

		// Update the progress text
		const int32 Percentage = FMath::RoundToInt(Progress * 100);
		M_ProgressText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Percentage)));

		if (CurrentTime >= M_TotalTime)
		{
			StopProgressBar();
		}
	}

}

void UTimeProgressBarWidget::OrientProgressBar()
{
	FRotator BarRotation(0.0f, 180.f, 0.0f);
	const FRotator CamRotation = M_CameraSphere->GetRelativeRotation();
	BarRotation.Add(CamRotation.Pitch, CamRotation.Yaw, CamRotation.Roll);
	M_BarAsMeshRef->SetWorldRotation(BarRotation);
	
}
