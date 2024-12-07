// DisableNextAttackNotify.cpp
#include "DisableNextAttackNotify.h"
#include "MHProjectCharacter.h"

void UDisableNextAttackNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (MeshComp && MeshComp->GetOwner())
    {
        if (AMHProjectCharacter* Character = Cast<AMHProjectCharacter>(MeshComp->GetOwner()))
        {
            Character->DisableNextAttack();
        }
    }
}