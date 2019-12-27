// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"
#include "Components/SplineComponent.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Crosshair = CreateDefaultSubobject<UWidgetComponent>("Crosshair");
	Crosshair->bVisible = false;
	Crosshair->SetWidgetSpace(EWidgetSpace::Screen);
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	// ensures that the crosshair path is set  
	ensure(CrosshairPath);

	if (CrosshairPath)
	{
		FVector InitLocation = CrosshairPath->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		Crosshair->SetWorldLocation(InitLocation);
	}
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemy::MoveCrosshair(float Duration)
{
	if (CrosshairPath)
	{
		FVector InitLocation = CrosshairPath->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		Crosshair->SetWorldLocation(InitLocation);
	}

	float StartTime = GetWorld()->GetTimeSeconds();

	FTimerHandle TimerHandle;
	FTimerDelegate CrosshairMoveDel;
	CrosshairMoveDel.BindLambda([=, &TimerHandle]
	{
		if (CrosshairPath)
		{
			float MoveRate = FMath::Clamp<float>((GetWorld()->GetTimeSeconds() - StartTime) / Duration, 0.f, 1.f);	
			float PathLength = CrosshairPath->GetSplineLength();
			FVector NewLocation = CrosshairPath->GetLocationAtDistanceAlongSpline(MoveRate * PathLength, ESplineCoordinateSpace::World);
			Crosshair->SetWorldLocation(NewLocation);

			if (FMath::IsNearlyEqual(MoveRate, 1.f)) // that means move time has passed the move duration
			{
				Crosshair->SetVisibility(false);
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
			}
		}
	});

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, CrosshairMoveDel, GetWorld()->GetDeltaSeconds(), true);
	Crosshair->SetVisibility(true);
}

void AEnemy::LookAt(AActor* Player)
{
	FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(this->GetActorLocation(), Player->GetActorLocation());
	SetActorRotation(NewRotation);
}

