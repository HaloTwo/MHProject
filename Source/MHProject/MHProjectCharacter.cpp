// Copyright Epic Games, Inc. All Rights Reserved.

// �ʿ��� ��� ���� ����
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

// �α� ī�װ� ����
DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMHProjectCharacter

// Ŭ���� ������
AMHProjectCharacter::AMHProjectCharacter()
{
    // ĸ�� ������Ʈ �ʱ�ȭ (ĳ������ �浹 ũ�� ����)
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // ��Ʈ�ѷ� ȸ���� ���� ĳ���Ͱ� ȸ������ �ʵ��� ����
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // ĳ���� �̵� ���� ����
    GetCharacterMovement()->bOrientRotationToMovement = true; // �Է� �������� �̵�
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ȸ�� �ӵ�
    GetCharacterMovement()->JumpZVelocity = 700.f; // ���� �ӵ�
    GetCharacterMovement()->AirControl = 0.35f; // ���߿����� �����
    GetCharacterMovement()->MaxWalkSpeed = 500.f; // �ִ� �ȱ� �ӵ�
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f; // �ּ� �Ƴ��α� �ȱ� �ӵ�
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f; // �ȱ� ����
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f; // ���� ����

    // ī�޶� �� ���� (ĳ���� �ڷ� ������� ī�޶� �Ÿ� ����)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f; // ī�޶�� ĳ���� ���� �Ÿ�
    CameraBoom->bUsePawnControlRotation = true; // ��Ʈ�ѷ� ���⿡ ���� ī�޶� ȸ��

    // �ȷο� ī�޶� ����
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // ī�޶� �տ� ����
    FollowCamera->bUsePawnControlRotation = false; // ī�޶� �� ȸ���� ���� ȸ������ ����

    // ���¹̳� �ý��� �ʱ�ȭ
    MaxStamina = 100.0f;
    CurrentStamina = MaxStamina;
    StaminaRegenRate = 10.0f;
    AttackStaminaCost = 20.0f;
    DodgeStaminaCost = 15.0f;

    // �⺻ ���� Ÿ�� ����
    CurrentWeaponType = EWeaponType::HammerAxe;

    // �⺻ ���� ����
    CurrentState = ECharacterState::Idle;
    CurrentAttackState = EAttackState::None;

    // ���� ���� �ʱ�ȭ
    bIsAttacking = false;
    bCanNextAttack = true;
    bCanCancelAttack = false;

    // ���� ���� �ʱ�ȭ
    bIsCharging = false;
    ChargeTime = 0.0f;
    MaxChargeTime = 3.0f;

    // ���� �޽� ����
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), FName("hand_rSocket")); // ������ ���Ͽ� ����
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �ݸ��� ��Ȱ��ȭ

    // ���� �Ӽ� �ʱ�ȭ
    WeaponElement = EElementalType::None;
    ElementalDamage = 0.0f;

    // ���� Ʈ���̽� �ʱ�ȭ
    bIsWeaponTracing = false;

    // ���� ������ �ʱ�ȭ
    NormalAttackDamage = 20.0f;
    ChargedAttackDamage = 40.0f;
    SpecialAttackDamage = 60.0f;
}

// ���� ���� �� ȣ��
void AMHProjectCharacter::BeginPlay()
{
    // �θ� Ŭ������ BeginPlay ȣ��
    Super::BeginPlay();

    // ���¹̳� ��� Ÿ�̸� ����
    GetWorldTimerManager().SetTimer(StaminaRegenTimer, this, &AMHProjectCharacter::RegenStamina, 0.1f, true);

    // ���� Ÿ�Կ� ���� �޽� �� ������ ����
    UpdateWeaponVisuals();
}

// Tick �Լ� �߰� (�� ������ ȣ��)
void AMHProjectCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ���� ���� ������Ʈ
    if (bIsCharging)
    {
        ChargeTime += DeltaTime;
        if (ChargeTime >= MaxChargeTime)
        {
            // �ִ� ���� �޼� ȿ�� (������)
            // ��: ����Ʈ ����, ���� ��� ��
        }
    }

    // ���¹̳��� �ִ�ġ���� ���� ���� ������Ʈ
    if (CurrentStamina < MaxStamina)
    {
        UpdateStaminaRegen();
    }

    // ���� Ʈ���̽� ������Ʈ
    if (bIsWeaponTracing)
    {
        PerformWeaponTrace();
    }
}

