// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "LawRoomCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "RoomAbilityComponent.h"
#include "Enemy.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

ALawRoomCharacter::ALawRoomCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	
	RoomAbilityComponent = CreateDefaultSubobject<URoomAbilityComponent>("RoomAbilityComponent");
}

//////////////////////////////////////////////////////////////////////////
// Input
void ALawRoomCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ALawRoomCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ALawRoomCharacter::MoveRight);

	// "turn" handles devices that provide an absolute delta, such as a mouse.
	PlayerInputComponent->BindAxis("Turn", this, &ALawRoomCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ALawRoomCharacter::LookUpAt);

	// Room abilities bindings
	PlayerInputComponent->BindAction("SpawnRoom", IE_Pressed, RoomAbilityComponent, &URoomAbilityComponent::CreateRoom);
	PlayerInputComponent->BindAction("LockOn", IE_Pressed, RoomAbilityComponent, &URoomAbilityComponent::LockOnTarget);
	PlayerInputComponent->BindAxis("ChangeTarget", RoomAbilityComponent, &URoomAbilityComponent::ChangeTarget);
	PlayerInputComponent->BindAction("InjectionShot", IE_Pressed, RoomAbilityComponent, &URoomAbilityComponent::RequestInjectionShot);
}

void ALawRoomCharacter::Turn(float Rate)
{
	if (!RoomAbilityComponent) { return; }

	if (!RoomAbilityComponent->GetIsFocused())
	{
		AddControllerYawInput(Rate);
	}
}

void ALawRoomCharacter::LookUpAt(float Rate)
{
	if (!RoomAbilityComponent) { return; }

	if (!RoomAbilityComponent->GetIsFocused())
	{
		AddControllerPitchInput(Rate);
	}
	else
	{
		RoomAbilityComponent->LookAtEnemy();
	}
}

void ALawRoomCharacter::BeginPlay()
{
	Super::BeginPlay();

	// ensures that these sounds has been set in LawRoomCharacter blueprint class defaults
	ensure(NaniSound);
	ensure(OmaeWaMouShindeiruSound);
	ensure(AimingSound);
	ensure(ShotSound);
}

void ALawRoomCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ALawRoomCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ALawRoomCharacter::ChangeCameraAndAttack()
{
	if (RoomAbilityComponent->GetLockedOnEnemy())
	{
		RoomAbilityComponent->GetLockedOnEnemy()->LookAt(this);

		FTimerDelegate CameraTimer;
		CameraTimer.BindLambda([&]()
		{
			if (ChangeToNaniCamera()) //  if the camera changed to nani camera (there is a FocusEnemy) the change to InjectionCamera
			{
				float Duration = NaniSound->GetDuration();

				FTimerDelegate AimingTimer;
				AimingTimer.BindLambda([&]()
				{
					float Duration = AimingSound->GetDuration();

					UGameplayStatics::PlaySound2D(GetWorld(), AimingSound);
					RoomAbilityComponent->GetLockedOnEnemy()->MoveCrosshair(Duration);

					FTimerHandle TimerHandle;
					// changes back to follow camera
					GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ALawRoomCharacter::ChangeToFollowCamera, Duration, false);
				});

				FTimerHandle TimerHandle;
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, AimingTimer, Duration, false);
			}
		});

		float Duration = OmaeWaMouShindeiruSound->GetDuration();
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, CameraTimer, Duration, false);

		UGameplayStatics::PlaySound2D(GetWorld(), OmaeWaMouShindeiruSound);
	}
}

bool ALawRoomCharacter::ChangeToNaniCamera()
{
	if (RoomAbilityComponent)
	{
		if (RoomAbilityComponent->GetLockedOnEnemy())
		{
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			PlayerController->SetViewTargetWithBlend(RoomAbilityComponent->GetLockedOnEnemy(), 0.2f, EViewTargetBlendFunction::VTBlend_Cubic);

			UGameplayStatics::PlaySound2D(GetWorld(), NaniSound);

			return true;
		}
	}

	return false;
}

void ALawRoomCharacter::ChangeToFollowCamera()
{
	FollowCamera->SetRelativeTransform(OldCameraRelativeTransform);

	float BlendTime = 0.5f;
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	PlayerController->SetViewTargetWithBlend(this, BlendTime, EViewTargetBlendFunction::VTBlend_Cubic);

	FTimerDelegate ShootingTimer;
	ShootingTimer.BindLambda([&]()
	{
			UGameplayStatics::PlaySound2D(GetWorld(), ShotSound);
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			RoomAbilityComponent->InjectionShot();
	});

	// perform injection attack when the camera has changed
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, ShootingTimer, BlendTime, false);
}