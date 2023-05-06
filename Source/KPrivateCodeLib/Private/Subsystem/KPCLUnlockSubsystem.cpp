// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/KPCLUnlockSubsystem.h"

#include "FGRecipeManager.h"
#include "KPrivateCodeLibModule.h"
#include "BFL/KBFL_Player.h"
#include "BFL/KBFL_Util.h"
#include "Components/KPCLNetworkPlayerComponent.h"
#include "Description/KPCLEndlessShopItem.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN( KPCLUnlockSubsystemLog, Log, All )

DEFINE_LOG_CATEGORY( KPCLUnlockSubsystemLog )

AKPCLUnlockSubsystem::AKPCLUnlockSubsystem()
{
	mShouldSave = true;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;

	mFluidItemsPerBytes = AKPCLNetworkCore::mFluidItemsPerBytes;
	mSolidItemsPerBytes = AKPCLNetworkCore::mSolidItemsPerBytes;

	mNetworkConnectionFluidBufferSize = 500;
	mNetworkConnectionSolidBufferSize = 1;
}

void AKPCLUnlockSubsystem::PreSaveGame_Implementation( int32 saveVersion, int32 gameVersion )
{
	Super::PreSaveGame_Implementation( saveVersion, gameVersion );
	DequeuePoints();
}

void AKPCLUnlockSubsystem::DequeuePoints()
{
	int64 PointsToAdd = 0;
	
	while( !mPointQueue.IsEmpty() )
	{
		int64 Points;
		if( mPointQueue.Dequeue( Points ) )
		{
			PointsToAdd += Points;
		}
	}
	
	if( PointsToAdd > 0 )
	{
		AddPoints( PointsToAdd );
	}
}

void AKPCLUnlockSubsystem::GenerateCategoryMap()
{
	for ( TSubclassOf< UKPCLEndlessShopItem > EndlessShopItem : mAllEndlessShopItem )
	{
		TSubclassOf< UKPCLEndlessShopMainCategory > MainCat = UKPCLEndlessShopItem::GetMainCategory( EndlessShopItem );
		TSubclassOf< UKPCLEndlessShopSubCategory > SubCat = UKPCLEndlessShopItem::GetSubCategory( EndlessShopItem );

		TMap< TSubclassOf< UKPCLEndlessShopSubCategory >, TArray< TSubclassOf< UKPCLEndlessShopItem > > >* CatMap = mCategoryMap.Find( MainCat );
		if( CatMap )
		{
			TArray< TSubclassOf< UKPCLEndlessShopItem > >* Array = CatMap->Find( SubCat );
			if( Array )
			{
				Array->AddUnique( EndlessShopItem );
			}
			else
			{
				CatMap->Add( SubCat, { EndlessShopItem } );
			}
		}
		else
		{
			TMap< TSubclassOf< UKPCLEndlessShopSubCategory >, TArray< TSubclassOf< UKPCLEndlessShopItem > > > NewCatMap;
			NewCatMap.Add( SubCat, { EndlessShopItem } );
			mCategoryMap.Add( MainCat, NewCatMap );
		}
	}
}

void AKPCLUnlockSubsystem::AddPoints( int64 Points )
{
	mCurrentPoints += Points;
	OnRep_PointsUpdated();
}

void AKPCLUnlockSubsystem::AddPoints_ThreadSafe( int64 Points )
{
	if( Points > 0 )
	{
		mPointQueue.Enqueue( Points );
	}
}

void AKPCLUnlockSubsystem::OnRep_PointsUpdated()
{
	if( OnPointsUpdated.IsBound() )
	{
		OnPointsUpdated.Broadcast( mCurrentPoints );
	}
}

AKPCLUnlockSubsystem* AKPCLUnlockSubsystem::Get( UObject* worldContext )
{
	return Cast< AKPCLUnlockSubsystem >( UKBFL_Util::GetSubsystemFromChild( worldContext, StaticClass() ) );
}

void AKPCLUnlockSubsystem::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	
	DOREPLIFETIME( AKPCLUnlockSubsystem, mCurrentPoints );
	DOREPLIFETIME( AKPCLUnlockSubsystem, mUnlockedNetworkTiers );
	DOREPLIFETIME( AKPCLUnlockSubsystem, mUnlockedShopItems );
	DOREPLIFETIME( AKPCLUnlockSubsystem, mBuildedNexus );
	
}

