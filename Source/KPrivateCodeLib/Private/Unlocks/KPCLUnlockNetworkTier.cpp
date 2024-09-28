// Fill out your copyright notice in the Description page of Project Settings.


#include "Unlocks/KPCLUnlockNetworkTier.h"

#include "FGRecipeManager.h"
#include "FGUnlockSubsystem.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

void UKPCLUnlockNetworkTier::Unlock(AFGUnlockSubsystem* unlockSubssytem)
{
	Super::Unlock(unlockSubssytem);
	SendToSubsystem(unlockSubssytem);
}

void UKPCLUnlockNetworkTier::Apply(AFGUnlockSubsystem* unlockSubssytem)
{
	Super::Apply(unlockSubssytem);
	SendToSubsystem(unlockSubssytem);
}

void UKPCLUnlockNetworkTier::SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem)
{
	if (unlockSubssytem && ensure(mSchematic))
	{
		AKPCLFaxitSubsystem* Subsystem = AKPCLFaxitSubsystem::Get(unlockSubssytem->GetWorld());
		fgcheck(Subsystem);
		Subsystem->UnlockNetworkTier(mSchematic);
	}
}
