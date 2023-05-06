// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Description/KPCLEndlessShopItem.h"

#include "FGSchematicManager.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

TArray<TSubclassOf<UFGRecipe>> UKPCLEndlessShopItem::GetRecipesToUnlock( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject()->GetRecipesToRegister();
	}
	return TArray<TSubclassOf<UFGRecipe>>();
}

TArray<TSubclassOf<UFGSchematic>> UKPCLEndlessShopItem::GetSchematicsToUnlock( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject()->GetSchematicsToRegister();
	}
	return TArray<TSubclassOf<UFGSchematic>>();
}

void UKPCLEndlessShopItem::GetCosts( TSubclassOf<UKPCLEndlessShopItem> InClass, TArray<FItemAmount>& Items, int64& Points )
{
	if( IsValid( InClass ) )
	{
		for ( FItemAmount ItemCost : InClass.GetDefaultObject()->mItemCosts )
		{
			bool Found = false;
			for ( FItemAmount& Item : Items )
			{
				if( Item.ItemClass == ItemCost.ItemClass )
				{
					Item.Amount += ItemCost.Amount;
					Found = true;
					break;
				}
			}
			if( !Found )
			{
				Items.Add( ItemCost );
			}
		}
		
		Points += InClass.GetDefaultObject()->mPointCost;
	}
}

void UKPCLEndlessShopItem::GetListCosts( TArray<TSubclassOf<UKPCLEndlessShopItem>> UnlockingItems, TArray<FItemAmount>& Items, int64& Points )
{
	for ( TSubclassOf<UKPCLEndlessShopItem> UnlockingItem : UnlockingItems )
	{
		GetCosts( UnlockingItem, Items, Points );
	}
}

FText UKPCLEndlessShopItem::GetName( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject()->mDisplayName;
	}
	return FText::FromString( "Invalid EndlessShopItem!" );
}

FText UKPCLEndlessShopItem::GetDescription( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject()->mDescription;
	}
	return FText::FromString( "Invalid EndlessShopItem!" );
}

FSlateBrush UKPCLEndlessShopItem::GetSlate( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject()->mIconSlate;
	}
	return FSlateBrush();
}

TArray<TSubclassOf<UKPCLEndlessShopItem>> UKPCLEndlessShopItem::GetSeeAlso( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject()->mSeeAlso;
	}
	return TArray<TSubclassOf<UKPCLEndlessShopItem>>();
}

FKPCLEndlessShopItemInformation UKPCLEndlessShopItem::GetInformation( TSubclassOf<UKPCLEndlessShopItem> InClass, UObject* WorldContext )
{
	FKPCLEndlessShopItemInformation Information;
	
	if( !IsValid( InClass ) )
	{
		return Information;
	}

	if( InClass.GetDefaultObject()->mDependencieSchematics.Num() <= 0 )
	{
		Information.mIsUnlockable = true;
	}
	
	if( IsValid( WorldContext ) && IsValid( WorldContext->GetWorld() ) )
	{
		const AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get( WorldContext->GetWorld() );
		if( IsValid( UnlockSubsystem ) )
		{
			Information.mIsUnlocked = UnlockSubsystem->IsEndlessShopItemUnlocked( InClass );
			if( !Information.mIsUnlocked )
			{
				const AFGSchematicManager* Manager = AFGSchematicManager::Get( WorldContext->GetWorld() );
				if( IsValid( Manager ) )
				{
					bool IsSomethingMissing = false;
					for ( TSubclassOf<UFGSchematic> Schematic : InClass.GetDefaultObject()->mDependencieSchematics )
					{
						if( !IsValid( Schematic ) )
						{
							continue;
						}
				
						if( !Manager->IsSchematicPurchased( Schematic ) )
						{
							IsSomethingMissing = true;
							Information.mMissingDependencies.Add( Schematic );
						}
					}
					Information.mIsUnlockable = !IsSomethingMissing;
				}
			}
		}
	}
	
	return Information;
}

TSubclassOf<UKPCLEndlessShopMainCategory> UKPCLEndlessShopItem::GetMainCategory( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject()->mMainCategory;
	}
	return UKPCLEndlessShopMainCategory::StaticClass();
}

TSubclassOf<UKPCLEndlessShopSubCategory> UKPCLEndlessShopItem::GetSubCategory( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject()->mSubCategory;
	}
	return UKPCLEndlessShopSubCategory::StaticClass();
}

bool UKPCLEndlessShopItem::IsShopItemsIsHidden( TSubclassOf<UKPCLEndlessShopItem> InClass )
{
	return ShouldRegister( InClass );
}

TArray<TSubclassOf<UFGRecipe>> UKPCLEndlessShopItem::GetRecipesToRegister()
{
	return mRecipesToUnlock;
}

TArray<TSubclassOf<UFGSchematic>> UKPCLEndlessShopItem::GetSchematicsToRegister()
{
	return mSchematicsToUnlock;
}
