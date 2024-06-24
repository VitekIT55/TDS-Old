// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/GameEngine.h"
#include "Engine/World.h"
#include "../Game/TPSGameInstance.h"

ATPSCharacter::ATPSCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Rotate character to moving direction
	//GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ATPSCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	if (CurrentCursor)
	{
		APlayerController* myPC = Cast<APlayerController>(GetController());
		if (myPC)
		{
			FHitResult TraceHitResult;
			myPC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();

			CurrentCursor->SetWorldLocation(TraceHitResult.Location);
			CurrentCursor->SetWorldRotation(CursorR);
		}
	}
	MovementTick(DeltaSeconds);
}

void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitWeapon(InitWeaponName);

	if (CursorMaterial)
	{
		CurrentCursor = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CursorMaterial, CursorSize, FVector(0));
	}
}

void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* NewInputComponent)
{
	Super::SetupPlayerInputComponent(NewInputComponent);

	NewInputComponent->BindAxis(TEXT("MoveForward"), this, &ATPSCharacter::InputAxisX);
	NewInputComponent->BindAxis(TEXT("MoveRight"), this, &ATPSCharacter::InputAxisY);

	NewInputComponent->BindAction(TEXT("FireEvent"), EInputEvent::IE_Pressed, this, &ATPSCharacter::InputAttackPressed);
	NewInputComponent->BindAction(TEXT("FireEvent"), EInputEvent::IE_Released, this, &ATPSCharacter::InputAttackReleased);
	NewInputComponent->BindAction(TEXT("ReloadEvent"), EInputEvent::IE_Released, this, &ATPSCharacter::TryReloadWeapon);
}

void ATPSCharacter::InputAxisX(float Value)
{
	AxisX = Value;
}

void ATPSCharacter::InputAxisY(float Value)
{
	AxisY = Value;
}

void ATPSCharacter::InputAttackPressed()
{
	AttackCharEvent(true);
}

void ATPSCharacter::InputAttackReleased()
{
	AttackCharEvent(false);
}


void ATPSCharacter::MovementTick(float DeltaTime)
{
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), AxisX);
	AddMovementInput(FVector(0.0f, 1.0f, 0.0f), AxisY);
	//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("AxisX: %f"), AxisX));
	//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, FString::Printf(TEXT("AxisY: %f"), AxisY));

	APlayerController* myController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (myController)
	{
		FHitResult TraceHitResult;
		myController->GetHitResultUnderCursor(ECC_GameTraceChannel1, false, TraceHitResult);
		float FindRotatorResultYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TraceHitResult.Location).Yaw;
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Yaw: %f"), FindRotatorResultYaw));
		SetActorRotation(FQuat(FRotator(0.0f, FindRotatorResultYaw, 0.0f)));
		int Xdir; int Ydir;
		if (-22.5 <= FindRotatorResultYaw && FindRotatorResultYaw <= 22.5)
		{
			Xdir = 1;
			Ydir = 0;
		}
		else if (22.5 < FindRotatorResultYaw && FindRotatorResultYaw <= 67.5)
		{
			Xdir = 1;
			Ydir = 1;
		}
		else if (67.5 < FindRotatorResultYaw && FindRotatorResultYaw <= 112.5)
		{
			Xdir = 0;
			Ydir = 1;
		}
		else if (112.5 < FindRotatorResultYaw && FindRotatorResultYaw <= 157.5)
		{
			Xdir = -1;
			Ydir = 1;
		}
		else if (157.5 < FindRotatorResultYaw || FindRotatorResultYaw <= -157.5)
		{
			Xdir = -1;
			Ydir = 0;
		}
		else if (-157.5 < FindRotatorResultYaw && FindRotatorResultYaw <= -112.5)
		{
			Xdir = -1;
			Ydir = -1;
		}
		else if (-112.5 < FindRotatorResultYaw && FindRotatorResultYaw <= -67.5)
		{
			Xdir = 0;
			Ydir = -1;
		}
		else if (-67.5 < FindRotatorResultYaw && FindRotatorResultYaw < -22.5)
		{
			Xdir = 1;
			Ydir = -1;
		}
		if (int(AxisX) == Xdir && int(AxisY) == Ydir)
			SprintAllow = 1;
		else
			SprintAllow = 0;
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Purple, FString::Printf(TEXT("Xdir: %i"), Xdir));
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, FString::Printf(TEXT("Ydir: %i"), Ydir));
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Black, FString::Printf(TEXT("Sprint: %i"), SprintAllow));
		if (CurrentWeapon)
		{
			FVector Displacement = FVector(0);
			switch (MovementState)
			{
			case EMovementState::Aim_State:
				Displacement = FVector(0.0f, 0.0f, 160.0f);
				CurrentWeapon->ShouldReduceDispersion = true;
				break;
			case EMovementState::AimWalk_State:
				CurrentWeapon->ShouldReduceDispersion = true;
				Displacement = FVector(0.0f, 0.0f, 160.0f);
				break;
			case EMovementState::Walk_State:
				Displacement = FVector(0.0f, 0.0f, 120.0f);
				CurrentWeapon->ShouldReduceDispersion = false;
				break;
			case EMovementState::Run_State:
				Displacement = FVector(0.0f, 0.0f, 120.0f);
				CurrentWeapon->ShouldReduceDispersion = false;
				break;
			case EMovementState::Sprint_State:
				break;
			default:
				break;
			}

			CurrentWeapon->ShootEndLocation = TraceHitResult.Location + Displacement;
			//aim cursor like 3d Widget?
		}
	}
}

