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
    GreatSword       UMETA(DisplayName = "Great Sword"),
    LongSword        UMETA(DisplayName = "Long Sword"),
    DualBlades       UMETA(DisplayName = "Dual Blades"),
    HammerAxe        UMETA(DisplayName = "Hammer/Axe")
};

// ���� �Ӽ� ������
UENUM(BlueprintType)
enum class EElementalType : uint8
{
    None        UMETA(DisplayName = "None"),
    Fire        UMETA(DisplayName = "Fire"),
    Water       UMETA(DisplayName = "Water"),
    Thunder     UMETA(DisplayName = "Thunder"),
    Ice         UMETA(DisplayName = "Ice"),
    Dragon      UMETA(DisplayName = "Dragon")
};

// ���� ���� ������
UENUM(BlueprintType)
enum class EAttackState : uint8
{
    None            UMETA(DisplayName = "None"),
    NormalAttack    UMETA(DisplayName = "NormalAttack"),
    ChargedAttack   UMETA(DisplayName = "ChargedAttack"),
    SpecialAttack   UMETA(DisplayName = "SpecialAttack")
};

// ĳ���� ���� ������
UENUM(BlueprintType)
enum class ECharacterState : uint8
{
    Idle            UMETA(DisplayName = "Idle"),
    Moving          UMETA(DisplayName = "Moving"),
    Attacking       UMETA(DisplayName = "Attacking"),
    Dodging         UMETA(DisplayName = "Dodging"),
    Guarding        UMETA(DisplayName = "Guarding"),
    Stunned         UMETA(DisplayName = "Stunned"),
    KnockedDown     UMETA(DisplayName = "KnockedDown")
};


// ��Ʈ ���� ����ü
USTRUCT(BlueprintType)
struct FHitInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ElementalDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TEnumAsByte<EPhysicalSurface> SurfaceType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector HitLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector HitDirection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StunChance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float KnockbackForce;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* HitActor;

    // �⺻ ������
    FHitInfo()
    {
        BaseDamage = 10.0f;
        ElementalDamage = 0.0f;
        SurfaceType = EPhysicalSurface::SurfaceType_Default;
        HitLocation = FVector::ZeroVector;
        HitDirection = FVector::ForwardVector;
        StunChance = 0.0f;
        KnockbackForce = 0.0f;
        HitActor = nullptr;
    }
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

    // ���� ���� �Լ�
    void PlayWeaponSpecificAnimation(UAnimMontage* BaseMontage);
    float GetWeaponSpeedModifier();
    void UpdateStaminaRegen();
    void UpdateWeaponVisuals();

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

    // ���� �ý���
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EWeaponType CurrentWeaponType;

    // ���¹̳� �ý���
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxStamina;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float CurrentStamina;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float StaminaRegenRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackStaminaCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float DodgeStaminaCost;

    // �ִϸ��̼� ��Ÿ�ֵ�
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* DodgeMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* GuardMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* ChargedAttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* SpecialAttackMontage;

    // ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    ECharacterState CurrentState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    EAttackState CurrentAttackState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    bool bIsCharging;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float ChargeTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float MaxChargeTime;

    // ���� ĵ�� ���� ����
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
    bool bCanCancelAttack;

    // Ÿ�̸� �ڵ�
    FTimerHandle ChargeTimer;
    FTimerHandle StaminaRegenTimer;

    // ���� ������Ʈ
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    UStaticMeshComponent* WeaponMesh;

    // ��Ʈ ȿ��
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* HitParticle;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* HitSound;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UMaterialInterface* HitDecalMaterial;

    // ���� �Ӽ�
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EElementalType WeaponElement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float ElementalDamage;

    // ���� �ǰ� ���� ����
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    bool bIsWeaponTracing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TArray<AActor*> HitActors;

    // ���ݺ� ������ �� �Ӽ� ����
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float NormalAttackDamage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float ChargedAttackDamage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float SpecialAttackDamage;

    // �׼� �Լ���
    UFUNCTION(BlueprintCallable, Category = "Actions")
    void Dodge();

    UFUNCTION(BlueprintCallable, Category = "Actions")
    void Guard();

    UFUNCTION(BlueprintCallable, Category = "Actions")
    void StartChargeAttack();

    UFUNCTION(BlueprintCallable, Category = "Actions")
    void ReleaseChargeAttack();

    UFUNCTION(BlueprintCallable, Category = "Actions")
    void SpecialAttack();

    // ���¹̳� ���� �Լ�
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void ConsumeStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void RegenStamina();

    UFUNCTION(BlueprintCallable, Category = "Stats")
    bool HasEnoughStamina(float Cost);

    // �ִϸ��̼� ĵ�� �Լ�
    UFUNCTION(BlueprintCallable, Category = "Actions")
    void CancelCurrentAction();

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool CanCancelIntoAction(ECharacterState NewState);

    // ���� ���� �Լ�
    UFUNCTION(BlueprintCallable, Category = "State")
    void SetCharacterState(ECharacterState NewState);

    // ���� ���� �Լ�
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SwitchWeapon(EWeaponType NewWeaponType);

    // ���� Ʈ���̽� �Լ�
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void EndWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void PerformWeaponTrace();

    // ��Ʈ ����Ʈ ����
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SpawnHitEffect(const FHitResult& Hit);

    // ������ ����
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void ApplyDamage(AActor* HitActor, const FHitInfo& HitInfo);

    // ���⺰ Ư�� ȿ�� ����
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void ApplyWeaponSpecialEffect(AActor* HitActor);

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

    // Ÿ�̸� �Լ�
    virtual void Tick(float DeltaTime) override;

    // ���ο� �Է� �׼ǵ�
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* DodgeAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* GuardAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* SpecialAction;

public:
    /** CameraBoom ���������Ʈ ��ȯ */
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    /** FollowCamera ���������Ʈ ��ȯ */
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};