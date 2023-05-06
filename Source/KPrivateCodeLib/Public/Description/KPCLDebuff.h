// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGPlayerController.h"
#include "Structures/KPCLLevelingStruc.h"
#include "UObject/Object.h"
#include "KPCLDebuff.generated.h"


UENUM( BlueprintType )
enum class EDebuffType : uint8
{
	Fixed   = 0 UMETA( DisplayName = "Fixed" ),
	Percent = 1 UMETA( DisplayName = "Percent" ),
	Stacked = 2 UMETA( DisplayName = "Stacked" )};

/**
 * 
 */
UCLASS( Blueprintable, BlueprintType, Abstract )
class KPRIVATECODELIB_API UKPCLDebuff_Base : public UObject, public IFGSaveInterface
{
	GENERATED_BODY()


	virtual bool IsSupportedForNetworking() const override;
	FORCEINLINE virtual bool ShouldSave_Implementation() const override { return true; }
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

public:
	// will Called from Subsystem || Parallel Tick!
	FORCEINLINE virtual void TickDebuff( float dt )
	{
	}

	FORCEINLINE virtual void InitDebuff() { mActive = false; }

	virtual float ValidateEXP( float Exp, bool Auth = true );

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="KMods" )
	bool mIsDebuffGlobal = true;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="KMods" )
	bool mIsExpDebuff = false;

	UFUNCTION( BlueprintPure, BlueprintNativeEvent, Category="KMods|Debuff" )
	bool IsActive() const;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="KMods|UI" )
	UTexture* mIcon = nullptr;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="KMods|UI" )
	FLinearColor mIconTint = FLinearColor::White;

	UFUNCTION( BlueprintPure, BlueprintNativeEvent )
	float GetDebuffValue() const;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="KMods|UI" )
	FText mDebuffName = FText::FromString( "Debuff Name" );

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="KMods|UI" )
	EDebuffType mDebuffType = EDebuffType::Fixed;

	UPROPERTY( Replicated )
	bool mActive = true;

	bool bShouldTick = false;
};

/**
 * 
 */
UCLASS( Blueprintable, BlueprintType, Abstract )
class KPRIVATECODELIB_API UKPCLDebuffExp : public UKPCLDebuff_Base
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

	virtual bool IsActive_Implementation() const override { return mDebuffValue > 0.0f; };

	float OnExpAdded( float Exp, bool Auth );
	float OnExpLost( float Exp );

	virtual float ValidateEXP( float Exp, bool Auth ) override;

	virtual float GetDebuffValue_Implementation() const override { return mDebuffValue; };

	UPROPERTY( SaveGame, Replicated )
	float mDebuffValue = 0.0f;
};

/**
 * 
 */
UCLASS( Blueprintable, BlueprintType, Abstract )
class KPRIVATECODELIB_API UKPCLBuffExp : public UKPCLDebuff_Base
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;
	virtual bool IsActive_Implementation() const override { return mExpBuffValue > 1.0f; };

	virtual float ValidateEXP( float Exp, bool Auth ) override;

	virtual float GetDebuffValue_Implementation() const override { return mExpBuffValue; };

	UPROPERTY( SaveGame, Replicated )
	float mExpBuffValue = 0.0f;
};