//////////////////////////////////////////////////////////////////////////
// Input

// �÷��̾� �Է� ����
void AMHProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // �Է� ���� ���ؽ�Ʈ �߰�
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0); // �⺻ ���� ���ؽ�Ʈ �߰�
        }
    }

    // �Է� �׼� ���ε�
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // ���� ���ε�
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // �̵� ���ε�
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMHProjectCharacter::Move);

        // �þ� ��ȯ ���ε�
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMHProjectCharacter::Look);

        // ���� ���ε�
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AMHProjectCharacter::StartChargeAttack);
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AMHProjectCharacter::ReleaseChargeAttack);

        // ���ο� �׼� ���ε�
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

// �̵� ó��
void AMHProjectCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>(); // �Է� ������ 2D ���� ����

    // ���� �߿��� �̵� �Ұ�
    if (bIsAttacking) return;

    // ��Ʈ�ѷ� �������� �̵�
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);

        // �̵� ���� �� ���� ����
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

// �þ� ��ȯ ó��
void AMHProjectCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>(); // �Է� ������ 2D ���� ����

    // ���� ���̾ �þ� ��ȯ�� ���
    if (Controller)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

// ���� ó��
void AMHProjectCharacter::Attack()
{
    // ���� �Ұ����� ���¶�� ����
    if (!bCanNextAttack || !HasEnoughStamina(AttackStaminaCost)) return;

    // ���� ���°� ���� ĵ���� �������� Ȯ��
    if (!CanCancelIntoAction(ECharacterState::Attacking)) return;

    // ���¹̳� �Ҹ�
    ConsumeStamina(AttackStaminaCost);

    bIsAttacking = true;
    SetCharacterState(ECharacterState::Attacking);
    CurrentAttackState = EAttackState::NormalAttack;

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (!AttackMontage) return;

        // ���� Ÿ�Կ� ���� �ӵ� ����
        float PlayRate = GetWeaponSpeedModifier();

        // �޺� ���¿� ���� �ִϸ��̼� ���
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
            ComboCount = 0; // ������ ���� �� �޺� ����
            SectionName = FName("Attack2");
            break;
        }

        // ���� Ÿ�Ժ� ���λ� �߰� (��: GS_Default, LS_Attack1 ��)
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

        // �ش� ������ ��Ÿ�ֿ� �����ϴ��� Ȯ��
        if (AttackMontage->IsValidSectionName(WeaponSection))
        {
            PlayAnimMontage(AttackMontage, PlayRate, WeaponSection);
        }
        else
        {
            // �������� �⺻ ���� ���
            PlayAnimMontage(AttackMontage, PlayRate, SectionName);
        }

        GetWorldTimerManager().SetTimer(ComboTimer, this, &AMHProjectCharacter::ResetCombo, fCombotime, false);

        UE_LOG(LogTemp, Warning, TEXT("Playing attack animation - ComboCount: %d, Weapon: %s"),
            ComboCount, *WeaponPrefix);

        bCanNextAttack = false; // ���� ���� �Ұ��� ���·� ����
    }
}

// �޺� ���� �Լ�
void AMHProjectCharacter::ResetCombo()
{
    ComboCount = 0; // �޺� ī��Ʈ �ʱ�ȭ
    bCanNextAttack = true; // ���� ���� ���� ���·� ����
    bIsAttacking = false;

    // ���� ���¿����� Idle�� ����
    if (CurrentState == ECharacterState::Attacking)
    {
        SetCharacterState(ECharacterState::Idle);
    }
}

// ���� ���� ���� ���� Ȱ��ȭ
void AMHProjectCharacter::EnableNextAttack()
{
    bCanNextAttack = true;
    bIsAttacking = false;
}

// ���� ���� ���� ���� ��Ȱ��ȭ
void AMHProjectCharacter::DisableNextAttack()
{
    bCanNextAttack = false;
}

// ���¹̳� �Һ� �Լ�
void AMHProjectCharacter::ConsumeStamina(float Amount)
{
    CurrentStamina = FMath::Max(0.0f, CurrentStamina - Amount);
}

