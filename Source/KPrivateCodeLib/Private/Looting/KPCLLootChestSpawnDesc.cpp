// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Looting/KPCLLootChestSpawnDesc.h"

void UKPCLLootChestSpawnDesc::ForeachLocations( TArray< AActor* >& ActorArray )
{
	for( int32 idx = 0; idx < mLootChestLocations.Num(); ++idx )
	{
		if( mLootChestContent.IsValidIndex( idx ) )
		{
			AActor* Actor;
			if( !CheckActorInRange( mLootChestLocations[ idx ], Actor ) )
			{
				if( AKPCLLootChest* LootChestActor = GetWorld()->SpawnActorDeferred< AKPCLLootChest >( GetActorClass(), mLootChestLocations[ idx ] ) )
				{
					//LootChestActor->SetLoot(mLootChestContent[idx].mLootChestContent);
					LootChestActor->FinishSpawning( mLootChestLocations[ idx ], true );
					ActorArray.Add( LootChestActor );
					UE_LOG( LogTemp, Log, TEXT("Spawn Lootchest <%s> | %s"), *mLootChestLocations[idx].ToString(), *LootChestActor->GetName() );
				}
			}

			if( Actor )
			{
				ActorArray.Add( Actor );
			}
		}
		else
		{
			UE_LOG( LogTemp, Log, TEXT("Spawn Lootchest failed! no content for idx: %d"), idx );
		}
	}
}

TArray< TSubclassOf< AActor > > UKPCLLootChestSpawnDesc::GetSearchingActorClasses()
{
	return { GetActorClass() };
}

TSubclassOf< AActor > UKPCLLootChestSpawnDesc::GetActorClass()
{
	return mLootChestClass;
}

TSubclassOf< AActor > UKPCLLootChestSpawnDesc::GetActorFreeClass()
{
	return GetActorClass();
}
