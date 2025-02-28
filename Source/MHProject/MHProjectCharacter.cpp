// Copyright Epic Games, Inc. All Rights Reserved.

// 필요한 헤더 파일 포함
#include "MHProjectCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "DrawDebugHelpers.h"

// 로그 카테고리 정의
DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMHProjectCharacter

// 클래스 생성자
AMHProjectCharacter::AMHProjectCharacter()
{
    // 캡슐 컴포넌트 초기화 (캐릭터의 충돌 크기 설정)
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // 컨트롤러 회전에 의해 캐릭터가 회전하지 않도록 설정
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 캐릭터 이동 관련 설정
    GetCharacterMovement()->bOrientRotationToMovement = true; // 입력 방향으로 이동
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 회전 속도
    GetCharacterMovement()->JumpZVelocity = 700.f; // 점프 속도
    GetCharacterMovement()->AirControl = 0.35f; // 공중에서의 제어력
    GetCharacterMovement()->MaxWalkSpeed = 500.f; // 최대 걷기 속도
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f; // 최소 아날로그 걷기 속도
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f; // 걷기 감속
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f; // 낙하 감속

    // 카메라 붐 생성 (캐릭터 뒤로 따라오는 카메라 거리 설정)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f; // 카메라와 캐릭터 간의 거리
    CameraBoom->bUsePawnControlRotation = true; // 컨트롤러 방향에 따라 카메라 회전

    // 팔로우 카메라 생성
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // 카메라 붐에 연결
    FollowCamera->bUsePawnControlRotation = false; // 카메라가 팔 회전에 따라 회전하지 않음

    // 스태미나 시스템 초기화
    MaxStamina = 100.0f;
    CurrentStamina = MaxStamina;
    StaminaRegenRate = 10.0f;
    AttackStaminaCost = 20.0f;
    DodgeStaminaCost = 15.0f;

    // 기본 무기 타입 설정
    CurrentWeaponType = EWeaponType::HammerAxe;

    // 기본 상태 설정
    CurrentState = ECharacterState::Idle;
    CurrentAttackState = EAttackState::None;

    // 공격 상태 초기화
    bIsAttacking = false;
    bCanNextAttack = true;
    bCanCancelAttack = false;

    // 충전 공격 초기화
    bIsCharging = false;
    ChargeTime = 0.0f;
    MaxChargeTime = 3.0f;

    // 무기 메시 생성
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), FName("hand_rSocket")); // 오른손 소켓에 부착
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 콜리전 비활성화

    // 무기 속성 초기화
    WeaponElement = EElementalType::None;
    ElementalDamage = 0.0f;

    // 무기 트레이스 초기화
    bIsWeaponTracing = false;

    // 공격 데미지 초기화
    NormalAttackDamage = 20.0f;
    ChargedAttackDamage = 40.0f;
    SpecialAttackDamage = 60.0f;
}

// 게임 시작 시 호출
void AMHProjectCharacter::BeginPlay()
{
    // 부모 클래스의 BeginPlay 호출
    Super::BeginPlay();

    // 스태미나 재생 타이머 설정
    GetWorldTimerManager().SetTimer(StaminaRegenTimer, this, &AMHProjectCharacter::RegenStamina, 0.1f, true);

    // 무기 타입에 따른 메시 및 데미지 설정
    UpdateWeaponVisuals();
}

// Tick 함수 추가 (매 프레임 호출)
void AMHProjectCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 충전 공격 업데이트
    if (bIsCharging)
    {
        ChargeTime += DeltaTime;
        if (ChargeTime >= MaxChargeTime)
        {
            // 최대 충전 달성 효과 (선택적)
            // 예: 이펙트 생성, 사운드 재생 등
        }
    }

    // 스태미나가 최대치보다 낮을 때만 업데이트
    if (CurrentStamina < MaxStamina)
    {
        UpdateStaminaRegen();
    }

    // 무기 트레이스 업데이트
    if (bIsWeaponTracing)
    {
        PerformWeaponTrace();
    }
}

//////////////////////////////////////////////////////////////////////////
// Input

