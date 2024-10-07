// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Looting/KPCLLootChest.h"

#include "FGBlueprintFunctionLibrary.h"
#include "FGCharacterPlayer.h"
#include "BFL/KBFL_Player.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "Net/UnrealNetwork.h"

#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGNoneDescriptor.h"
#include "Subsystem/KPCLUnlockSubsystem.h"


// Sets default values
AKPCLLootChest::AKPCLLootChest() : Super()
{
	PrimaryActorTick.bCanEverTick = 1;
	PrimaryActorTick.bStartWithTickEnabled = 1;

	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
	mMesh = CreateDefaultSubobject<UFGColoredInstanceMeshProxy>("LootChestMesh");
}

void AKPCLLootChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLLootChest, mLootableTable);
	DOREPLIFETIME(AKPCLLootChest, mChestIsLooted);
}

bool AKPCLLootChest::ShouldSave_Implementation() const
{
	return true;
}

void AKPCLLootChest::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GenerateLoot();

		GetInventory( )->mItemFilter.BindUObject(this, &AKPCLLootChest::FilterItemClasses);
		
		GetInventory()->OnItemRemovedDelegate.AddUniqueDynamic(this, &AKPCLLootChest::OnInputItemRemoved);
	}

	OnRep_OnLooted();
}

void AKPCLLootChest::GenerateLoot()
{
	if (WasLooted() || !GetInventory()->IsEmpty() || !HasAuthority())
	{
		return;
	}

	if (!WasLooted() && mLootableTable.Num() <= 0)
	{
		int32 Trys = mRandomTrys.GetRandom();

		for (int32 idx = 0; idx < Trys; ++idx)
		{
			FKPCLLootChestRandomData Data = mRandomData[UKismetMathLibrary::RandomIntegerInRange(
				0, mRandomData.Num() - 1)];
			if (Data.mItemClass)
			{
				mLootableTable.Add(FItemAmount(Data.mItemClass,
				                               FMath::Max<int32>(
					                               Data.mAmountRange.GetRandom() * FMath::Max<int32>(
						                               1, UFGItemDescriptor::GetStackSize(Data.mItemClass) / 100) * Data
					                               .mAmountMultiplier, 1)));
			}
		}
		
		GetInventory()->Resize(mLootableTable.Num());
		for(int32 idx = 0; idx < mLootableTable.Num(); ++idx)
		{
			FInventoryStack Stack = FInventoryStack(mLootableTable[idx].Amount, mLootableTable[idx].ItemClass);
			GetInventory()->AddStackToIndex(idx, Stack, false);
		}
	}

	OnRep_LootTableUpdate();
}

bool AKPCLLootChest::WasLooted() const
{
	return mChestIsLooted;
}

UFGInventoryComponent* AKPCLLootChest::GetInventory() const
{
	return mInventory;
}

void AKPCLLootChest::Loot(AFGCharacterPlayer* Player)
{
	if (!Player)
	{
		return;
	}

	if (HasAuthority())
	{
		TArray<FItemAmount> NotAddedAmount;

		for (FItemAmount Loot : mLootableTable)
		{
			if (Loot.ItemClass && Loot.Amount > 0)
			{
				const int32 Added = Player->GetInventory()->
				                            AddStack(FInventoryStack(Loot.Amount, Loot.ItemClass), true);
				Loot.Amount -= Added;

				if (Loot.Amount > 0)
				{
					NotAddedAmount.Add(Loot);
				}
			}
		}

		mLootableTable = NotAddedAmount;
		OnRep_LootTableUpdate();
		ForceNetUpdate();
	}
	else if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld()))
	{
		RCO->Server_LootChest(this, Player);
	}
}

bool AKPCLLootChest::FilterItemClasses(TSubclassOf<UObject> object, int32 idx) const
{
	return false;
}

void AKPCLLootChest::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                        UFGInventoryComponent* sourceInventory)
{
	OnRep_LootTableUpdate();
}

void AKPCLLootChest::OnRep_LootTableUpdate()
{
	if(IsValid(GetInventory()) && GetInventory()->IsEmpty() != WasLooted())
	{
		mChestIsLooted = GetInventory()->IsEmpty();
	}
	
	if (OnLootTableUpdated.IsBound())
	{
		OnLootTableUpdated.Broadcast();
	}

	LootTableUpdated();
	OnRep_OnLooted();
}

void AKPCLLootChest::OnRep_OnLooted()
{
	if (OnLootedChanged.IsBound())
	{
		OnLootedChanged.Broadcast(WasLooted());
	}
	OnLootedUpdated(WasLooted());
	
	if (WasLooted())
	{
		SetActorTickEnabled(false);
		if (mMesh)
		{
			mMesh->DestroyComponent();
		}
	}
}