// ���¹̳� ��� �Լ�
void AMHProjectCharacter::RegenStamina()
{
    // ����/ȸ�� �߿��� ���¹̳� ȸ�� �ӵ� ����
    if (CurrentState == ECharacterState::Attacking || CurrentState == ECharacterState::Dodging)
    {
        CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenRate * 0.1f));
    }
    else
    {
        CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenRate * 0.2f));
    }
}

// ���¹̳� Ȯ�� �Լ�
bool AMHProjectCharacter::HasEnoughStamina(float Cost)
{
    return CurrentStamina >= Cost;
}

// ���¹̳� ��� ������Ʈ
void AMHProjectCharacter::UpdateStaminaRegen()
{
    // �̹� Ÿ�̸ӿ��� ȣ�� ��
}

// ȸ�� ���
void AMHProjectCharacter::Dodge()
{
    // ȸ�� �Ұ����� ���¶�� ����
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned ||
        !HasEnoughStamina(DodgeStaminaCost)) return;

    // ���� �׼� ĵ�� (�ִϸ��̼� ���̶��)
    CancelCurrentAction();

    // ���¹̳� �Ҹ�
    ConsumeStamina(DodgeStaminaCost);

    // ���� ����
    SetCharacterState(ECharacterState::Dodging);

    // ȸ�� ���� ��� (�Է� ���� ����)
    FVector DodgeDirection = FVector::ZeroVector;
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // ������ �̵� �Է� ���� ���
        if (GetLastMovementInputVector().SizeSquared() > 0)
        {
            DodgeDirection = GetLastMovementInputVector().GetSafeNormal();
        }
        else
        {
            // �Է��� ������ �ڷ� ȸ��
            DodgeDirection = -ForwardDirection;
        }
    }

    // ȸ�� �ִϸ��̼� ���
    if (DodgeMontage)
    {
        PlayAnimMontage(DodgeMontage, 1.0f);
    }

    // ȸ�� �̵� ���� (������ ������)
    const float DodgeStrength = 800.0f;
    LaunchCharacter(DodgeDirection * DodgeStrength, true, true);

    // ȸ�� ���� �� ���� ������ ���� Ÿ�̸� ����
    FTimerHandle DodgeTimerHandle;
    float DodgeDuration = DodgeMontage ? DodgeMontage->GetPlayLength() : 0.5f;
    GetWorldTimerManager().SetTimer(DodgeTimerHandle, [this]()
        {
            SetCharacterState(ECharacterState::Idle);
        }, DodgeDuration, false);
}

// ���� ���
void AMHProjectCharacter::Guard()
{
    // ���� �Ұ����� ���¶�� ����
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned ||
        CurrentState == ECharacterState::Attacking) return;

    // ���� ����
    SetCharacterState(ECharacterState::Guarding);

    // ���� �ִϸ��̼� ���
    if (GuardMontage)
    {
        PlayAnimMontage(GuardMontage, 1.0f);
    }

    // ���� �߿��� �̵� �ӵ� ����
    GetCharacterMovement()->MaxWalkSpeed = 200.0f;
}

// ���� ���� ����
void AMHProjectCharacter::StartChargeAttack()
{
    // ���� ���� �Ұ����� ���¶�� �Ϲ� �������� ��ȯ
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned ||
        !HasEnoughStamina(AttackStaminaCost))
    {
        Attack();
        return;
    }

    // ���� ����
    bIsCharging = true;
    ChargeTime = 0.0f;

    // ���� ���� ����
    SetCharacterState(ECharacterState::Attacking);
    CurrentAttackState = EAttackState::ChargedAttack;

    // ���� �ִϸ��̼� ���� (����)
    if (ChargedAttackMontage)
    {
        PlayAnimMontage(ChargedAttackMontage, 1.0f, FName("Start"));
    }

    // ���� �߿��� �̵� �ӵ� ����
    GetCharacterMovement()->MaxWalkSpeed = 100.0f;
}