// 플레이어 입력 설정
void AMHProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // 입력 매핑 컨텍스트 추가
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0); // 기본 매핑 컨텍스트 추가
        }
    }

    // 입력 액션 바인딩
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // 점프 바인딩
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // 이동 바인딩
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMHProjectCharacter::Move);

        // 시야 전환 바인딩
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMHProjectCharacter::Look);

        // 공격 바인딩
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AMHProjectCharacter::StartChargeAttack);
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AMHProjectCharacter::ReleaseChargeAttack);

        // 새로운 액션 바인딩
        if (DodgeAction)
            EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AMHProjectCharacter::Dodge);

        if (GuardAction)
        {
            EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Started, this, &AMHProjectCharacter::Guard);
            EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Completed, this, &AMHProjectCharacter::CancelCurrentAction);
        }

        if (SpecialAction)
            EnhancedInputComponent->BindAction(SpecialAction, ETriggerEvent::Started, this, &AMHProjectCharacter::SpecialAttack);
    }
}

// 이동 처리
void AMHProjectCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>(); // 입력 값에서 2D 벡터 추출

    // 공격 중에는 이동 불가
    if (bIsAttacking) return;

    // 컨트롤러 방향으로 이동
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);

        // 이동 중일 때 상태 변경
        if (MovementVector.SizeSquared() > 0 && CurrentState == ECharacterState::Idle)
        {
            SetCharacterState(ECharacterState::Moving);
        }
        else if (MovementVector.SizeSquared() == 0 && CurrentState == ECharacterState::Moving)
        {
            SetCharacterState(ECharacterState::Idle);
        }
    }
}

// 시야 전환 처리
void AMHProjectCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>(); // 입력 값에서 2D 벡터 추출

    // 공격 중이어도 시야 전환은 허용
    if (Controller)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

// 공격 처리
void AMHProjectCharacter::Attack()
{
    // 공격 불가능한 상태라면 리턴
    if (!bCanNextAttack || !HasEnoughStamina(AttackStaminaCost)) return;

    // 현재 상태가 공격 캔슬이 가능한지 확인
    if (!CanCancelIntoAction(ECharacterState::Attacking)) return;

    // 스태미나 소모
    ConsumeStamina(AttackStaminaCost);

    bIsAttacking = true;
    SetCharacterState(ECharacterState::Attacking);
    CurrentAttackState = EAttackState::NormalAttack;

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (!AttackMontage) return;

        // 무기 타입에 따른 속도 보정
        float PlayRate = GetWeaponSpeedModifier();

        // 콤보 상태에 따라 애니메이션 재생
        FName SectionName;
        switch (ComboCount)
        {
        case 0:
            ComboCount = 1;
            SectionName = FName("Default");
            break;
        case 1:
            ComboCount = 2;
            SectionName = FName("Attack1");
            break;
        case 2:
            ComboCount = 0; // 마지막 공격 후 콤보 리셋
            SectionName = FName("Attack2");
            break;
        }

        // 무기 타입별 접두사 추가 (예: GS_Default, LS_Attack1 등)
        FString WeaponPrefix;
        switch (CurrentWeaponType)
        {
        case EWeaponType::GreatSword:
            WeaponPrefix = "GS_";
            break;
        case EWeaponType::LongSword:
            WeaponPrefix = "LS_";
            break;
        case EWeaponType::DualBlades:
            WeaponPrefix = "DB_";
            break;
        case EWeaponType::HammerAxe:
            WeaponPrefix = "HA_";
            break;
        }

        FName WeaponSection = FName(*(WeaponPrefix + SectionName.ToString()));

        // 해당 섹션이 몽타주에 존재하는지 확인
        if (AttackMontage->IsValidSectionName(WeaponSection))
        {
            PlayAnimMontage(AttackMontage, PlayRate, WeaponSection);
        }
        else
        {
            // 폴백으로 기본 섹션 사용
            PlayAnimMontage(AttackMontage, PlayRate, SectionName);
        }

        GetWorldTimerManager().SetTimer(ComboTimer, this, &AMHProjectCharacter::ResetCombo, fCombotime, false);

        UE_LOG(LogTemp, Warning, TEXT("Playing attack animation - ComboCount: %d, Weapon: %s"),
            ComboCount, *WeaponPrefix);

        bCanNextAttack = false; // 다음 공격 불가능 상태로 설정
    }
}

