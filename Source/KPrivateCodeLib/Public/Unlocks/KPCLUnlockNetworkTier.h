// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "Description/Decor/KPCLDecorationActorData.h"
#include "Unlocks/FGUnlockInfoOnly.h"
#include "KPCLUnlockNetworkTier.generated.h"

/**
 * 
 */
UCLASS( Blueprintable, EditInlineNew, abstract, DefaultToInstanced )
class KPRIVATECODELIB_API UKPCLUnlockNetworkTier : public UFGUnlockInfoOnly
{
	GENERATED_BODY()

	virtual void Unlock( AFGUnlockSubsystem* unlockSubssytem ) override;
	virtual void Apply( AFGUnlockSubsystem* unlockSubssytem ) override;
	void SendToSubsystem( AFGUnlockSubsystem* unlockSubssytem );

private:
	UPROPERTY( EditAnywhere )
	TSubclassOf< UFGSchematic > mSchematic;
};


/**
 * 
 */
UCLASS( Blueprintable, EditInlineNew, abstract, DefaultToInstanced )
class KPRIVATECODELIB_API UKPCLUnlockDecoration : public UFGUnlock
{
	GENERATED_BODY()

	virtual void Unlock( AFGUnlockSubsystem* unlockSubssytem ) override;
	virtual void Apply( AFGUnlockSubsystem* unlockSubssytem ) override;
	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;
	void SendToSubsystem( AFGUnlockSubsystem* unlockSubssytem );

private:
	UPROPERTY( EditAnywhere )
	TArray< TSubclassOf< UKPCLDecorationRecipe > > mDecorations;
};