// ���� ���� ����
void AMHProjectCharacter::ReleaseChargeAttack()
{
    // ���� ���� �ƴϸ� ����
    if (!bIsCharging) return;

    // ���� ����
    bIsCharging = false;

    // �ּ� ���� �ð� Ȯ��
    if (ChargeTime < 0.5f)
    {
        // ���� �ð��� ª���� �Ϲ� �������� ��ȯ
        Attack();
        return;
    }

    // ���¹̳� �Ҹ� (���� �ð��� ���)
    float ChargeRatio = FMath::Min(ChargeTime / MaxChargeTime, 1.0f);
    float StaminaCost = AttackStaminaCost + (AttackStaminaCost * ChargeRatio);
    ConsumeStamina(StaminaCost);

    // ���� ���� �ִϸ��̼� ���
    if (ChargedAttackMontage)
    {
        // ���� ���ؿ� ���� �ٸ� ���� ���
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

    // ���� ���� Ÿ�̸� ����
    float AttackDuration = ChargedAttackMontage ? ChargedAttackMontage->GetPlayLength() : 1.0f;
    GetWorldTimerManager().SetTimer(ComboTimer, this, &AMHProjectCharacter::ResetCombo, AttackDuration, false);

    // �̵� �ӵ� ���� Ÿ�̸�
    FTimerHandle MoveSpeedTimerHandle;
    GetWorldTimerManager().SetTimer(MoveSpeedTimerHandle, [this]()
        {
            GetCharacterMovement()->MaxWalkSpeed = 500.0f;
        }, 0.5f, false);
}

// Ư�� ���� ���
void AMHProjectCharacter::SpecialAttack()
{
    // Ư�� ���� �Ұ����� ���¶�� ����
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned ||
        !HasEnoughStamina(AttackStaminaCost * 1.5f)) return;

    // ���¹̳� �Ҹ�
    ConsumeStamina(AttackStaminaCost * 1.5f);

    // ���� ����
    SetCharacterState(ECharacterState::Attacking);
    CurrentAttackState = EAttackState::SpecialAttack;

    // ���⺰ Ư�� ���� �ִϸ��̼� ���
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

    // Ư�� ���� ���� Ÿ�̸� ����
    float AttackDuration = SpecialAttackMontage ? SpecialAttackMontage->GetPlayLength() : 1.5f;
    GetWorldTimerManager().SetTimer(ComboTimer, this, &AMHProjectCharacter::ResetCombo, AttackDuration, false);
}

// ���� �׼� ĵ��
void AMHProjectCharacter::CancelCurrentAction()
{
    // ĵ�� �Ұ����� ���¶�� ����
    if (CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned) return;

    // ���� ������� ��Ÿ�� ����
    StopAnimMontage();

    // ���� �ʱ�ȭ
    if (CurrentState == ECharacterState::Guarding)
    {
        // ���� ���� �� �̵� �ӵ� ����
        GetCharacterMovement()->MaxWalkSpeed = 500.0f;
    }

    // ���� ���̶�� ����
    if (bIsCharging)
    {
        bIsCharging = false;
        GetCharacterMovement()->MaxWalkSpeed = 500.0f;
    }

    // �޺� ����
    ResetCombo();

    // ���� ����
    SetCharacterState(ECharacterState::Idle);
}

// �׼� ĵ�� ���� ���� Ȯ��
bool AMHProjectCharacter::CanCancelIntoAction(ECharacterState NewState)
{
    // ���� ���¿� ���� ĵ�� ���� ����
    switch (CurrentState)
    {
    case ECharacterState::Idle:
    case ECharacterState::Moving:
        return true;

    case ECharacterState::Attacking:
        // ���� �߿��� ĵ�� ���� �÷��װ� Ȱ��ȭ���� ���� ĵ�� ����
        return bCanCancelAttack;

    case ECharacterState::Dodging:
        // ȸ�� �߿��� ȸ�� ���� �������� ĵ�� ���� (���� ��Ƽ���̷� ����)
        return false;

    case ECharacterState::Guarding:
        // ���� �߿��� �ٸ� �׼����� �����Ӱ� ĵ�� ����
        return true;

    default:
        return false;
    }
}

