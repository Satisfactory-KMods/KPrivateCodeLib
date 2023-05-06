// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Looting/KPCLLootChest.h"

#include "FGBlueprintFunctionLibrary.h"
#include "FGCharacterPlayer.h"
#include "BFL/KBFL_Player.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGNoneDescriptor.h"
#include "Subsystem/KPCLUnlockSubsystem.h"


void AKPCLLootChest::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AKPCLLootChest, mLootableTable );
	DOREPLIFETIME( AKPCLLootChest, mContentLooted );
}

bool AKPCLLootChest::ShouldSave_Implementation() const
{
	return true;
}

void AKPCLLootChest::OnUse_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state )
{
	UE_LOG( LogTemp, Error, TEXT("OnUse_Implementation(%d) %d %d"), mLootableTable.Num(), !mContentLooted, mLootableTable.Num() <= 0 )
	Super::OnUse_Implementation( byCharacter, state );
}

// Sets default values
AKPCLLootChest::AKPCLLootChest() : Super()
{
	PrimaryActorTick.bCanEverTick = 1;
	PrimaryActorTick.bStartWithTickEnabled = 1;
}

void AKPCLLootChest::BeginPlay()
{
	Super::BeginPlay();

	Mesh = FindComponentByClass< UFGColoredInstanceMeshProxy >();

	// Looted chests dont need any checking
	if( HasAuthority() )
	{
		GenerateLoot();
	}
	if( WasLooted() )
	{
		if( Mesh )
		{
			Mesh->DestroyComponent();
		}
	}
}

void AKPCLLootChest::GenerateLoot()
{
	if( !mContentLooted && mLootableTable.Num() <= 0 )
	{
		int32 Trys = mRandomTrys.GetRandom();

		for( int32 idx = 0; idx < Trys; ++idx )
		{
			FKPCLLootChestRandomData Data = mRandomData[ UKismetMathLibrary::RandomIntegerInRange( 0, mRandomData.Num() - 1 ) ];
			if( Data.mItemClass )
			{
				mLootableTable.Add( FItemAmount( Data.mItemClass, FMath::Max< int32 >( Data.mAmountRange.GetRandom() * FMath::Max< int32 >( 1, UFGItemDescriptor::GetStackSize( Data.mItemClass ) / 100 ) * Data.mAmountMultiplier, 1 ) ) );;
			}
		}
	}

	if( mLootableTable.Num() > 0 )
	{
		TMap< TSubclassOf< UFGItemDescriptor >, int32 > AmountMap;
		for( FItemAmount LootableTable : mLootableTable )
		{
			int32 Amount = 0;
			if( AmountMap.Contains( LootableTable.ItemClass ) )
			{
				Amount = AmountMap[ LootableTable.ItemClass ];
			}
			Amount += LootableTable.Amount;
			AmountMap.Add( LootableTable.ItemClass, Amount );
		}

		mLootableTable.Empty();
		for( TTuple< TSubclassOf< UFGItemDescriptor >, int > Map : AmountMap )
		{
			mLootableTable.Add( FItemAmount( Map.Key, Map.Value ) );
		}
	}
}

TArray< FItemAmount > AKPCLLootChest::GetLoot() const
{
	return mLootableTable;
}

bool AKPCLLootChest::WasLooted() const
{
	return mContentLooted;
}

void AKPCLLootChest::Loot( AFGCharacterPlayer* Player )
{
	if( !Player )
	{
		return;
	}

	if( HasAuthority() )
	{
		TArray< FItemAmount > NotAddedAmount;

		for( FItemAmount Loot : GetLoot() )
		{
			if( Loot.ItemClass && Loot.Amount > 0 )
			{
				const int32 Added = Player->GetInventory()->AddStack( FInventoryStack( Loot.Amount, Loot.ItemClass ), true );
				Loot.Amount -= Added;

				if( Loot.Amount > 0 )
				{
					NotAddedAmount.Add( Loot );
				}
			}
		}

		mLootableTable = NotAddedAmount;
		OnRep_LootTableUpdate();
		ForceNetUpdate();
	}
	else if( UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get( GetWorld() ) )
	{
		RCO->Server_Core_LootChest( this, Player );
	}
}

void AKPCLLootChest::OnRep_LootTableUpdate()
{
	mContentLooted = mLootableTable.Num() <= 0;

	if( OnLootTableUpdated.IsBound() )
	{
		OnLootTableUpdated.Broadcast();
	}

	LootTableUpdated();
	if( WasLooted() )
	{
		if( Mesh )
		{
			Mesh->DestroyComponent();
		}
	}
}
