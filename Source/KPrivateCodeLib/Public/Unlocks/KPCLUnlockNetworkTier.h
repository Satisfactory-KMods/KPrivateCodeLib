// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "Unlocks/FGUnlockInfoOnly.h"
#include "KPCLUnlockNetworkTier.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KPRIVATECODELIB_API UKPCLUnlockNetworkTier : public UFGUnlockInfoOnly
{
	GENERATED_BODY()

	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;
	virtual void Apply(AFGUnlockSubsystem* unlockSubssytem) override;
	void SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem);

	UPROPERTY(EditAnywhere)
	TSubclassOf<UFGSchematic> mSchematic;
};