// ĳ���� ���� ����
void AMHProjectCharacter::SetCharacterState(ECharacterState NewState)
{
    ECharacterState PrevState = CurrentState;
    CurrentState = NewState;

    // ���� ���� �α�
    UE_LOG(LogTemp, Log, TEXT("Character state changed: %s -> %s"),
        *UEnum::GetValueAsString(PrevState),
        *UEnum::GetValueAsString(CurrentState));

    // ���¿� ���� �߰� ȿ��
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

// ���� ���� �Լ�
void AMHProjectCharacter::SwitchWeapon(EWeaponType NewWeaponType)
{
    // ���� ������ ������ �������� Ȯ��
    if (CurrentState == ECharacterState::Attacking ||
        CurrentState == ECharacterState::Dodging ||
        CurrentState == ECharacterState::KnockedDown ||
        CurrentState == ECharacterState::Stunned) return;

    CurrentWeaponType = NewWeaponType;

    // ���� �޽� �� �Ӽ� ������Ʈ
    UpdateWeaponVisuals();

    // ���� Ÿ�Ժ� ������ �� ȿ�� ����
    switch (NewWeaponType)
    {
    case EWeaponType::GreatSword:
        NormalAttackDamage = 40.0f;
        ChargedAttackDamage = 80.0f;
        SpecialAttackDamage = 100.0f;
        GetCharacterMovement()->MaxWalkSpeed = 400.0f; // ����� ����
        break;

    case EWeaponType::LongSword:
        NormalAttackDamage = 25.0f;
        ChargedAttackDamage = 50.0f;
        SpecialAttackDamage = 80.0f;
        GetCharacterMovement()->MaxWalkSpeed = 500.0f; // �⺻ �ӵ�
        break;

    case EWeaponType::DualBlades:
        NormalAttackDamage = 15.0f;
        ChargedAttackDamage = 30.0f;
        SpecialAttackDamage = 60.0f;
        GetCharacterMovement()->MaxWalkSpeed = 600.0f; // �ְ��� ����
        break;

    case EWeaponType::HammerAxe:
        NormalAttackDamage = 35.0f;
        ChargedAttackDamage = 70.0f;
        SpecialAttackDamage = 90.0f;
        GetCharacterMovement()->MaxWalkSpeed = 450.0f; // �ظ�/������ �ణ ����
        break;
    }

    // ���� ���� �α�
    UE_LOG(LogTemp, Log, TEXT("Weapon changed to: %s"),
        *UEnum::GetValueAsString(CurrentWeaponType));
}

// ���⺰ ���� �ӵ� ���� ��� ��ȯ
float AMHProjectCharacter::GetWeaponSpeedModifier()
{
    switch (CurrentWeaponType)
    {
    case EWeaponType::GreatSword:
        return 0.7f; // ����� ����

    case EWeaponType::LongSword:
        return 1.0f; // �⺻ �ӵ�

    case EWeaponType::DualBlades:
        return 1.3f; // �ְ��� ����

    case EWeaponType::HammerAxe:
        return 0.8f; // �ظ�/������ �ణ ����

    default:
        return 1.0f;
    }
}

// ���⺰ �ִϸ��̼� ���
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

    // �ش� ������ ��Ÿ�ֿ� �����ϴ��� Ȯ��
    if (BaseMontage->IsValidSectionName(SectionName))
    {
        PlayAnimMontage(BaseMontage, GetWeaponSpeedModifier(), SectionName);
    }
    else
    {
        // �������� �⺻ ���� ���
        PlayAnimMontage(BaseMontage, GetWeaponSpeedModifier());
    }
}

