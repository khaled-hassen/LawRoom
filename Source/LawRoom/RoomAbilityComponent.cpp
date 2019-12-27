// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomAbilityComponent.h"
#include "Components/TimelineComponent.h"
#include "Enemy.h"
#include "LawRoomCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values for this component's properties
URoomAbilityComponent::URoomAbilityComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// setup SpawnRoomTimeline
	SpawnRoomTimeline = CreateDefaultSubobject<UTimelineComponent>("SpawnRoomTimeline");
	SpawnRoomTimeline->SetLooping(false);
	SpawnRoomTimeline->SetIgnoreTimeDilation(true);

	// setup UpdateColorTimeline
	UpdateColorTimeline = CreateDefaultSubobject<UTimelineComponent>("UpdateColorTimeline");
	UpdateColorTimeline->SetLooping(false);
	UpdateColorTimeline->SetIgnoreTimeDilation(true);

	Room = CreateDefaultSubobject<UStaticMeshComponent>("Room");
}

// Called when the game starts
void URoomAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	Player = Cast<ALawRoomCharacter>(GetOwner());

	// ensures that these sounds has been set in RoomAbilityComponent
	ensure(NaniSound);
	ensure(OmaeWaMouShindeiruSound);
	ensure(AimingSound);
	ensure(ShotSound);

	// room setup
	if (ensure(RoomMesh) && ensure(RoomMaterial))
	{
		// setup room mesh and material
		Room->SetStaticMesh(RoomMesh);
		Room->SetMaterial(0, RoomMaterial);

		// setup room collision 
		Room->SetCollisionProfileName("OverlapAll");
		Room->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Room->SetWorldScale3D(FVector::ZeroVector);
		Room->SetGenerateOverlapEvents(true);
		Room->OnComponentBeginOverlap.AddDynamic(this, &URoomAbilityComponent::OnRoomDetectedEnemy);

		// setup RoomBaseColor
		GetRoomDynamicMaterial()->GetVectorParameterValue(FMaterialParameterInfo("BaseColor"), RoomBaseColor);
	}
}

// Called every frame
void URoomAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void URoomAbilityComponent::SetupPlayerKatana(UStaticMeshComponent* PlayerKatana)
{
	if (ensure(PlayerKatana))
	{
		Katana = PlayerKatana;

		// setup katana collision 
		Katana->SetGenerateOverlapEvents(true);
		Katana->SetCollisionProfileName("OverlapAll");
		Katana->SetCollisionObjectType(ECC_WorldDynamic);
		Katana->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Katana->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Katana->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Katana->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

		//
		Katana->OnComponentBeginOverlap.AddDynamic(this, &URoomAbilityComponent::OnKatanaCollidedWithEnemy);
	}
}

