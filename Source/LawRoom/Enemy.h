// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enemy.generated.h"

UCLASS()
class LAWROOM_API AEnemy : public ACharacter
{
	GENERATED_BODY()

private:
	// is the enemy dead or not
	bool bIsDead = false;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	// the path that the crosshair follows when aims at the enemy : it is set in Enemy bp construction script
	class USplineComponent* CrosshairPath;

	UPROPERTY(VisibleAnywhere)
	class UWidgetComponent* Crosshair;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Sets default values for this character's properties
	AEnemy();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE bool GetIsDead() const { return bIsDead; }
	FORCEINLINE void SetIsDead(bool Value) { bIsDead = Value; }

	void MoveCrosshair(float Duration);
	void LookAt(AActor* Player);
};