void AKPCLUnlockSubsystem::BeginPlay()
{
	Super::BeginPlay();
	
	UKBFLAssetDataSubsystem* Subsystem = UKBFLAssetDataSubsystem::Get( GetWorld() );
	if( IsValid( Subsystem ) )
	{
		//Register default content
		TArray< TSubclassOf< UKPCLEndlessShopItem > > EndlessShopItems;
		Subsystem->GetObjectsOfChilds_Internal( { UKPCLRegistryObject::StaticClass() }, EndlessShopItems );
		
		for ( TSubclassOf< UKPCLEndlessShopItem >EndlessShopItem : EndlessShopItems )
		{
			if( !UKPCLEndlessShopItem::IsShopItemsIsHidden( EndlessShopItem ) )
			{
				mAllEndlessShopItem.Add( EndlessShopItem );
			}
		}

		GenerateCategoryMap();
	}
	OnRep_Decoration();

	if( HasAuthority() )
	{
		AFGSchematicManager* SchematicManager = AFGSchematicManager::Get( GetWorld() );
		if( IsValid( SchematicManager ) )
		{
			bPassiveIsUnlocked = SchematicManager->IsSchematicPurchased( mSchematicToUnlockPassivPoints );
			if( !bPassiveIsUnlocked )
			{
				SchematicManager->PurchasedSchematicDelegate.AddUniqueDynamic( this, &AKPCLUnlockSubsystem::OnSchematicUnlocked );
			}
		}
		
		mUnlockedShopItems.Remove( nullptr );
		mShoppingList.Remove( nullptr );
		for ( const TSubclassOf<UKPCLEndlessShopItem> UnlockedShopItem : mUnlockedShopItems )
		{
			ApplyEndlessShopItem( UnlockedShopItem );
		}
	}
}

void AKPCLUnlockSubsystem::OnSchematicUnlocked( TSubclassOf<UFGSchematic> UnlockedSchematic )
{
	if( UnlockedSchematic == mSchematicToUnlockPassivPoints )
	{
		bPassiveIsUnlocked = true;
	}
}

void AKPCLUnlockSubsystem::Init()
{
	Super::Init();

	AKPCLNetworkCore::mFluidItemsPerBytes = mFluidItemsPerBytes;
	AKPCLNetworkCore::mSolidItemsPerBytes = mSolidItemsPerBytes;

	FNetworkConnectionInformations::mNetworkConnectionFluidBufferSize = mNetworkConnectionFluidBufferSize;
	FNetworkConnectionInformations::mNetworkConnectionSolidBufferSize = mNetworkConnectionSolidBufferSize;

	SetActorTickInterval( 1 / 15 );
}

void AKPCLUnlockSubsystem::Tick( float DeltaSeconds )
{
	Super::Tick( DeltaSeconds );

	for( AFGPlayerState* Player : mPlayerStates )
	{
		if( IsValid( Player ) )
		{
			UKPCLNetworkPlayerComponent* Comp = UKPCLNetworkPlayerComponent::GetOrCreateNetworkComponentToPlayerState( GetWorld(), Player, mStateComponentClass );
			if( IsValid( Comp ) )
			{
				Comp->CustomTick( DeltaSeconds );
			}
		}
	}

	if( bPassiveIsUnlocked )
	{
		if( HasAuthority() )
		{
			if( mTimeToGetPassivePoints.Tick( DeltaSeconds ) )
			{
				AddPoints_ThreadSafe( mPassivePoints );
			}
		}
	}
	
	DequeuePoints();
}

int32 AKPCLUnlockSubsystem::GetNetworkTier() const
{
	int32 Tier = 1;
	if( mUnlockedNetworkTiers.Num() > 0 )
	{
		for( UClass* UnlockedNetworkTier : mUnlockedNetworkTiers )
		{
			if( UnlockedNetworkTier )
			{
				Tier++;
			}
		}
	}
	return Tier;
}

void AKPCLUnlockSubsystem::UnlockNetworkTier( TSubclassOf< UFGSchematic > BoundedSchematic )
{
	if( HasAuthority() )
	{
		if( mUnlockedNetworkTiers.AddUnique( BoundedSchematic ) != INDEX_NONE )
		{
			OnNetworkTierUnlocked.Broadcast( GetNetworkTier() );
		}
	}
}

