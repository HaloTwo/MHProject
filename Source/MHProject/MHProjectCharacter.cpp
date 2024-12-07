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
}

// ���� ���� �� ȣ��
void AMHProjectCharacter::BeginPlay()
{
    // �θ� Ŭ������ BeginPlay ȣ��
    Super::BeginPlay();
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
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

        // ���� ���ε�
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // �̵� ���ε�
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMHProjectCharacter::Move);

        // �þ� ��ȯ ���ε�
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMHProjectCharacter::Look);

        // ���� ���ε�
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AMHProjectCharacter::Attack);
    }
    else
    {
        // Enhanced Input�� ã�� ������ �� ���� �α� ���
        UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
    }
}

// �̵� ó��
void AMHProjectCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>(); // �Է� ������ 2D ���� ����

    if (Controller != nullptr)
    {
        // �̵� �� �޺� �� ���� �ִϸ��̼� ����
        if (MovementVector.X != 0.0f || MovementVector.Y != 0.0f)
        {
            ResetCombo();
            StopAnimMontage(AttackMontage);
        }

        // ��Ʈ�ѷ� �������� �̵�
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

// �þ� ��ȯ ó��
void AMHProjectCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>(); // �Է� ������ 2D ���� ����

    if (Controller != nullptr)
    {
        // ��Ʈ�ѷ��� ��ġ�� �� �� �߰�
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

// ���� ó��
void AMHProjectCharacter::Attack()
{
    // ���� �Լ� ȣ�� �� �α� ���
    UE_LOG(LogTemp, Warning, TEXT("Attack() called - ComboCount: %d, bCanNextAttack: %d"), ComboCount, bCanNextAttack);

    // ���� �Ұ����� ���¶�� ����
    if (!bCanNextAttack)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot attack now - waiting for animation"));
        return;
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (!AttackMontage) return;

        // �޺� ���¿� ���� �ִϸ��̼� ���
        switch (ComboCount)
        {
        case 0:
            PlayAnimMontage(AttackMontage, 0.75f, FName("Default"));
            ComboCount = 1;
            break;

        case 1:
            PlayAnimMontage(AttackMontage, 1.0f, FName("Attack1"));
            ComboCount = 2;
            break;

        case 2:
            PlayAnimMontage(AttackMontage, 1.5f, FName("Attack2"));
            ComboCount = 0; // ������ ���� �� �޺� ����
            break;
        }

        UE_LOG(LogTemp, Warning, TEXT("Playing attack animation - ComboCount: %d"), ComboCount);
        bCanNextAttack = false; // ���� ���� �Ұ��� ���·� ����
    }
}

// �޺� ���� �Լ�
void AMHProjectCharacter::ResetCombo()
{
    ComboCount = 0; // �޺� ī��Ʈ �ʱ�ȭ
    bCanNextAttack = true; // ���� ���� ���� ���·� ����
}

// ���� ���� ���� ���� Ȱ��ȭ
void AMHProjectCharacter::EnableNextAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("EnableNextAttack called - Current ComboCount: %d"), ComboCount);
    bCanNextAttack = true;
}

// ���� ���� ���� ���� ��Ȱ��ȭ
void AMHProjectCharacter::DisableNextAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("DisableNextAttack called!"));
    bCanNextAttack = false;
}
