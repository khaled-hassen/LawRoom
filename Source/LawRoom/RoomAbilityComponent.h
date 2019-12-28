// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RoomAbilityComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LAWROOM_API URoomAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UPROPERTY()
	UStaticMeshComponent* Katana = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	UAnimMontage* RoomSpawnAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	UAnimMontage* InjectionShotAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	class UStaticMesh* RoomMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	class UMaterialInterface* RoomMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	//Room radius in meter
	float RoomRadius = 15.f;

	UPROPERTY()
	UStaticMeshComponent* Room = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Setup")
	// RoomLifeSpan in seconds, it is set by the RoomColorCurve's max time value. Default is 10 seconds
	float RoomLifeSpan;

	// to prevent spamming room spawning
	bool bCanCreateRoom = true;

	bool bIsInjectionShot = false;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	UCurveFloat* SpawnTimeCurve = nullptr;

	UPROPERTY()
	class UTimelineComponent* UpdateColorTimeline = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	UCurveFloat* RoomColorCurve = nullptr;

	UPROPERTY()
	FLinearColor RoomBaseColor;

	UPROPERTY()
	class UTimelineComponent* SpawnRoomTimeline = nullptr;

	// prevents room from being destroyed infinitively
	bool bIsCreatingRoom = false;

	UPROPERTY()
	TArray<class AEnemy*> Enemies;

	// Toggle focus on and off
	bool bIsFocused = false;

	// the targeted enemy
	class AEnemy* LockedOnEnemy = nullptr;

	// player character: owner
	class ALawRoomCharacter* Player = nullptr;
	
private:
	// calculate the distance between the player and the LockedOnEnemy
	float CalculateDistance(class AEnemy* Enemy) const;

	// checks if the player is in the room to enable him to use his abilities
	bool CheckPlayerInsideRoom(class ALawRoomCharacter* Player) const;

	// Get the closest visible enemy to the player
	class AEnemy* GetClosestEnemy() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Sets default values for this component's properties
	URoomAbilityComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Setup")
	// setup room ability dependencies
	void SetupPlayerKatana(UStaticMeshComponent* PlayerKatana);

	void CreateRoom();

	UFUNCTION()
	void SpawnRoom(float Alpha);

	UFUNCTION(BlueprintCallable)
	void SetRoomSpawnLocation(const FVector& SpawnLocation);

	UFUNCTION()
	void ChangeColor();

	// wait for the room to complete spawning then update color every frame
	UFUNCTION()
	void UpdateRoomColor(float Alpha);

	// returns a dynamic Room material to be able change the color of the room over time
	class UMaterialInstanceDynamic* GetRoomDynamicMaterial() const;

	UFUNCTION()
	void DestroyRoom();

	UFUNCTION()
	void OnRoomDetectedEnemy(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnKatanaCollidedWithEnemy(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	FORCEINLINE class AEnemy* GetLockedOnEnemy() const { return LockedOnEnemy; }
	FORCEINLINE bool GetIsFocused() const { return bIsFocused; }

	// focus only when the player is in the room
	void LockOnTarget();
	// LookAtEnemy only when the player is in the room
	void LookAtEnemy();

	// change the Lock on enemy
	void ChangeTarget(float Value);

	// check if the player can perform injection shot attack  if true calls the StartInjectionShot method from LawRoomCharacter
	void RequestInjectionShot();

	UFUNCTION(BlueprintCallable)
	void InjectionShot();

	// updates enemy death state and collision also enables rag doll death
	void UpdateEnemyStatus(AEnemy* Enemy);

	// used in animation blueprint
	UFUNCTION(BlueprintCallable)
	FORCEINLINE class UTimelineComponent* GetSpawnRoomTimeline() const { return SpawnRoomTimeline; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetIsInjectionShot() const { return bIsInjectionShot; }

	UFUNCTION(BlueprintCallable)
	void SetIsInjectionShot(bool Value) { bIsInjectionShot = Value; }

};
