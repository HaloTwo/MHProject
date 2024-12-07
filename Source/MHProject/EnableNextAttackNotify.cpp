// Fill out your copyright notice in the Description page of Project Settings.


#include "EnableNextAttackNotify.h"
#include "MHProjectCharacter.h"

void UEnableNextAttackNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (MeshComp && MeshComp->GetOwner())
    {
        if (AMHProjectCharacter* Character = Cast<AMHProjectCharacter>(MeshComp->GetOwner()))
        {
            Character->EnableNextAttack();
        }
    }
}