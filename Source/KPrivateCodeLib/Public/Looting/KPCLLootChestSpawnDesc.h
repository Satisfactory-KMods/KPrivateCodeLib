// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemAmount.h"
#include "KPCLLootChest.h"
#include "Subsystems/ResourceNodes/KBFLActorSpawnDescriptor.h"
#include "KPCLLootChestSpawnDesc.generated.h"

USTRUCT( BlueprintType )
struct FKPCLLootChestContent
{
	GENERATED_BODY()

	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite )
	TArray< FItemAmount > mLootChestContent;
};

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLLootChestSpawnDesc : public UKBFLActorSpawnDescriptor
{
	GENERATED_BODY()

protected:
	virtual void ForeachLocations( TArray< AActor* >& ActorArray ) override;

	virtual TArray< TSubclassOf< AActor > > GetSearchingActorClasses() override;
	virtual TSubclassOf< AActor > GetActorClass() override;
	virtual TSubclassOf< AActor > GetActorFreeClass() override;

	UPROPERTY( EditDefaultsOnly, Category="Actor" )
	TSubclassOf< AKPCLLootChest > mLootChestClass;

public:
	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite, Category="Actor" )
	TArray< FTransform > mLootChestLocations;

	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite, Category="Actor" )
	TArray< FKPCLLootChestContent > mLootChestContent;
};
