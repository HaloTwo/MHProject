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

// 무기 타입 열거형
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    GreatSword       UMETA(DisplayName = "Great Sword"),
    LongSword        UMETA(DisplayName = "Long Sword"),
    DualBlades       UMETA(DisplayName = "Dual Blades"),
    HammerAxe        UMETA(DisplayName = "Hammer/Axe")
};

// 무기 속성 열거형
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

// 공격 상태 열거형
UENUM(BlueprintType)
enum class EAttackState : uint8
{
    None            UMETA(DisplayName = "None"),
    NormalAttack    UMETA(DisplayName = "NormalAttack"),
    ChargedAttack   UMETA(DisplayName = "ChargedAttack"),
    SpecialAttack   UMETA(DisplayName = "SpecialAttack")
};

// 캐릭터 상태 열거형
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


// 히트 정보 구조체
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

    // 기본 생성자
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

    // 내부 헬퍼 함수
    void PlayWeaponSpecificAnimation(UAnimMontage* BaseMontage);
    float GetWeaponSpeedModifier();
    void UpdateStaminaRegen();
    void UpdateWeaponVisuals();

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

    // 무기 시스템
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EWeaponType CurrentWeaponType;

    // 스태미나 시스템
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

    // 애니메이션 몽타주들
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* DodgeMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* GuardMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* ChargedAttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* SpecialAttackMontage;

    // 상태 관리
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

    // 공격 캔슬 가능 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
    bool bCanCancelAttack;

    // 타이머 핸들
    FTimerHandle ChargeTimer;
    FTimerHandle StaminaRegenTimer;

    // 무기 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    UStaticMeshComponent* WeaponMesh;

    // 히트 효과
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* HitParticle;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* HitSound;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UMaterialInterface* HitDecalMaterial;

    // 무기 속성
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EElementalType WeaponElement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float ElementalDamage;

    // 무기 피격 판정 관련
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    bool bIsWeaponTracing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TArray<AActor*> HitActors;

    // 공격별 데미지 및 속성 설정
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float NormalAttackDamage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float ChargedAttackDamage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float SpecialAttackDamage;

    // 액션 함수들
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

    // 스태미나 관련 함수
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void ConsumeStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void RegenStamina();

    UFUNCTION(BlueprintCallable, Category = "Stats")
    bool HasEnoughStamina(float Cost);

    // 애니메이션 캔슬 함수
    UFUNCTION(BlueprintCallable, Category = "Actions")
    void CancelCurrentAction();

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool CanCancelIntoAction(ECharacterState NewState);

    // 상태 관리 함수
    UFUNCTION(BlueprintCallable, Category = "State")
    void SetCharacterState(ECharacterState NewState);

    // 무기 변경 함수
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SwitchWeapon(EWeaponType NewWeaponType);

    // 무기 트레이스 함수
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void EndWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void PerformWeaponTrace();

    // 히트 이펙트 생성
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SpawnHitEffect(const FHitResult& Hit);

    // 데미지 적용
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void ApplyDamage(AActor* HitActor, const FHitInfo& HitInfo);

    // 무기별 특수 효과 적용
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void ApplyWeaponSpecialEffect(AActor* HitActor);

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

    // 타이머 함수
    virtual void Tick(float DeltaTime) override;

    // 새로운 입력 액션들
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* DodgeAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* GuardAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* SpecialAction;

public:
    /** CameraBoom 서브오브젝트 반환 */
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    /** FollowCamera 서브오브젝트 반환 */
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};