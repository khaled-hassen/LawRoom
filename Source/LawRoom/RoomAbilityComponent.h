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
	class AEnemy* FocusEnemy = nullptr;

	// player character: owner
	class ALawRoomCharacter* Player = nullptr;

	/// sound effects
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	class USoundWave* OmaeWaMouShindeiruSound;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	class USoundWave* NaniSound;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	class USoundWave* AimingSound;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	class USoundWave* ShotSound;
	///
private:
	// calculate the distance between the enemy and the FocusedActor or the player. Default is the player when target == nullptr
	float CalculateDistance(class AEnemy* Enemy, AActor* Target = nullptr) const;

	// calculate the angle between the enemy and the player.
	float CalculateAngle(class AEnemy* Enemy) const;

	// checks if the player is in the room to enable him to use his abilities
	bool CheckPlayerInsideRoom(class ALawRoomCharacter* Player) const;

	// used with the sort method
	int32 Partition(TArray<class AEnemy*>& Enemies, int32 Start, int32 End);
	// used with the Partition method : swap two enemies array elements
	void Swap(AEnemy*& Enemy1, AEnemy*& Enemy2);

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

	/** wait for the room to complete spawning the update color every frame
	 * @param SpawnTime: the time when the room starts spawning
	 * @param SpawnDuration: the duration that takes to spawn the room*/
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

	UFUNCTION(BlueprintCallable)
	// Get the closest enemy to the player (using angles not distance)
	class AEnemy* GetClosestEnemy() const;

	FORCEINLINE class AEnemy* GetFocusEnemy() const { return FocusEnemy; }
	FORCEINLINE bool GetIsFocused() const { return bIsFocused; }

	// focus only when the player is in the room
	void FocusOnTarget();
	// LookAtEnemy only when the player is in the room
	void LookAtEnemy();

	/** sort Enemies array by distance(between TheClosestEnemy and the Enemies elements) ascending
	 * using quickSort for info link : https://en.wikipedia.org/wiki/Quicksort and https://www.geeksforgeeks.org/cpp-program-for-quicksort/*/
	void Sort(TArray<class AEnemy*>& Enemies, int32 Start, int32 End);

	/** change the targeted enemy changes the FocusEnemy to the closest enemy (to FocusEnemy)
	 * so the Enemies array needs to be sorted by distance
	 * @param Value : 1 means forward / -1 means backward
	 * for example if the enemies array contains 3 enemies and the index of the target is 1
	 * then (value = 1) the next target index  is 2
	 * (value = -1)  the next target index is 0
	 * if the index is negative :
	 * index -1 is the same as 2 / index -2 is the same as 1 / index -3 is the same as 0 */
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

	FORCEINLINE class USoundWave* GetOmaeSound() const { return OmaeWaMouShindeiruSound; }
	FORCEINLINE class USoundWave* GetNaniSound() const { return NaniSound; }
	FORCEINLINE class USoundWave* GetAimingSound() const { return AimingSound; }
	FORCEINLINE class USoundWave* GetShotSound() const { return ShotSound; }
};
