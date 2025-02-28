// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MHProjectCharacter.generated.h"

// �α� ī�װ� ����
DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// ���� ����(Ŭ������ ����ü)
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

// ���� Ÿ�� ������
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	GreatSword       UMETA(DisplayName = "���"),
	LongSword        UMETA(DisplayName = "�µ�"),
	DualBlades       UMETA(DisplayName = "�ְ�"),
	HammerAxe        UMETA(DisplayName = "�ظ�/����")
};

// ���� ���� ������
UENUM(BlueprintType)
enum class EAttackState : uint8
{
	None            UMETA(DisplayName = "����"),
	NormalAttack    UMETA(DisplayName = "�⺻����"),
	ChargedAttack   UMETA(DisplayName = "��������"),
	SpecialAttack   UMETA(DisplayName = "Ư������")
};

// ĳ���� ���� ������
UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle            UMETA(DisplayName = "���"),
	Moving          UMETA(DisplayName = "�̵���"),
	Attacking       UMETA(DisplayName = "������"),
	Dodging         UMETA(DisplayName = "ȸ����"),
	Guarding        UMETA(DisplayName = "������"),
	Stunned         UMETA(DisplayName = "����"),
	KnockedDown     UMETA(DisplayName = "�˴ٿ�")
};

/**
 * AMHProjectCharacter
 * - ���� ĳ������ �⺻ ���� �� �Է� ó���� ������ Ŭ����
 */
UCLASS(config = Game)
class AMHProjectCharacter : public ACharacter
{
	GENERATED_BODY()

private:
	/** ī�޶� ��: ĳ���� �ڿ��� ī�޶� ��ġ��Ű�� ���� ������Ʈ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** �ȷο� ī�޶�: �÷��̾ ����ٴϴ� ī�޶� */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** �Է� ���� ���ؽ�Ʈ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** ���� �Է� �׼� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** �̵� �Է� �׼� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** ī�޶� ȸ�� �Է� �׼� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** ���� �Է� �׼� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

	/** ���� �ִϸ��̼� ��Ÿ�� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* AttackMontage;

	/** �޺� Ÿ�̸� �ڵ� */
	FTimerHandle ComboTimer;

	/** �޺� Ÿ�̸� �ð�*/
	const float fCombotime = 1.5f;

	/** �޺� ī��Ʈ */
	int32 ComboCount = 0;

	/** ���� ���� ������ ���� */
	bool bIsAttacking = false;

	/** ���� ������ �������� ���� */
	bool bCanNextAttack = true;

public:
	/** ������ */
	AMHProjectCharacter();

	/** �޺� �ʱ�ȭ */
	void ResetCombo();

	/** ���� ���� Ȱ��ȭ */
	UFUNCTION(BlueprintCallable)
	void EnableNextAttack();

	/** ���� ���� ��Ȱ��ȭ */
	UFUNCTION(BlueprintCallable)
	void DisableNextAttack();

protected:
	/** �̵� �Է� ó�� */
	void Move(const FInputActionValue& Value);

	/** ī�޶� ȸ�� �Է� ó�� */
	void Look(const FInputActionValue& Value);

	/** ���� ó�� */
	void Attack();

protected:
	/**
	 * �Է� ������Ʈ�� �����ϴ� �Լ� (APawn �������̽� ����)
	 * @param PlayerInputComponent �Է� ������Ʈ
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * ���� �� ȣ��Ǵ� �Լ� (���� ���ؽ�Ʈ �߰�)
	 */
	virtual void BeginPlay();

public:
	/** CameraBoom ���������Ʈ ��ȯ */
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** FollowCamera ���������Ʈ ��ȯ */
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