// 콤보 리셋 함수
void AMHProjectCharacter::ResetCombo()
{
    ComboCount = 0; // 콤보 카운트 초기화
    bCanNextAttack = true; // 다음 공격 가능 상태로 설정
    bIsAttacking = false;

    // 공격 상태에서만 Idle로 변경
    if (CurrentState == ECharacterState::Attacking)
    {
        SetCharacterState(ECharacterState::Idle);
    }
}

// 다음 공격 가능 상태 활성화
void AMHProjectCharacter::EnableNextAttack()
{
    bCanNextAttack = true;
    bIsAttacking = false;
}

// 다음 공격 가능 상태 비활성화
void AMHProjectCharacter::DisableNextAttack()
{
    bCanNextAttack = false;
}

// 스태미나 소비 함수
void AMHProjectCharacter::ConsumeStamina(float Amount)
{
    CurrentStamina = FMath::Max(0.0f, CurrentStamina - Amount);
}

// 스태미나 재생 함수
void AMHProjectCharacter::RegenStamina()
{
    // 공격/회피 중에는 스태미나 회복 속도 감소
    if (CurrentState == ECharacterState::Attacking || CurrentState == ECharacterState::Dodging)
    {
        CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenRate * 0.1f));
    }
    else
    {
        CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenRate * 0.2f));
    }
}

// 스태미나 확인 함수
bool AMHProjectCharacter::HasEnoughStamina(float Cost)
{
    return CurrentStamina >= Cost;
}

// 스태미나 재생 업데이트
void AMHProjectCharacter::UpdateStaminaRegen()
{
    // 이미 타이머에서 호출 중
}

// 회피 기능
void AMHProjectCharacter::Dodge()
{
    // 회피 불가능한 상태라면 리턴
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned ||
        !HasEnoughStamina(DodgeStaminaCost)) return;

    // 현재 액션 캔슬 (애니메이션 중이라면)
    CancelCurrentAction();

    // 스태미나 소모
    ConsumeStamina(DodgeStaminaCost);

    // 상태 변경
    SetCharacterState(ECharacterState::Dodging);

    // 회피 방향 계산 (입력 방향 기준)
    FVector DodgeDirection = FVector::ZeroVector;
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // 마지막 이동 입력 방향 사용
        if (GetLastMovementInputVector().SizeSquared() > 0)
        {
            DodgeDirection = GetLastMovementInputVector().GetSafeNormal();
        }
        else
        {
            // 입력이 없으면 뒤로 회피
            DodgeDirection = -ForwardDirection;
        }
    }

    // 회피 애니메이션 재생
    if (DodgeMontage)
    {
        PlayAnimMontage(DodgeMontage, 1.0f);
    }

    // 회피 이동 적용 (물리적 움직임)
    const float DodgeStrength = 800.0f;
    LaunchCharacter(DodgeDirection * DodgeStrength, true, true);

    // 회피 종료 후 상태 복구를 위한 타이머 설정
    FTimerHandle DodgeTimerHandle;
    float DodgeDuration = DodgeMontage ? DodgeMontage->GetPlayLength() : 0.5f;
    GetWorldTimerManager().SetTimer(DodgeTimerHandle, [this]()
        {
            SetCharacterState(ECharacterState::Idle);
        }, DodgeDuration, false);
}

// 가드 기능
void AMHProjectCharacter::Guard()
{
    // 가드 불가능한 상태라면 리턴
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned ||
        CurrentState == ECharacterState::Attacking) return;

    // 상태 변경
    SetCharacterState(ECharacterState::Guarding);

    // 가드 애니메이션 재생
    if (GuardMontage)
    {
        PlayAnimMontage(GuardMontage, 1.0f);
    }

    // 가드 중에는 이동 속도 감소
    GetCharacterMovement()->MaxWalkSpeed = 200.0f;
}