void URoomAbilityComponent::CreateRoom()
{
	if (ensure(SpawnTimeCurve) && bCanCreateRoom && ensure(RoomSpawnAnim) && Player && ensure(Room) && ensure(RoomColorCurve))
	{
		Player->GetCharacterMovement()->DisableMovement();

		// reset room base color
		if (GetRoomDynamicMaterial())
		{
			// reset room color back to origin
			GetRoomDynamicMaterial()->SetVectorParameterValue("BaseColor", RoomBaseColor);
		}

		bIsCreatingRoom = true;

		// set RoomLifeSpan
		float Min;
		RoomColorCurve->GetTimeRange(Min, RoomLifeSpan);

		FOnTimelineFloat SpawnRoomDelegate;
		SpawnRoomDelegate.BindUFunction(this, "SpawnRoom");
		
		FOnTimelineEventStatic OnRoomFinishedSpawn;
		OnRoomFinishedSpawn.BindUFunction(this, "ChangeColor");
		SpawnRoomTimeline->SetTimelineFinishedFunc(OnRoomFinishedSpawn);

		float RoomSpawnDuration;
		SpawnTimeCurve->GetTimeRange(Min, RoomSpawnDuration);
		SpawnRoomTimeline->SetTimelineLength(RoomSpawnDuration);
		SpawnRoomTimeline->AddInterpFloat(SpawnTimeCurve, SpawnRoomDelegate, "Value");
	
		Player->PlayAnimMontage(RoomSpawnAnim);
		//SpawnRoomTimeline->PlayFromStart(); it will be called by an anim notify

		bCanCreateRoom = false;

		Room->DetachFromParent(true);
		Room->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void URoomAbilityComponent::SpawnRoom(float Alpha)
{
	float NewRadius = FMath::Lerp<float>(0.f, RoomRadius, Alpha);
	Room->SetWorldScale3D(UKismetMathLibrary::Conv_FloatToVector(NewRadius));
}

void URoomAbilityComponent::SetRoomSpawnLocation(const FVector& SpawnLocation)
{
	if (Room)
	{
		Room->SetWorldLocation(SpawnLocation);
	}
}

void URoomAbilityComponent::ChangeColor()
{
	if (ensure(RoomColorCurve) && bIsCreatingRoom)
	{
		FOnTimelineEventStatic DestroyRoomDelegate;
		DestroyRoomDelegate.BindUFunction(this, "DestroyRoom");
		UpdateColorTimeline->SetTimelineFinishedFunc(DestroyRoomDelegate);

		FOnTimelineFloat UpdateColorDelegate;
		UpdateColorDelegate.BindUFunction(this, "UpdateRoomColor");

		UpdateColorTimeline->SetTimelineLength(RoomLifeSpan);
		UpdateColorTimeline->AddInterpFloat(RoomColorCurve, UpdateColorDelegate, "Alpha");

		UpdateColorTimeline->PlayFromStart();
	}
}

void URoomAbilityComponent::UpdateRoomColor(float Alpha)
{
	if (GetRoomDynamicMaterial())
	{
		FLinearColor LifeEndColor = UKismetMathLibrary::LinearColorLerp(RoomBaseColor, FLinearColor::Red, Alpha);
		GetRoomDynamicMaterial()->SetVectorParameterValue("BaseColor", LifeEndColor);	
	}
}

UMaterialInstanceDynamic* URoomAbilityComponent::GetRoomDynamicMaterial() const
{
	if (Room)
	{
		// create a dynamic material to change the color of the room over time
		FName MaterialSlotName = Room->GetMaterialSlotNames()[0];
		int32 MaterialIndex = Room->GetMaterialIndex(MaterialSlotName);
		UMaterialInstanceDynamic* DynamicMat = Room->CreateDynamicMaterialInstance(MaterialIndex, Room->GetMaterial(MaterialIndex));

		return DynamicMat;
	}

	return nullptr;
}

void URoomAbilityComponent::DestroyRoom()
{
	if (Room)
	{
		SpawnRoomTimeline->ReverseFromEnd();
		Room->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	Enemies.Empty();
	bIsFocused = false;
	bCanCreateRoom = true;
	bIsCreatingRoom = false;

	if (Player)
	{
		Player->bUseControllerRotationYaw = false;
		Player->StopAnimMontage(InjectionShotAnim);
		bIsInjectionShot = false;
	}
}

void URoomAbilityComponent::OnRoomDetectedEnemy(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AEnemy* Enemy = Cast<AEnemy>(OtherActor);
	if (Enemy)
	{
		if (!Enemy->GetIsDead())
		{
			Enemies.AddUnique(Enemy);
		}
	}
}

class AEnemy* URoomAbilityComponent::GetClosestEnemy() const
{
	if (Enemies.Num() != 0)
	{
		AEnemy* ClosestEnemy = Enemies[0];
		float MinAngle = CalculateAngle(ClosestEnemy);
		for (AEnemy* const Enemy : Enemies)
		{
			if (CalculateAngle(Enemy) < MinAngle)
			{
				MinAngle = CalculateAngle(Enemy);
				ClosestEnemy = Enemy;
			}
		}

		return ClosestEnemy;
	}

	return nullptr;
}

float URoomAbilityComponent::CalculateDistance(AEnemy* Enemy, AActor* Target) const
{
	return (Target) ? (Target->GetActorLocation() - Enemy->GetActorLocation()).Size() : 0.f;
}

float URoomAbilityComponent::CalculateAngle(class AEnemy* Enemy) const
{
	if (Player)
	{
		FVector ControllerForwardVector = FVector(Player->GetControlRotation().Vector().X, Player->GetControlRotation().Vector().Y, 0.f).GetSafeNormal();
		FVector EnemyDirection = FVector((Player->GetActorLocation() - Enemy->GetActorLocation()).X,
			(Player->GetActorLocation() - Enemy->GetActorLocation()).Y, 0.f).GetSafeNormal();

		float Angle = FMath::Acos(FVector::DotProduct(-ControllerForwardVector, EnemyDirection));

		return Angle;
	}

	return 0.f;
}

void URoomAbilityComponent::FocusOnTarget()
{
	if (Player)
	{
		FocusEnemy = GetClosestEnemy();
		if ((Enemies.Num() != 0) && FocusEnemy && CheckPlayerInsideRoom(Player))
		{
			if (bIsFocused)
			{
				Player->bUseControllerRotationYaw = false;
				FocusEnemy = nullptr;
				bIsFocused = false;
			}
			else
			{
				Sort(Enemies, 0, Enemies.Num() - 1);
				Player->bUseControllerRotationYaw = true;
				LookAtEnemy();
				bIsFocused = true;
			}
		}
		else
		{
			FocusEnemy = nullptr;
			Player->bUseControllerRotationYaw = false;
			bIsFocused = false;
		}
	}
}

void URoomAbilityComponent::LookAtEnemy()
{
	if (Player && FocusEnemy)
	{
		if (CheckPlayerInsideRoom(Player))
		{
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Player->GetActorLocation(), FocusEnemy->GetActorLocation());
				FRotator NewRotation = FRotator(LookAtRotation.Pitch - 30.f, LookAtRotation.Yaw, LookAtRotation.Roll);
				PlayerController->SetControlRotation(NewRotation);
			}
		}
		else
		{
			Player->bUseControllerRotationYaw = false;
			bIsFocused = false;
		}
	}

}

bool URoomAbilityComponent::CheckPlayerInsideRoom(ALawRoomCharacter* Player) const
{
	if (Player && Room)
	{
		// the distance between the player and the center of the room
		float Distance = (Player->GetActorLocation() - Room->GetComponentLocation()).Size();
		// RoomRadius * 100: bescause unreal unit is cm and the radius is in meter
		return (Distance <= RoomRadius * 100);
	}

	return false;
}

void URoomAbilityComponent::Sort(TArray<AEnemy*>& Enemies, int32 Start, int32 End)
{
	if (Start < End)
	{
		int32 Pi = Partition(Enemies, Start, End);

		// Separately sort elements before 
		// partition and after partition 
		Sort(Enemies, Start, Pi - 1);
		Sort(Enemies, Pi + 1, End);
	}
}

int32 URoomAbilityComponent::Partition(TArray<AEnemy*>& Enemies, int32 Start, int32 End)
{
	AEnemy* Pivot = Enemies[End];    // pivot 
	int32 i = (Start - 1);  // Index of smaller element 

	for (int32 j = Start; j <= End - 1; j++)
	{
		// If current element is smaller than or 
		// equal to pivot 
		if (CalculateDistance(GetClosestEnemy(), Enemies[j]) <= CalculateDistance(GetClosestEnemy(), Pivot))
		{
			i++; // increment index of smaller element 
			Swap(Enemies[i], Enemies[j]);
		}
	}
	Swap(Enemies[i + 1], Enemies[End]);

	return (i + 1);
}

void URoomAbilityComponent::Swap(AEnemy*& Enemy1, AEnemy*& Enemy2)
{
	AEnemy* Temp = Enemy1;
	Enemy1 = Enemy2;
	Enemy2 = Temp;
}


void URoomAbilityComponent::ChangeTarget(float Value)
{
	if (bIsFocused && (Enemies.Num() != 0) && FocusEnemy && (Value != 0))
	{
		int32 Index = Enemies.Find(FocusEnemy);
		if (Index != INDEX_NONE)
		{
			Index = (Index + (int32)Value) % Enemies.Num();
			Index = (Index < 0) ? Enemies.Num() + Index : Index;

			FocusEnemy = Enemies[Index];
		}
	}
}

void URoomAbilityComponent::RequestInjectionShot()
{
	if (FocusEnemy && bIsFocused && Player && !bIsInjectionShot && CheckPlayerInsideRoom(Player))
	{
		// prevent the room from being destroyed when performing injection shot (pause it's life progression)
		UpdateColorTimeline->Stop();

		// disable player input when performing attack
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		if (PlayerController)
		{
			Player->DisableInput(PlayerController);
		}

		Player->StartInjectionShot();
	}
}

void URoomAbilityComponent::InjectionShot()
{
	if (FocusEnemy && bIsFocused && ensure(InjectionShotAnim) && Player && CheckPlayerInsideRoom(Player))
	{
		bIsInjectionShot = true;
		Player->PlayAnimMontage(InjectionShotAnim);

		FVector LaunchDirection = UKismetMathLibrary::GetDirectionUnitVector(Player->GetActorLocation(), FocusEnemy->GetActorLocation());
		float LaunchForce = (Player->GetActorLocation() - FocusEnemy->GetActorLocation()).Size() * 20.f;
		FVector LaunchVelocity = LaunchDirection * LaunchForce;

		Player->LaunchCharacter(LaunchVelocity, true, true);

		bIsFocused = false;
		FocusEnemy = nullptr;
		Player->bUseControllerRotationYaw = false;
	}
}

void URoomAbilityComponent::UpdateEnemyStatus(AEnemy* Enemy)
{
	if (Enemy)
	{
		// disable enemy capsule component collision with the player pawn and katana
		Enemy->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

		//Rag doll death
		Enemy->GetMesh()->SetAllBodiesBelowSimulatePhysics(FName("pelvis"), true, true);
		Enemy->GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(FName("pelvis"), 1.f);
		Enemy->SetIsDead(true);

		// launch enemy
		FVector LaunchDirection = Katana->GetRightVector() * 700.f;
		Enemy->GetMesh()->AddImpulseToAllBodiesBelow(LaunchDirection, "pelvis", true);

		//delete dead enemies from the array
		Enemies.Remove(Enemy);
	}
}

void URoomAbilityComponent::OnKatanaCollidedWithEnemy(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AEnemy* Enemy = Cast<AEnemy>(OtherActor);
	if (Enemy && bIsInjectionShot)
	{	
		UpdateEnemyStatus(Enemy);

		if (Player && ensure(InjectionShotAnim))
		{
			Player->StopAnimMontage(InjectionShotAnim);
			bIsInjectionShot = false;

			// continue room's life progression
			UpdateColorTimeline->Play();

			// enable back player input after performing attack
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				Player->EnableInput(PlayerController);
			}
		}
	}
}