void AKPCLUnlockSubsystem::OnNexusConstruct( AKPCLNetworkCore* Nexus )
{
	mBuildedNexus.Add( Nexus );
}

void AKPCLUnlockSubsystem::OnNexusDeconstruct( AKPCLNetworkCore* Nexus )
{
	if( mBuildedNexus.Contains( Nexus ) )
	{
		mBuildedNexus.Remove( Nexus );
	}
}

int32 AKPCLUnlockSubsystem::GetGlobalNexusCount() const
{
	return mBuildedNexus.Num();
}

int32 AKPCLUnlockSubsystem::GetMaxGlobalNexusCount() const
{
	return mNetworkGlobalNexusMaxCount;
}

TArray< AKPCLNetworkCore* > AKPCLUnlockSubsystem::GetAllNexusInTheWorld() const
{
	return mBuildedNexus;
}

bool AKPCLUnlockSubsystem::IsEndlessShopItemUnlocked( TSubclassOf<UKPCLEndlessShopItem> InClass ) const
{
	if( !IsValid( InClass ) )
	{
		return false;
	}
	return mUnlockedShopItems.Contains( InClass );
}

void AKPCLUnlockSubsystem::UnlockEndlessShopItems( AFGCharacterPlayer* Player, TArray<TSubclassOf<UKPCLEndlessShopItem>> UnlockingItems )
{
	if( !CanBuyShoppingList( Player, UnlockingItems ) )
	{
		return;
	}
	
	if( HasAuthority() && IsValid( Player ) )
	{
		TArray<TSubclassOf<UKPCLEndlessShopItem>> UnlockedItems;
		for ( TSubclassOf<UKPCLEndlessShopItem> UnlockingItem : UnlockingItems )
		{
			if( UnlockEndlessShopItem( Player, UnlockingItem ) )
			{
				UnlockedItems.Add( UnlockingItem );
			}
		}

		if( UnlockedItems.Num() > 0 )
		{
			MultiCast_UnlockedItems( UnlockedItems );
		}
	}
	else if( UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get( GetWorld() ) )
	{
		RCO->Server_UnlockEndlessShopItems( this, Player, UnlockingItems );
	}
}

bool AKPCLUnlockSubsystem::UnlockEndlessShopItem( AFGCharacterPlayer* Player, TSubclassOf<UKPCLEndlessShopItem> UnlockingItem )
{
	FKPCLEndlessShopItemInformation Info = UKPCLEndlessShopItem::GetInformation( UnlockingItem, GetWorld() );
	if( Info.mIsUnlockable && !Info.mIsUnlocked )
	{
		if( IsValid( Player ) && IsValid( Player->GetInventory() ) )
		{
			UFGInventoryComponent* PlayerInventory = Player->GetInventory();
			int64 PointCost;
			TArray< FItemAmount > ItemCosts;
			UKPCLEndlessShopItem::GetCosts( UnlockingItem, ItemCosts, PointCost );
			mCurrentPoints -= PointCost;
			for ( FItemAmount ItemCost : ItemCosts )
			{
				PlayerInventory->Remove( ItemCost.ItemClass, ItemCost.Amount );
			}
		}
		else
		{
			return false;
		}
		
		ApplyEndlessShopItem( UnlockingItem );
		
		return mUnlockedShopItems.AddUnique( UnlockingItem ) > INDEX_NONE;
	}
	return false;
}

void AKPCLUnlockSubsystem::ApplyEndlessShopItem( TSubclassOf<UKPCLEndlessShopItem> UnlockingItem )
{
	if( IsValid( UnlockingItem ) )
	{
		AFGRecipeManager* RecipeManager = AFGRecipeManager::Get( GetWorld() );
		AFGSchematicManager* SchematicManager = AFGSchematicManager::Get( GetWorld() );

		for ( const TSubclassOf<UFGRecipe> RecipeToUnlock : UKPCLEndlessShopItem::GetRecipesToUnlock( UnlockingItem ) )
		{
			if( !RecipeManager->IsRecipeAvailable( RecipeToUnlock ) )
			{
				RecipeManager->AddAvailableRecipe( RecipeToUnlock );
			}
		}

		for ( const TSubclassOf<UFGSchematic> SchematicToUnlock : UKPCLEndlessShopItem::GetSchematicsToUnlock( UnlockingItem ) )
		{
			if( !SchematicManager->IsSchematicPurchased( SchematicToUnlock ) )
			{
				SchematicManager->GiveAccessToSchematic( SchematicToUnlock );
			}
		}
	}
}