// 충전 공격 시작
void AMHProjectCharacter::StartChargeAttack()
{
    // 충전 공격 불가능한 상태라면 일반 공격으로 전환
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned ||
        !HasEnoughStamina(AttackStaminaCost))
    {
        Attack();
        return;
    }

    // 충전 시작
    bIsCharging = true;
    ChargeTime = 0.0f;

    // 충전 상태 설정
    SetCharacterState(ECharacterState::Attacking);
    CurrentAttackState = EAttackState::ChargedAttack;

    // 충전 애니메이션 시작 (루프)
    if (ChargedAttackMontage)
    {
        PlayAnimMontage(ChargedAttackMontage, 1.0f, FName("Start"));
    }

    // 충전 중에는 이동 속도 감소
    GetCharacterMovement()->MaxWalkSpeed = 100.0f;
}

// 충전 공격 해제
void AMHProjectCharacter::ReleaseChargeAttack()
{
    // 충전 중이 아니면 무시
    if (!bIsCharging) return;

    // 충전 종료
    bIsCharging = false;

    // 최소 충전 시간 확인
    if (ChargeTime < 0.5f)
    {
        // 충전 시간이 짧으면 일반 공격으로 전환
        Attack();
        return;
    }

    // 스태미나 소모 (충전 시간에 비례)
    float ChargeRatio = FMath::Min(ChargeTime / MaxChargeTime, 1.0f);
    float StaminaCost = AttackStaminaCost + (AttackStaminaCost * ChargeRatio);
    ConsumeStamina(StaminaCost);

    // 충전 공격 애니메이션 재생
    if (ChargedAttackMontage)
    {
        // 충전 수준에 따라 다른 섹션 재생
        FName SectionName;
        if (ChargeRatio > 0.8f)
        {
            SectionName = FName("FullCharge");
        }
        else if (ChargeRatio > 0.4f)
        {
            SectionName = FName("MediumCharge");
        }
        else
        {
            SectionName = FName("LowCharge");
        }

        PlayAnimMontage(ChargedAttackMontage, 1.0f, SectionName);
    }

    // 공격 종료 타이머 설정
    float AttackDuration = ChargedAttackMontage ? ChargedAttackMontage->GetPlayLength() : 1.0f;
    GetWorldTimerManager().SetTimer(ComboTimer, this, &AMHProjectCharacter::ResetCombo, AttackDuration, false);

    // 이동 속도 복구 타이머
    FTimerHandle MoveSpeedTimerHandle;
    GetWorldTimerManager().SetTimer(MoveSpeedTimerHandle, [this]()
        {
            GetCharacterMovement()->MaxWalkSpeed = 500.0f;
        }, 0.5f, false);
}

// 특수 공격 기능
void AMHProjectCharacter::SpecialAttack()
{
    // 특수 공격 불가능한 상태라면 리턴
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned ||
        !HasEnoughStamina(AttackStaminaCost * 1.5f)) return;

    // 스태미나 소모
    ConsumeStamina(AttackStaminaCost * 1.5f);

    // 상태 변경
    SetCharacterState(ECharacterState::Attacking);
    CurrentAttackState = EAttackState::SpecialAttack;

    // 무기별 특수 공격 애니메이션 재생
    if (SpecialAttackMontage)
    {
        FName SectionName;
        switch (CurrentWeaponType)
        {
        case EWeaponType::GreatSword:
            SectionName = FName("GS_Special");
            break;
        case EWeaponType::LongSword:
            SectionName = FName("LS_Special");
            break;
        case EWeaponType::DualBlades:
            SectionName = FName("DB_Special");
            break;
        case EWeaponType::HammerAxe:
            SectionName = FName("HA_Special");
            break;
        }

        PlayAnimMontage(SpecialAttackMontage, 1.0f, SectionName);
    }

    // 특수 공격 종료 타이머 설정
    float AttackDuration = SpecialAttackMontage ? SpecialAttackMontage->GetPlayLength() : 1.5f;
    GetWorldTimerManager().SetTimer(ComboTimer, this, &AMHProjectCharacter::ResetCombo, AttackDuration, false);
}

