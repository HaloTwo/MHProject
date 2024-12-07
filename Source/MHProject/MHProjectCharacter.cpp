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
}

// 게임 시작 시 호출
void AMHProjectCharacter::BeginPlay()
{
    // 부모 클래스의 BeginPlay 호출
    Super::BeginPlay();
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
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

        // 점프 바인딩
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // 이동 바인딩
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMHProjectCharacter::Move);

        // 시야 전환 바인딩
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMHProjectCharacter::Look);

        // 공격 바인딩
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AMHProjectCharacter::Attack);
    }
    else
    {
        // Enhanced Input을 찾지 못했을 때 오류 로그 출력
        UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
    }
}

// 이동 처리
void AMHProjectCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>(); // 입력 값에서 2D 벡터 추출

    if (Controller != nullptr)
    {
        // 이동 시 콤보 및 공격 애니메이션 리셋
        if (MovementVector.X != 0.0f || MovementVector.Y != 0.0f)
        {
            ResetCombo();
            StopAnimMontage(AttackMontage);
        }

        // 컨트롤러 방향으로 이동
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

// 시야 전환 처리
void AMHProjectCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>(); // 입력 값에서 2D 벡터 추출

    if (Controller != nullptr)
    {
        // 컨트롤러의 피치와 요 값 추가
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

// 공격 처리
void AMHProjectCharacter::Attack()
{
    // 공격 함수 호출 시 로그 출력
    UE_LOG(LogTemp, Warning, TEXT("Attack() called - ComboCount: %d, bCanNextAttack: %d"), ComboCount, bCanNextAttack);

    // 공격 불가능한 상태라면 리턴
    if (!bCanNextAttack)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot attack now - waiting for animation"));
        return;
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (!AttackMontage) return;

        // 콤보 상태에 따라 애니메이션 재생
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
            ComboCount = 0; // 마지막 공격 후 콤보 리셋
            break;
        }

        UE_LOG(LogTemp, Warning, TEXT("Playing attack animation - ComboCount: %d"), ComboCount);
        bCanNextAttack = false; // 다음 공격 불가능 상태로 설정
    }
}

// 콤보 리셋 함수
void AMHProjectCharacter::ResetCombo()
{
    ComboCount = 0; // 콤보 카운트 초기화
    bCanNextAttack = true; // 다음 공격 가능 상태로 설정
}

// 다음 공격 가능 상태 활성화
void AMHProjectCharacter::EnableNextAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("EnableNextAttack called - Current ComboCount: %d"), ComboCount);
    bCanNextAttack = true;
}

// 다음 공격 가능 상태 비활성화
void AMHProjectCharacter::DisableNextAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("DisableNextAttack called!"));
    bCanNextAttack = false;
}
