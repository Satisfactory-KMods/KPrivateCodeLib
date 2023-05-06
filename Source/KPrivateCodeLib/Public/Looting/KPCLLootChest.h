// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FGCrate.h"
#include "FGItemPickup.h"
#include "FGSignificanceInterface.h"
#include "ItemAmount.h"
#include "Kismet/KismetMathLibrary.h"
#include "KPCLLootChest.generated.h"

USTRUCT( BlueprintType )
struct FKPCLRange
{
	GENERATED_BODY()

	FKPCLRange()
	{
	}

	FKPCLRange( int32 A, int32 B )
	{
		mMin = A;
		mMax = A;
	}

	UPROPERTY( BlueprintReadWrite, EditAnywhere )
	int32 mMin = 5;

	UPROPERTY( BlueprintReadWrite, EditAnywhere )
	int32 mMax = 15;

	int32 GetRandom() const
	{
		return UKismetMathLibrary::RandomIntegerInRange( mMin, mMax );
	}
};

USTRUCT( BlueprintType )
struct FKPCLLootChestRandomData
{
	GENERATED_BODY()

	UPROPERTY( BlueprintReadWrite, EditAnywhere )
	TSubclassOf< UFGItemDescriptor > mItemClass;

	UPROPERTY( BlueprintReadWrite, EditAnywhere )
	FKPCLRange mAmountRange = FKPCLRange( 5, 10 );

	UPROPERTY( BlueprintReadWrite, EditAnywhere )
	int32 mAmountMultiplier;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE( FOnLootTableUpdated );

UCLASS()
class KPRIVATECODELIB_API AKPCLLootChest : public AFGInteractActor, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	/** Decide on what properties to replicate */
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

	virtual bool ShouldSave_Implementation() const override;

	virtual void OnUse_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state ) override;

	// Sets default values for this actor's properties
	AKPCLLootChest();

	virtual void BeginPlay() override;

	void GenerateLoot();

	UFUNCTION( BlueprintPure )
	TArray< FItemAmount > GetLoot() const;

	UFUNCTION( BlueprintPure )
	bool WasLooted() const;

	UFUNCTION( BlueprintCallable )
	void Loot( AFGCharacterPlayer* Player );

	UPROPERTY( BlueprintAssignable )
	FOnLootTableUpdated OnLootTableUpdated;

	UFUNCTION( BlueprintImplementableEvent )
	void LootTableUpdated();

private:
	friend class UKPCLLootChestSpawnDesc;

	UFUNCTION()
	void OnRep_LootTableUpdate();

	UPROPERTY( EditAnywhere, SaveGame, Replicated, ReplicatedUsing=OnRep_LootTableUpdate )
	TArray< FItemAmount > mLootableTable;

	UPROPERTY( SaveGame, Replicated, ReplicatedUsing=OnRep_LootTableUpdate )
	bool mContentLooted;

	UPROPERTY( EditAnywhere, Category="KMods" )
	TArray< FKPCLLootChestRandomData > mRandomData;

	UPROPERTY( EditAnywhere, Category="KMods" )
	FKPCLRange mRandomTrys = FKPCLRange( 5, 20 );

	UPROPERTY()
	UFGColoredInstanceMeshProxy* Mesh;
};