// 현재 액션 캔슬
void AMHProjectCharacter::CancelCurrentAction()
{
    // 캔슬 불가능한 상태라면 리턴
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned) return;

    // 현재 재생중인 몽타주 중지
    StopAnimMontage();

    // 상태 초기화
    if (CurrentState == ECharacterState::Guarding)
    {
        // 가드 해제 시 이동 속도 복구
        GetCharacterMovement()->MaxWalkSpeed = 500.0f;
    }

    // 충전 중이라면 해제
    if (bIsCharging)
    {
        bIsCharging = false;
        GetCharacterMovement()->MaxWalkSpeed = 500.0f;
    }

    // 콤보 리셋
    ResetCombo();

    // 상태 변경
    SetCharacterState(ECharacterState::Idle);
}

// 액션 캔슬 가능 여부 확인
bool AMHProjectCharacter::CanCancelIntoAction(ECharacterState NewState)
{
    // 현재 상태에 따른 캔슬 가능 여부
    switch (CurrentState)
    {
    case ECharacterState::Idle:
    case ECharacterState::Moving:
        return true;

    case ECharacterState::Attacking:
        // 공격 중에는 캔슬 가능 플래그가 활성화됐을 때만 캔슬 가능
        return bCanCancelAttack;

    case ECharacterState::Dodging:
        // 회피 중에는 회피 종료 직전에만 캔슬 가능 (에님 노티파이로 구현)
        return false;

    case ECharacterState::Guarding:
        // 가드 중에는 다른 액션으로 자유롭게 캔슬 가능
        return true;

    default:
        return false;
    }
}

// 캐릭터 상태 설정
void AMHProjectCharacter::SetCharacterState(ECharacterState NewState)
{
    ECharacterState PrevState = CurrentState;
    CurrentState = NewState;

    // 상태 변경 로그
    UE_LOG(LogTemp, Log, TEXT("Character state changed: %s -> %s"),
        *UEnum::GetValueAsString(PrevState),
        *UEnum::GetValueAsString(CurrentState));

    // 상태에 따른 추가 효과
    switch (NewState)
    {
    case ECharacterState::Idle:
        bIsAttacking = false;
        bCanNextAttack = true;
        break;

    case ECharacterState::Attacking:
        bIsAttacking = true;
        break;

    default:
        break;
    }
}

// 무기 변경 함수
void AMHProjectCharacter::SwitchWeapon(EWeaponType NewWeaponType)
{
    // 무기 변경이 가능한 상태인지 확인
    if (CurrentState == ECharacterState::Attacking ||
        CurrentState == ECharacterState::Dodging ||
        CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned) return;

    CurrentWeaponType = NewWeaponType;

    // 무기 메시 및 속성 업데이트
    UpdateWeaponVisuals();

    // 무기 타입별 데미지 및 효과 설정
    switch (NewWeaponType)
    {
    case EWeaponType::GreatSword:
        NormalAttackDamage = 40.0f;
        ChargedAttackDamage = 80.0f;
        SpecialAttackDamage = 100.0f;
        GetCharacterMovement()->MaxWalkSpeed = 400.0f; // 대검은 느림
        break;

    case EWeaponType::LongSword:
        NormalAttackDamage = 25.0f;
        ChargedAttackDamage = 50.0f;
        SpecialAttackDamage = 80.0f;
        GetCharacterMovement()->MaxWalkSpeed = 500.0f; // 기본 속도
        break;

    case EWeaponType::DualBlades:
        NormalAttackDamage = 15.0f;
        ChargedAttackDamage = 30.0f;
        SpecialAttackDamage = 60.0f;
        GetCharacterMovement()->MaxWalkSpeed = 600.0f; // 쌍검은 빠름
        break;

    case EWeaponType::HammerAxe:
        NormalAttackDamage = 35.0f;
        ChargedAttackDamage = 70.0f;
        SpecialAttackDamage = 90.0f;
        GetCharacterMovement()->MaxWalkSpeed = 450.0f; // 해머/도끼는 약간 느림
        break;
    }

    // 무기 변경 로그
    UE_LOG(LogTemp, Log, TEXT("Weapon changed to: %s"),
        *UEnum::GetValueAsString(CurrentWeaponType));
}

