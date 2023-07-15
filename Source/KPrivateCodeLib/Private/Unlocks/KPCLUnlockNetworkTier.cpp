// Fill out your copyright notice in the Description page of Project Settings.


#include "Unlocks/KPCLUnlockNetworkTier.h"

#include "FGRecipeManager.h"
#include "FGUnlockSubsystem.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

void UKPCLUnlockNetworkTier::Unlock(AFGUnlockSubsystem* unlockSubssytem) {
	Super::Unlock(unlockSubssytem);
	SendToSubsystem(unlockSubssytem);
}

void UKPCLUnlockNetworkTier::Apply(AFGUnlockSubsystem* unlockSubssytem) {
	Super::Apply(unlockSubssytem);
	SendToSubsystem(unlockSubssytem);
}

void UKPCLUnlockNetworkTier::SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem) {
	if(unlockSubssytem && ensure(mSchematic)) {
		AKPCLUnlockSubsystem* Subsystem = AKPCLUnlockSubsystem::Get(unlockSubssytem);
		check(Subsystem);
		Subsystem->UnlockNetworkTier(mSchematic);
	}
}

void UKPCLUnlockDecoration::Unlock(AFGUnlockSubsystem* unlockSubssytem) {
	Super::Unlock(unlockSubssytem);
	SendToSubsystem(unlockSubssytem);
}

void UKPCLUnlockDecoration::Apply(AFGUnlockSubsystem* unlockSubssytem) {
	Super::Apply(unlockSubssytem);
	SendToSubsystem(unlockSubssytem);
}

bool UKPCLUnlockDecoration::IsRepeatPurchasesAllowed_Implementation() const {
	return true;
}

void UKPCLUnlockDecoration::SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem) {
	if(unlockSubssytem && ensure(mDecorations.Num() > 0)) {
		AKPCLUnlockSubsystem* Subsystem = AKPCLUnlockSubsystem::Get(unlockSubssytem);
		check(Subsystem);
		Subsystem->UnlockDecorations(mDecorations);
	}
}
