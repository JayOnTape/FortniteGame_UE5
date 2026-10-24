// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FortWeaponAnimSet.generated.h"

/**
 * 
 */
UCLASS()
class FORTNITEGAME_API UFortWeaponAnimSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	bool bShouldUseIK;

	// --- Targeting Aim Offset ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Targeting")
	class UAimOffsetBlendSpace* TargetingAimOffset_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Targeting")
	class UAimOffsetBlendSpace* TargetingAimOffset_Female;

	// --- Non-Targeting Aim Offset ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Targeting")
	class UAimOffsetBlendSpace* NonTargetingAimOffset_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Targeting")
	class UAimOffsetBlendSpace* NonTargetingAimOffset_Female;

	// --- Relaxed Pose ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Relaxed")
	class UAnimSequence* RelaxedPose_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Relaxed")
	class UAnimSequence* RelaxedPose_Female;

	// --- Relaxed Aim Offset ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Relaxed")
	class UAimOffsetBlendSpace* RelaxedAimOffset_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Relaxed")
	class UAimOffsetBlendSpace* RelaxedAimOffset_Female;

	// --- Jog Additive Blend Space ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Locomotion")
	class UBlendSpace* JogAdditiveBlendSpace_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Locomotion")
	class UBlendSpace* JogAdditiveBlendSpace_Female;

	// --- Jog Core Blend Space ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Locomotion")
	class UBlendSpace* JogCoreBlendSpace_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Locomotion")
	class UBlendSpace* JogCoreBlendSpace_Female;

	// --- Jog Additive Blend Space Relaxed ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Locomotion")
	class UBlendSpace* JogAdditiveBlendSpaceRelaxed_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Locomotion")
	class UBlendSpace* JogAdditiveBlendSpaceRelaxed_Female;

	// --- Sprint Animation ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Locomotion")
	class UAnimSequence* SprintAnimation_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Locomotion")
	class UAnimSequence* SprintAnimation_Female;

	// --- Crouch Targeting Pose ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UAnimSequence* CrouchTargetingPose_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UAnimSequence* CrouchTargetingPose_Female;

	// --- Crouch Non-Targeting Pose ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UAnimSequence* CrouchNonTargetingPose_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UAnimSequence* CrouchNonTargetingPose_Female;

	// --- Crouch Relaxed Pose ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UAnimSequence* CrouchRelaxedPose_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UAnimSequence* CrouchRelaxedPose_Female;

	// --- Crouch Core Targeting Blend Space ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchCoreTargetingBlendSpace_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchCoreTargetingBlendSpace_Female;

	// --- Crouch Core Non-Targeting Blend Space ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchCoreNonTargetingBlendSpace_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchCoreNonTargetingBlendSpace_Female;

	// --- Crouch Walk Additive Blend Space ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchWalkAdditiveBlendSpace_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchWalkAdditiveBlendSpace_Female;

	// --- Crouch Jog Additive Blend Space ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchJogAdditiveBlendSpace_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchJogAdditiveBlendSpace_Female;

	// --- Crouch Jog Additive Blend Space Relaxed ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchJogAdditiveBlendSpaceRelaxed_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UBlendSpace* CrouchJogAdditiveBlendSpaceRelaxed_Female;

	// --- Crouch Sprint Animation ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UAnimSequence* CrouchSprintAnimation_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Crouch")
	class UAnimSequence* CrouchSprintAnimation_Female;

	// --- Idle Noise AR Down Sights ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Idle")
	class UAnimSequence* IdleNoise_AR_DownSights_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Idle")
	class UAnimSequence* IdleNoise_AR_DownSights_Female;

	// --- Fly Mode Loop Additive ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Movement Extras")
	class UAnimSequence* FlyModeLoopAdditive_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Movement Extras")
	class UAnimSequence* FlyModeLoopAdditive_Female;

	// --- Fly Mode Start Additive ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Movement Extras")
	class UAnimSequence* FlyModeStartAdditive_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Movement Extras")
	class UAnimSequence* FlyModeStartAdditive_Female;

	// --- Zipline Start Additive ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Movement Extras")
	class UAnimSequence* ZipLineStartAdditve_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Movement Extras")
	class UAnimSequence* ZipLineStartAdditve_Female;

	// --- Fall Additive ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Jump")
	class UAnimSequence* FallAdditive_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Jump")
	class UAnimSequence* FallAdditive_Female;

	// --- Jump Loop Additive ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Jump")
	class UAnimSequence* JumpLoopAdditive_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Jump")
	class UAnimSequence* JumpLoopAdditive_Female;

	// --- Jump Up Additive ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Jump")
	class UAnimSequence* JumpUpAdditive_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Jump")
	class UAnimSequence* JumpUpAdditive_Female;

	// --- Idle Noise ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Idle")
	class UAnimSequence* IdleNoise_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Idle")
	class UAnimSequence* IdleNoise_Female;

	// --- Targeting Pose ---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Targeting")
	class UAnimSequence* TargetingPose_Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation | Targeting")
	class UAnimSequence* TargetingPose_Female;

};