void AKPCLUnlockSubsystem::ClearShoppingList()
{
	mShoppingList.Empty(  );
}

int64 AKPCLUnlockSubsystem::GetCurrentPoints() const
{
	return mCurrentPoints;
}

bool AKPCLUnlockSubsystem::AddToShoppingList( TSubclassOf<UKPCLEndlessShopItem> UnlockingItem )
{
	FKPCLEndlessShopItemInformation Info = UKPCLEndlessShopItem::GetInformation( UnlockingItem, GetWorld() );
	if( Info.mIsUnlockable && !Info.mIsUnlocked )
	{
		return mShoppingList.AddUnique( UnlockingItem ) > INDEX_NONE;
	}
	return false;
}

void AKPCLUnlockSubsystem::RemoveFromShoppingList( TSubclassOf<UKPCLEndlessShopItem> UnlockingItem )
{
	mShoppingList.Remove( UnlockingItem );
}

bool AKPCLUnlockSubsystem::CanBuyShoppingList( AFGCharacterPlayer* Player, TArray< TSubclassOf< UKPCLEndlessShopItem > > ShoppingList ) const
{
	if( IsValid( Player ) && IsValid( Player->GetInventory() ) )
	{
		UFGInventoryComponent* PlayerInventory = Player->GetInventory();
		int64 PointCost;
		TArray< FItemAmount > ItemCosts;
		UKPCLEndlessShopItem::GetListCosts( ShoppingList, ItemCosts, PointCost );
		if( PointCost <= GetCurrentPoints() )
		{
			for ( const FItemAmount ItemCost : ItemCosts )
			{
				if( !PlayerInventory->HasItems( ItemCost.ItemClass, ItemCost.Amount ) )
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

TArray<TSubclassOf<UKPCLEndlessShopItem>> AKPCLUnlockSubsystem::GetShoppingList( bool FilterUnlockable ) const
{
	if( !FilterUnlockable )
	{
		return mShoppingList;
	}
	
	TArray<TSubclassOf<UKPCLEndlessShopItem>> List = TArray<TSubclassOf<UKPCLEndlessShopItem>>();
	for ( TSubclassOf<UKPCLEndlessShopItem> ShoppingList : mShoppingList )
	{
		FKPCLEndlessShopItemInformation Info = UKPCLEndlessShopItem::GetInformation( ShoppingList, GetWorld() );
		if( Info.mIsUnlockable )
		{
			List.AddUnique( ShoppingList );
		}
	}
	return List;
}

TArray<TSubclassOf<UKPCLEndlessShopItem>> AKPCLUnlockSubsystem::GetUnlockedEndlessShopItems() const
{
	return mUnlockedShopItems;
}

void AKPCLUnlockSubsystem::ES_GetAllMainCats( TArray<TSubclassOf<UKPCLEndlessShopMainCategory>>& MainCats )
{
	mCategoryMap.GenerateKeyArray( MainCats );
}

void AKPCLUnlockSubsystem::ES_GetAllSubCatsOfMainCat( TSubclassOf<UKPCLEndlessShopMainCategory> MainCat, TArray<TSubclassOf<UKPCLEndlessShopSubCategory>>& SubCats )
{
	if( IsValid( MainCat ) )
	{
		mCategoryMap[ MainCat ].GenerateKeyArray( SubCats );
	}
}

TArray<TSubclassOf<UKPCLEndlessShopItem>> AKPCLUnlockSubsystem::ES_GetAllItemsOfSubCat( TSubclassOf< UKPCLEndlessShopMainCategory > MainCat, TSubclassOf<UKPCLEndlessShopSubCategory> SubCat )
{
	if( IsValid( MainCat ) && IsValid( SubCat ) )
	{
		return mCategoryMap[ MainCat ][ SubCat ];
	}
	return TArray<TSubclassOf<UKPCLEndlessShopItem>>();
}

void AKPCLUnlockSubsystem::MultiCast_UnlockedItems_Implementation( const TArray<TSubclassOf<UKPCLEndlessShopItem>>& UnlockedItems )
{
	if( OnEndlessShoppingItemsUnlocked.IsBound() )
	{
		OnEndlessShoppingItemsUnlocked.Broadcast( UnlockedItems );
	}
	BP_OnEndlessShoppingItemsUnlocked( UnlockedItems );
}

void AKPCLUnlockSubsystem::RegisterPlayerState( AFGPlayerState* State )
{
	if( IsValid( State ) )
	{
		mPlayerStates.AddUnique( State );
		UE_LOG( LogKPCL, Warning, TEXT("Register PlayerState by BeginPlay! %s"), *State->GetName() )
		UKPCLNetworkPlayerComponent::GetOrCreateNetworkComponentToPlayerState( GetWorld(), State, mStateComponentClass );
	}
}

void AKPCLUnlockSubsystem::UnlockDecorations( TArray<TSubclassOf<UKPCLDecorationRecipe>> Decorations )
{
	for (TSubclassOf<UKPCLDecorationRecipe> Decoration : Decorations )
	{
		if( IsValid( Decoration ) )
		{
			mUnlockedDecorations.AddUnique( Decoration );
		}
	}
	OnRep_Decoration();
}

void AKPCLUnlockSubsystem::OnRep_Decoration()
{
	for ( TSubclassOf< UKPCLDecorationRecipe > Recipe : mUnlockedDecorations )
	{
		TArray< FItemAmount > Amounts = UFGRecipe::GetProducts( Recipe );
		if( Amounts.Num() > 0 )
		{
			TSubclassOf< UKPCLDecorationActorData > ActorData = TSubclassOf< UKPCLDecorationActorData >{ Amounts[ 0 ].ItemClass };
			if( IsValid( ActorData ) )
			{
				TArray< TSubclassOf< UFGCategory > > Cats;
				UKPCLDecorationActorData::GetSubCategories( ActorData, Cats );
				
				TSubclassOf< UKPCLDecorMainCategory > MainCat = UKPCLDecorationActorData::GetCategory( ActorData );
				TSubclassOf< UKPCLDecorSubCategory > SubCat = Cats[ 0 ];

				TMap< TSubclassOf< UKPCLDecorSubCategory >, TArray< TSubclassOf< UKPCLDecorationRecipe > > >* CatMap = mDecorationCategoryMap.Find( MainCat );
				if( CatMap )
				{
					TArray< TSubclassOf< UKPCLDecorationRecipe > >* Array = CatMap->Find( SubCat );
					if( Array )
					{
						Array->AddUnique( Recipe );
					}
					else
					{
						CatMap->Add( SubCat, { Recipe } );
					}
				}
				else
				{
					TMap< TSubclassOf< UKPCLDecorSubCategory >, TArray< TSubclassOf< UKPCLDecorationRecipe > > > NewCatMap;
					NewCatMap.Add( SubCat, { Recipe } );
					mDecorationCategoryMap.Add( MainCat, NewCatMap );
				}
			}
		}
	}
}

void AKPCLUnlockSubsystem::Decor_GetAllMainCats( TArray<TSubclassOf<UKPCLDecorMainCategory>>& MainCats )
{
	mDecorationCategoryMap.GenerateKeyArray( MainCats );
}

void AKPCLUnlockSubsystem::Decor_GetAllSubCatsOfMainCat( TSubclassOf<UKPCLDecorMainCategory> MainCat, TArray<TSubclassOf<UKPCLDecorSubCategory>>& SubCats )
{
	if( IsValid( MainCat ) )
	{
		mDecorationCategoryMap[ MainCat ].GenerateKeyArray( SubCats );
	}
}

TArray<TSubclassOf<UKPCLDecorationRecipe>> AKPCLUnlockSubsystem::Decor_GetAllItemsOfSubCat( TSubclassOf<UKPCLDecorMainCategory> MainCat, TSubclassOf<UKPCLDecorSubCategory> SubCat )
{
	if( IsValid( MainCat ) && IsValid( SubCat ) )
	{
		return mDecorationCategoryMap[ MainCat ][ SubCat ];
	}
	return TArray<TSubclassOf<UKPCLDecorationRecipe>>();
}
