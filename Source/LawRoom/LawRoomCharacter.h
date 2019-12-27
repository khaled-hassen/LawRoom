// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LawRoomCharacter.generated.h"

UCLASS(config=Game)
class ALawRoomCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	// the initial follow camera relative (to camera boom) transform, it's needed to change follow camera back to its location and rotation
	FTransform OldCameraRelativeTransform;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	// Room ability
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	class URoomAbilityComponent* RoomAbilityComponent;

protected:

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void Turn(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAt(float Rate);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	ALawRoomCharacter();

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintImplementableEvent)
    // starts injection shot animation and attack
	void StartInjectionShot();

	UFUNCTION(BlueprintCallable)
	// changes player camera to naniCamera then InjectionCamera and performs Injection shot attack
	void ChangeCameraAndAttack();

	// returns if the camera has been changed successfully and change the camera
	bool ChangeToNaniCamera();

	UFUNCTION()
	// after the change is done perform injection shot attack
	void ChangeToFollowCamera();
};