// ���� �ð� ȿ�� ������Ʈ
void AMHProjectCharacter::UpdateWeaponVisuals()
{
    if (!WeaponMesh) return;

    // ���� Ÿ�Կ� ���� �ٸ� �޽ÿ� ������ ����
    UStaticMesh* NewMesh = nullptr;
    FVector Scale = FVector(1.0f);

    // �ְ˿� �޼� ���� ������ �̸� ����
    UStaticMeshComponent* LeftWeapon = nullptr;

    switch (CurrentWeaponType)
    {
    case EWeaponType::GreatSword:
        // ��� �޽� ã��
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/GreatSword/GreatSword_Mesh"));
        Scale = FVector(1.2f);
        break;

    case EWeaponType::LongSword:
        // �µ� �޽� ã��
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/LongSword/LongSword_Mesh"));
        Scale = FVector(1.0f);
        break;

    case EWeaponType::DualBlades:
    {  // �߰�ȣ�� �������� ����
        // �ְ� �޽� ã�� (�ֹ���)
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/DualBlades/DualBlade_Mesh"));
        Scale = FVector(0.8f);

        // �ְ��� ��� �޼տ��� ���� �߰�
        LeftWeapon = NewObject<UStaticMeshComponent>(this, TEXT("LeftWeapon"));
        if (LeftWeapon)
        {
            LeftWeapon->RegisterComponent();
            LeftWeapon->SetStaticMesh(NewMesh);
            LeftWeapon->SetupAttachment(GetMesh(), FName("hand_lSocket")); // �޼� ����
            LeftWeapon->SetRelativeScale3D(Scale);
            LeftWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
    break;

    case EWeaponType::HammerAxe:
        // �ظ�/���� �޽� ã��
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/Hammer/Hammer_Mesh"));
        Scale = FVector(1.1f);
        break;
    }

    if (!NewMesh) 
    {
        // �޽ø� ã�� �� ���� ��� �⺻ ť�� �޽� �� ���
        NewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
        UE_LOG(LogTemp, Warning, TEXT("�ظ� �޽ø� ã�� �� ���� �⺻ �޽ø� ����մϴ�."));
    }

    if (NewMesh)
    {
        WeaponMesh->SetStaticMesh(NewMesh);
        WeaponMesh->SetRelativeScale3D(Scale);
    }
}

// ���� Ʈ���̽� ����
void AMHProjectCharacter::StartWeaponTrace()
{
    bIsWeaponTracing = true;
    HitActors.Empty(); // ��Ʈ ���� �迭 �ʱ�ȭ
}

// ���� Ʈ���̽� ����
void AMHProjectCharacter::EndWeaponTrace()
{
    bIsWeaponTracing = false;
}

// ���� Ʈ���̽� ����
void AMHProjectCharacter::PerformWeaponTrace()
{
    if (!bIsWeaponTracing) return;

    // ���� ��ġ ���� ��������
    FVector TraceStart = WeaponMesh->GetComponentLocation();
    FVector WeaponForward = WeaponMesh->GetForwardVector();

    // ���� Ÿ�Ժ� Ʈ���̽� ���� ����
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

    // Ʈ���̽� ����
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // �ڱ� �ڽ� ����
    QueryParams.bTraceComplex = true; // ������ �ݸ��� ���
    QueryParams.bReturnPhysicalMaterial = true; // ���� ���� ��ȯ

    // Ʈ���̽� ����
    FHitResult Hit;
    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Pawn, QueryParams);

    // ����� �ð�ȭ (�ʿ��)
    if (false) // ����� ��忡���� Ȱ��ȭ
    {
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1.0f, 0, 1.0f);
    }

    // ��Ʈ ó��
    if (bHit)
    {
        AActor* HitActor = Hit.GetActor();

        // �̹� ��Ʈ�� ���Ͱ� �ƴ� ��쿡�� ó��
        if (HitActor && !HitActors.Contains(HitActor))
        {
            // ��Ʈ ���� �迭�� �߰�
            HitActors.Add(HitActor);

            // ��Ʈ ����Ʈ ����
            SpawnHitEffect(Hit);

            // ��Ʈ ���� ����
            FHitInfo HitInfo;

            // ���� ���¿� ���� ������ ����
            switch (CurrentAttackState)
            {
            case EAttackState::NormalAttack:
                HitInfo.BaseDamage = NormalAttackDamage;
                HitInfo.StunChance = 0.1f;
                HitInfo.KnockbackForce = 100.0f;
                break;

            case EAttackState::ChargedAttack:
            {
                // ���� ���ؿ� ���� ������ ���
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

            // ���� �Ӽ� ������ �߰�
            HitInfo.ElementalDamage = ElementalDamage;

            // ��Ʈ ��ġ �� ���� ����
            HitInfo.HitLocation = Hit.Location;
            HitInfo.HitDirection = (Hit.Location - TraceStart).GetSafeNormal();
            HitInfo.SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
            HitInfo.HitActor = HitActor;

            // ������ ����
            ApplyDamage(HitActor, HitInfo);

            // ���⺰ Ư�� ȿ�� ����
            ApplyWeaponSpecialEffect(HitActor);
        }
    }
}

// ��Ʈ ����Ʈ ����
void AMHProjectCharacter::SpawnHitEffect(const FHitResult& Hit)
{
    if (!GetWorld()) return;

    // ��ƼŬ ȿ��
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

    // ���� ȿ��
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

    // ��Į (��Ʈ ��ũ)
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

    // ���� �Ӽ��� ���� �߰� ȿ��
    switch (WeaponElement)
    {
    case EElementalType::Fire:
        // �� ��ƼŬ
        // ��: UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FireParticle, ...);
        break;

    case EElementalType::Water:
        // �� ��ƼŬ
        break;

    case EElementalType::Thunder:
        // ���� ��ƼŬ
        break;

    case EElementalType::Ice:
        // ���� ��ƼŬ
        break;

    case EElementalType::Dragon:
        // �� �Ӽ� ��ƼŬ
        break;

    default:
        break;
    }
}

// ������ ����
void AMHProjectCharacter::ApplyDamage(AActor* HitActor, const FHitInfo& HitInfo)
{
    if (!HitActor) return;

    // �⺻ ������
    float TotalDamage = HitInfo.BaseDamage;

    // �Ӽ� ������ �߰�
    if (WeaponElement != EElementalType::None && HitInfo.ElementalDamage > 0)
    {
        TotalDamage += HitInfo.ElementalDamage;
    }

    // ������ ����
    UGameplayStatics::ApplyDamage(
        HitActor,
        TotalDamage,
        GetController(),
        this,
        UDamageType::StaticClass()
    );

    // �˹� ���� (���� ������Ʈ�� �ִ� ���)
    UPrimitiveComponent* HitPrimitive = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
    if (HitPrimitive && HitPrimitive->IsSimulatingPhysics())
    {
        HitPrimitive->AddImpulseAtLocation(
            HitInfo.HitDirection * HitInfo.KnockbackForce,
            HitInfo.HitLocation
        );
    }

    // ���� ���� (Ȯ�� ���)
    if (FMath::FRand() <= HitInfo.StunChance)
    {
        // ���ͳ� �� ĳ���Ϳ� ���� ���� ����
        // ���� ���������� �������̽��� �ٸ� ������� ó���� �� ����
        /*
        IGameplayInterface* GameplayInterface = Cast<IGameplayInterface>(HitActor);
        if (GameplayInterface)
        {
            // ���� �Լ� ȣ��
            GameplayInterface->Execute_ApplyStun(HitActor, 2.0f); // 2�� ����
        }
        */
    }

    // ��Ʈ �α�
    UE_LOG(LogTemp, Log, TEXT("Hit Actor: %s, Damage: %.2f, Elemental: %s"),
        *HitActor->GetName(),
        TotalDamage,
        *UEnum::GetValueAsString(WeaponElement));
}

// ���⺰ Ư�� ȿ�� ����
void AMHProjectCharacter::ApplyWeaponSpecialEffect(AActor* HitActor)
{
    if (!HitActor) return;

    switch (CurrentWeaponType)
    {
    case EWeaponType::GreatSword:
        // ��� Ÿ�� �� ���� ����
        break;

    case EWeaponType::LongSword:
        // �µ� Ÿ�� �� ������ ���
        break;

    case EWeaponType::DualBlades:
        // �ְ� Ÿ�� �� ���� ���� ���ʽ�
        break;

    case EWeaponType::HammerAxe:
        // �ظ�/���� Ÿ�� �� ���� ���� ȿ��
        break;

    default:
        break;
    }

    // �Ӽ� ȿ�� ����
    switch (WeaponElement)
    {
    case EElementalType::Fire:
        // ȭ�� �����̻� ����
        break;

    case EElementalType::Water:
        // ���� �����̻� ����
        break;

    case EElementalType::Thunder:
        // ���� �����̻� ����
        break;

    case EElementalType::Ice:
        // ���� �����̻� ����
        break;

    case EElementalType::Dragon:
        // �� �Ӽ� ��ȭ ����
        break;

    default:
        break;
    }
}