// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types.generated.h"

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Aim_State UMETA(DisplayName = "Aim State"),
	AimWalk_State UMETA(DisplayName = "AimWalk State"),
	Walk_State UMETA(DisplayName = "Walk State"),
	Run_State UMETA(DisplayName = "Run State"),
	Sprint_State UMETA(DisplayName = "Sprint State"),
	LessStamina_State UMETA(DisplayName = "Less Stamina"),
};

USTRUCT(BlueprintType)
struct FChatacterSpeed
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AimSpeedNormal = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeedNormal = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RunSpeedNormal = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AimSpeedWalk = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RunSpeedSprint = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LessStaminaSpeed = 0.0f;

};

UCLASS()
class TPS_API UTypes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
};