// 무기별 공격 속도 보정 요소 반환
float AMHProjectCharacter::GetWeaponSpeedModifier()
{
    switch (CurrentWeaponType)
    {
    case EWeaponType::GreatSword:
        return 0.7f; // 대검은 느림

    case EWeaponType::LongSword:
        return 1.0f; // 기본 속도

    case EWeaponType::DualBlades:
        return 1.3f; // 쌍검은 빠름

    case EWeaponType::HammerAxe:
        return 0.8f; // 해머/도끼는 약간 느림

    default:
        return 1.0f;
    }
}

// 무기별 애니메이션 재생
void AMHProjectCharacter::PlayWeaponSpecificAnimation(UAnimMontage* BaseMontage)
{
    if (!BaseMontage) return;

    FString WeaponPrefix;
    switch (CurrentWeaponType)
    {
    case EWeaponType::GreatSword:
        WeaponPrefix = "GS_";
        break;
    case EWeaponType::LongSword:
        WeaponPrefix = "LS_";
        break;
    case EWeaponType::DualBlades:
        WeaponPrefix = "DB_";
        break;
    case EWeaponType::HammerAxe:
        WeaponPrefix = "HA_";
        break;
    }

    FName SectionName = FName(*WeaponPrefix);

    // 해당 섹션이 몽타주에 존재하는지 확인
    if (BaseMontage->IsValidSectionName(SectionName))
    {
        PlayAnimMontage(BaseMontage, GetWeaponSpeedModifier(), SectionName);
    }
    else
    {
        // 폴백으로 기본 섹션 사용
        PlayAnimMontage(BaseMontage, GetWeaponSpeedModifier());
    }
}

