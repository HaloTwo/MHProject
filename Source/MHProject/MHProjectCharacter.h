// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MHProjectCharacter.generated.h"

// 로그 카테고리 선언
DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// 전방 선언(클래스와 구조체)
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * AMHProjectCharacter
 * - 게임 캐릭터의 기본 동작 및 입력 처리를 정의한 클래스
 */
UCLASS(config = Game)
class AMHProjectCharacter : public ACharacter
{
	GENERATED_BODY()

private:
	/** 카메라 붐: 캐릭터 뒤에서 카메라를 위치시키기 위한 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** 팔로우 카메라: 플레이어를 따라다니는 카메라 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** 입력 매핑 컨텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** 점프 입력 액션 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** 이동 입력 액션 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** 카메라 회전 입력 액션 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** 공격 입력 액션 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

	/** 공격 애니메이션 몽타주 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* AttackMontage;

	/** 콤보 타이머 핸들 */
	FTimerHandle ComboTimer;

	/** 콤보 타이머 시간*/
	const float fCombotime = 1.5f;

	/** 콤보 카운트 */
	int32 ComboCount = 0;

	/** 현재 공격 중인지 여부 */
	bool bIsAttacking = false;

	/** 다음 공격이 가능한지 여부 */
	bool bCanNextAttack = true;

public:
	/** 생성자 */
	AMHProjectCharacter();

	/** 콤보 초기화 */
	void ResetCombo();

	/** 다음 공격 활성화 */
	UFUNCTION(BlueprintCallable)
	void EnableNextAttack();

	/** 다음 공격 비활성화 */
	UFUNCTION(BlueprintCallable)
	void DisableNextAttack();

protected:
	/** 이동 입력 처리 */
	void Move(const FInputActionValue& Value);

	/** 카메라 회전 입력 처리 */
	void Look(const FInputActionValue& Value);

	/** 공격 처리 */
	void Attack();

protected:
	/**
	 * 입력 컴포넌트를 설정하는 함수 (APawn 인터페이스 구현)
	 * @param PlayerInputComponent 입력 컴포넌트
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * 시작 시 호출되는 함수 (맵핑 컨텍스트 추가)
	 */
	virtual void BeginPlay();

public:
	/** CameraBoom 서브오브젝트 반환 */
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** FollowCamera 서브오브젝트 반환 */
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