void ATPSCharacter::AttackCharEvent(bool bIsFiring)
{
	AWeaponDefault* myWeapon = nullptr;
	myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		//ToDo Check melee or range
		myWeapon->SetWeaponStateFire(bIsFiring);
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("ATPSCharacter::AttackCharEvent - CurrentWeapon -NULL"));
}

void ATPSCharacter::CharacterUpdate()
{
	float ResSpeed = 600.0f;
	switch (MovementState)
	{
	case EMovementState::Aim_State:
		ResSpeed = MovementInfo.AimSpeedNormal;
		break;
	case EMovementState::AimWalk_State:
		ResSpeed = MovementInfo.AimSpeedWalk;
		break;
	case EMovementState::Walk_State:
		ResSpeed = MovementInfo.WalkSpeedNormal;
		break;
	case EMovementState::Run_State:
		ResSpeed = MovementInfo.RunSpeedNormal;
		break;
	case EMovementState::Sprint_State:
		ResSpeed = MovementInfo.RunSpeedSprint;
		break;
	default:
		break;
	}
	GetCharacterMovement()->MaxWalkSpeed = ResSpeed;
	if (CharacterSpeed >= 790 && Stamina > 0.0f && SprintBlock == 0)
	{
		Stamina -= 0.005;
	}
	else if (CharacterSpeed <= 789 && Stamina < 1.0f)
		Stamina += 0.005;
	else if (Stamina >= 1.0f && SprintBlock == 1)
		SprintBlock = 0;
	if (Stamina <= 0.0f)
		SprintBlock = 1;
	//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("SprintBlock: %i"), SprintBlock));
	//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, FString::Printf(TEXT("Stamina: %f"), Stamina));
}

void ATPSCharacter::ChangeMovementState()
{
	if (!WalkEnabled && !SprintEnabled && !AimEnabled)
	{
		MovementState = EMovementState::Run_State;
	}
	else
	{
		if (SprintEnabled)
		{
			WalkEnabled = false;
			AimEnabled = false;
			MovementState = EMovementState::Sprint_State;
		}
		if (WalkEnabled && !SprintEnabled && AimEnabled)
			MovementState = EMovementState::AimWalk_State;
		else if (WalkEnabled && !SprintEnabled && !AimEnabled)
			MovementState = EMovementState::Walk_State;
		else if (!WalkEnabled && !SprintEnabled && AimEnabled)
			MovementState = EMovementState::Aim_State;
	}
	CharacterUpdate();
	//Weapon state update
	AWeaponDefault* myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		myWeapon->UpdateStateWeapon(MovementState);
	}
}

AWeaponDefault* ATPSCharacter::GetCurrentWeapon()
{
	return CurrentWeapon;
}

void ATPSCharacter::InitWeapon(FName IdWeaponName)//ToDo Init by id row by table
{
	UTPSGameInstance* myGI = Cast<UTPSGameInstance>(GetGameInstance());
	FWeaponInfo myWeaponInfo;
	if (myGI)
	{
		if (myGI->GetWeaponInfoByName(IdWeaponName, myWeaponInfo))
		{
			if (myWeaponInfo.WeaponClass)
			{
				FVector SpawnLocation = FVector(0);
				FRotator SpawnRotation = FRotator(0);

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.Owner = GetOwner();
				SpawnParams.Instigator = GetInstigator();

				AWeaponDefault* myWeapon = Cast<AWeaponDefault>(GetWorld()->SpawnActor(myWeaponInfo.WeaponClass, &SpawnLocation, &SpawnRotation, SpawnParams));
				if (myWeapon)
				{
					FAttachmentTransformRules Rule(EAttachmentRule::SnapToTarget, false);
					myWeapon->AttachToComponent(GetMesh(), Rule, FName("WeaponSocketRightHand"));
					CurrentWeapon = myWeapon;

					myWeapon->WeaponSetting = myWeaponInfo;
					myWeapon->WeaponInfo.Round = myWeaponInfo.MaxRound;
					//Remove !!! Debug
					//myWeapon->ReloadTime = myWeaponInfo.ReloadTime;
					//myWeapon->UpdateStateWeapon(MovementState);

					myWeapon->OnWeaponReloadStart.AddDynamic(this, &ATPSCharacter::WeaponReloadStart);
					myWeapon->OnWeaponReloadEnd.AddDynamic(this, &ATPSCharacter::WeaponReloadEnd);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ATPSCharacter::InitWeapon - Weapon not found in table -NULL"));
		}
	}
}

void ATPSCharacter::TryReloadWeapon()
{
	if (CurrentWeapon)
	{
		if (CurrentWeapon->GetWeaponRound() <= CurrentWeapon->WeaponSetting.MaxRound)
			CurrentWeapon->InitReload();
	}
}

void ATPSCharacter::WeaponReloadStart(UAnimMontage* Anim)
{
	WeaponReloadStart_BP(Anim);
}

void ATPSCharacter::WeaponReloadEnd()
{
	WeaponReloadEnd_BP();
}

void ATPSCharacter::WeaponReloadStart_BP_Implementation(UAnimMontage* Anim)
{
	// in BP
}

void ATPSCharacter::WeaponReloadEnd_BP_Implementation()
{
	// in BP
}

UDecalComponent* ATPSCharacter::GetCursorToWorld()
{
	return CurrentCursor;
}