// 무기 시각 효과 업데이트
void AMHProjectCharacter::UpdateWeaponVisuals()
{
    if (!WeaponMesh) return;

    // 무기 타입에 따라 다른 메시와 스케일 적용
    UStaticMesh* NewMesh = nullptr;
    FVector Scale = FVector(1.0f);

    // 쌍검용 왼손 무기 변수를 미리 선언
    UStaticMeshComponent* LeftWeapon = nullptr;

    switch (CurrentWeaponType)
    {
    case EWeaponType::GreatSword:
        // 대검 메시 찾기
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/GreatSword/GreatSword_Mesh"));
        Scale = FVector(1.2f);
        break;

    case EWeaponType::LongSword:
        // 태도 메시 찾기
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/LongSword/LongSword_Mesh"));
        Scale = FVector(1.0f);
        break;

    case EWeaponType::DualBlades:
    {  // 중괄호로 스코프를 제한
        // 쌍검 메시 찾기 (주무기)
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/DualBlades/DualBlade_Mesh"));
        Scale = FVector(0.8f);

        // 쌍검인 경우 왼손에도 무기 추가
        LeftWeapon = NewObject<UStaticMeshComponent>(this, TEXT("LeftWeapon"));
        if (LeftWeapon)
        {
            LeftWeapon->RegisterComponent();
            LeftWeapon->SetStaticMesh(NewMesh);
            LeftWeapon->SetupAttachment(GetMesh(), FName("hand_lSocket")); // 왼손 소켓
            LeftWeapon->SetRelativeScale3D(Scale);
            LeftWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
    break;

    case EWeaponType::HammerAxe:
        // 해머/도끼 메시 찾기
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/Hammer/Hammer_Mesh"));
        Scale = FVector(1.1f);
        break;
    }

    if (!NewMesh) 
    {
        // 메시를 찾을 수 없는 경우 기본 큐브 메시 등 사용
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
        UE_LOG(LogTemp, Warning, TEXT("해머 메시를 찾을 수 없어 기본 메시를 사용합니다."));
    }

    if (NewMesh)
    {
        WeaponMesh->SetStaticMesh(NewMesh);
        WeaponMesh->SetRelativeScale3D(Scale);
    }
}

// 무기 트레이스 시작
void AMHProjectCharacter::StartWeaponTrace()
{
    bIsWeaponTracing = true;
    HitActors.Empty(); // 히트 액터 배열 초기화
}

// 무기 트레이스 종료
void AMHProjectCharacter::EndWeaponTrace()
{
    bIsWeaponTracing = false;
}

// 무기 트레이스 수행
void AMHProjectCharacter::PerformWeaponTrace()
{
    if (!bIsWeaponTracing) return;

    // 무기 위치 정보 가져오기
    FVector TraceStart = WeaponMesh->GetComponentLocation();
    FVector WeaponForward = WeaponMesh->GetForwardVector();

    // 무기 타입별 트레이스 길이 조정
    float TraceLength = 0.0f;
    switch (CurrentWeaponType)
    {
    case EWeaponType::GreatSword:
        TraceLength = 150.0f;
        break;
    case EWeaponType::LongSword:
        TraceLength = 200.0f;
        break;
    case EWeaponType::DualBlades:
        TraceLength = 100.0f;
        break;
    case EWeaponType::HammerAxe:
        TraceLength = 120.0f;
        break;
    default:
        TraceLength = 100.0f;
    }

    FVector TraceEnd = TraceStart + (WeaponForward * TraceLength);

    // 트레이스 설정
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // 자기 자신 무시
    QueryParams.bTraceComplex = true; // 복잡한 콜리전 사용
    QueryParams.bReturnPhysicalMaterial = true; // 물리 재질 반환

    // 트레이스 실행
    FHitResult Hit;
    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Pawn, QueryParams);

    // 디버그 시각화 (필요시)
    if (false) // 디버그 모드에서만 활성화
    {
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1.0f, 0, 1.0f);
    }

    // 히트 처리
    if (bHit)
    {
        AActor* HitActor = Hit.GetActor();

        // 이미 히트한 액터가 아닌 경우에만 처리
        if (HitActor && !HitActors.Contains(HitActor))
        {
            // 히트 액터 배열에 추가
            HitActors.Add(HitActor);

            // 히트 이펙트 생성
            SpawnHitEffect(Hit);

            // 히트 정보 구성
            FHitInfo HitInfo;

            // 공격 상태에 따른 데미지 설정
            switch (CurrentAttackState)
            {
            case EAttackState::NormalAttack:
                HitInfo.BaseDamage = NormalAttackDamage;
                HitInfo.StunChance = 0.1f;
                HitInfo.KnockbackForce = 100.0f;
                break;

            case EAttackState::ChargedAttack:
            {
                // 충전 수준에 따른 데미지 계산
                float ChargeRatio = FMath::Min(ChargeTime / MaxChargeTime, 1.0f);
                HitInfo.BaseDamage = NormalAttackDamage + (ChargedAttackDamage - NormalAttackDamage) * ChargeRatio;
                HitInfo.StunChance = 0.3f + (0.3f * ChargeRatio);
                HitInfo.KnockbackForce = 300.0f + (500.0f * ChargeRatio);
            }
            break;

            case EAttackState::SpecialAttack:
                HitInfo.BaseDamage = SpecialAttackDamage;
                HitInfo.StunChance = 0.5f;
                HitInfo.KnockbackForce = 800.0f;
                break;

            default:
                HitInfo.BaseDamage = NormalAttackDamage;
                HitInfo.StunChance = 0.1f;
                HitInfo.KnockbackForce = 100.0f;
            }

            // 무기 속성 데미지 추가
            HitInfo.ElementalDamage = ElementalDamage;

            // 히트 위치 및 방향 설정
            HitInfo.HitLocation = Hit.Location;
            HitInfo.HitDirection = (Hit.Location - TraceStart).GetSafeNormal();
            HitInfo.SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
            HitInfo.HitActor = HitActor;

            // 데미지 적용
            ApplyDamage(HitActor, HitInfo);

            // 무기별 특수 효과 적용
            ApplyWeaponSpecialEffect(HitActor);
        }
    }
}

// 히트 이펙트 생성
void AMHProjectCharacter::SpawnHitEffect(const FHitResult& Hit)
{
    if (!GetWorld()) return;

    // 파티클 효과
    if (HitParticle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            HitParticle,
            Hit.Location,
            Hit.Normal.Rotation(),
            true
        );
    }

    // 사운드 효과
    if (HitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            HitSound,
            Hit.Location,
            1.0f,
            1.0f,
            0.0f,
            nullptr,
            nullptr
        );
    }

    // 데칼 (히트 마크)
    if (HitDecalMaterial)
    {
        UGameplayStatics::SpawnDecalAtLocation(
            GetWorld(),
            HitDecalMaterial,
            FVector(10.0f),
            Hit.Location,
            Hit.Normal.Rotation(),
            5.0f
        );
    }

    // 무기 속성에 따른 추가 효과
    switch (WeaponElement)
    {
    case EElementalType::Fire:
        // 불 파티클
        // 예: UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FireParticle, ...);
        break;

    case EElementalType::Water:
        // 물 파티클
        break;

    case EElementalType::Thunder:
        // 번개 파티클
        break;

    case EElementalType::Ice:
        // 얼음 파티클
        break;

    case EElementalType::Dragon:
        // 용 속성 파티클
        break;

    default:
        break;
    }
}

// 데미지 적용
void AMHProjectCharacter::ApplyDamage(AActor* HitActor, const FHitInfo& HitInfo)
{
    if (!HitActor) return;

    // 기본 데미지
    float TotalDamage = HitInfo.BaseDamage;

    // 속성 데미지 추가
    if (WeaponElement != EElementalType::None && HitInfo.ElementalDamage > 0)
    {
        TotalDamage += HitInfo.ElementalDamage;
    }

    // 데미지 적용
    UGameplayStatics::ApplyDamage(
        HitActor,
        TotalDamage,
        GetController(),
        this,
        UDamageType::StaticClass()
    );

    // 넉백 적용 (물리 컴포넌트가 있는 경우)
    UPrimitiveComponent* HitPrimitive = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
    if (HitPrimitive && HitPrimitive->IsSimulatingPhysics())
    {
        HitPrimitive->AddImpulseAtLocation(
            HitInfo.HitDirection * HitInfo.KnockbackForce,
            HitInfo.HitLocation
        );
    }

    // 스턴 적용 (확률 기반)
    if (FMath::FRand() <= HitInfo.StunChance)
    {
        // 몬스터나 적 캐릭터에 스턴 상태 적용
        // 실제 구현에서는 인터페이스나 다른 방식으로 처리할 수 있음
        /*
        IGameplayInterface* GameplayInterface = Cast<IGameplayInterface>(HitActor);
        if (GameplayInterface)
        {
            // 스턴 함수 호출
            GameplayInterface->Execute_ApplyStun(HitActor, 2.0f); // 2초 스턴
        }
        */
    }

    // 히트 로그
    UE_LOG(LogTemp, Log, TEXT("Hit Actor: %s, Damage: %.2f, Elemental: %s"),
        *HitActor->GetName(),
        TotalDamage,
        *UEnum::GetValueAsString(WeaponElement));
}

// 무기별 특수 효과 적용
void AMHProjectCharacter::ApplyWeaponSpecialEffect(AActor* HitActor)
{
    if (!HitActor) return;

    switch (CurrentWeaponType)
    {
    case EWeaponType::GreatSword:
        // 대검 타격 시 경직 증가
        break;

    case EWeaponType::LongSword:
        // 태도 타격 시 게이지 상승
        break;

    case EWeaponType::DualBlades:
        // 쌍검 타격 시 연속 공격 보너스
        break;

    case EWeaponType::HammerAxe:
        // 해머/도끼 타격 시 방어력 감소 효과
        break;

    default:
        break;
    }

    // 속성 효과 적용
    switch (WeaponElement)
    {
    case EElementalType::Fire:
        // 화염 상태이상 적용
        break;

    case EElementalType::Water:
        // 젖음 상태이상 적용
        break;

    case EElementalType::Thunder:
        // 감전 상태이상 적용
        break;

    case EElementalType::Ice:
        // 빙결 상태이상 적용
        break;

    case EElementalType::Dragon:
        // 용 속성 약화 적용
        break;

    default:
        break;
    }
}