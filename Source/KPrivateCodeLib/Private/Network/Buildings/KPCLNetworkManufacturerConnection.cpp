// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkManufacturerConnection.h"

#include "BFL/KBFL_Inventory.h"
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Kismet/KismetMathLibrary.h"
#include "Network/Buildings/KPCLNetworkCore.h"


AKPCLNetworkManufacturerConnection::AKPCLNetworkManufacturerConnection()
{
	PrimaryActorTick.bCanEverTick = false;
	bBindNetworkComponent = true;
}

void AKPCLNetworkManufacturerConnection::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AKPCLNetworkManufacturerConnection, mSinkOverflow );
	DOREPLIFETIME( AKPCLNetworkManufacturerConnection, mManufacturer );
	DOREPLIFETIME( AKPCLNetworkManufacturerConnection, mLastRecipe );
}

void AKPCLNetworkManufacturerConnection::BeginPlay()
{
	Super::BeginPlay();

	if( HasAuthority() )
	{
		if( !IsValid( mManufacturer ) )
		{
			Execute_Dismantle( this );
		}
	}
}

void AKPCLNetworkManufacturerConnection::Factory_Tick( float dt )
{
	Super::Factory_Tick( dt );

	if( HasAuthority() )
	{
		if( mCycleTimer.Tick( dt ) && GetInventory() && IsSafeToModifyInventorys() )
		{
			for( int32 Idx = 0; Idx < mStartIndexOutput; ++Idx )
			{
				FInventoryStack Stack;
				if( GetInventory()->GetStackFromIndex( Idx, Stack ) )
				{
					if( Stack.HasItems() )
					{
						GetInventory()->RemoveFromIndex( Idx, mManufacturer->GetInputInventory()->AddStack( Stack, true ) );
					}
				}
			}

			for( int32 Idx = 0; Idx < mManufacturer->GetOutputInventory()->GetSizeLinear(); ++Idx )
			{
				FInventoryStack Stack;
				if( mManufacturer->GetOutputInventory()->GetStackFromIndex( Idx, Stack ) )
				{
					if( Stack.HasItems() )
					{
						mManufacturer->GetOutputInventory()->RemoveFromIndex( Idx, GetInventory()->AddStack( Stack, true ) );
					}
				}
			}
		}
	}
}

bool AKPCLNetworkManufacturerConnection::GetNeededResourceClassesThisFrame( TArray< FItemAmount >& Items ) const
{
	Items.Empty();

	if( mStartIndexOutput < 0 || !IsValid( mLastRecipe ) || !IsValid( GetInventory() ) ) return false;

	for( int32 Idx = 0; Idx < mStartIndexOutput; ++Idx )
	{
		FInventoryStack Stack;
		if( GetInventory()->GetStackFromIndex( Idx, Stack ) )
		{
			const int32 MaxItems = GetInventory()->GetSlotSize( Idx );
			Stack.NumItems = MaxItems - Stack.NumItems;
			Stack.Item.SetItemClass( GetInventory()->GetAllowedItemOnIndex( Idx ) );
			if( Stack.HasItems() )
			{
				Items.Add( FItemAmount( Stack.Item.GetItemClass(), Stack.NumItems ) );
			}
		}
	}

	return Items.Num() > 0;
}

bool AKPCLNetworkManufacturerConnection::PushToNetwork( TArray< FItemAmount >& Pushed, float MaxSolidBytes, float MaxFluidBytes )
{
	if( mStartIndexOutput < 0 || !IsValid( mLastRecipe ) || !IsValid( GetInventory() ) ) return false;

	for( int32 Idx = mStartIndexOutput; Idx < GetInventory()->GetSizeLinear(); ++Idx )
	{
		FInventoryStack Stack;
		if( GetInventory()->GetStackFromIndex( Idx, Stack ) )
		{
			if( Stack.HasItems() )
			{
				const int32 MaxAmount = AKPCLNetworkCore::GetMaxItemsByBytes( Stack.Item.GetItemClass(), MaxFluidBytes, MaxSolidBytes );

				const FItemAmount ItemAmount = FItemAmount( Stack.Item.GetItemClass(), FMath::Min< int32 >( Stack.NumItems, MaxAmount ) );
				const int32 SinkDiff = Stack.NumItems - ItemAmount.Amount;

				if( ItemAmount.Amount > 0 )
				{
					FKPCLItemTransferQueue Queue;
					Queue.mAddAmount = false;
					Queue.mAmount = ItemAmount;
					mInventoryQueue.Enqueue( Queue );
					Pushed.Add( ItemAmount );

					bool IsFluid;
					const float Bytes = AKPCLNetworkCore::GetBytesForItemAmount( ItemAmount, IsFluid );
					if( IsFluid )
					{
						MaxFluidBytes -= Bytes;
					}
					else
					{
						MaxSolidBytes -= Bytes;
					}
				}

				if( SinkDiff > 0 && GetIsAllowedToSinkOverflow() )
				{
					FKPCLSinkQueue SinkQueue;
					SinkQueue.mIndex = Idx;
					SinkQueue.mAmount = ItemAmount;
					mSinkQueue.Enqueue( SinkQueue );
				}
			}
		}
	}

	return Pushed.Num() > 0;
}

bool AKPCLNetworkManufacturerConnection::GetFromNetwork( const TArray< FItemAmount >& Received )
{
	if( mStartIndexOutput < 0 || !IsValid( mLastRecipe ) || !IsValid( GetInventory() ) ) return false;

	for( FItemAmount ItemAmount : Received )
	{
		FKPCLItemTransferQueue Queue;
		Queue.mAddAmount = true;
		Queue.mAmount = ItemAmount;
		mInventoryQueue.Enqueue( Queue );
	}
	return true;
}

AFGResourceSinkSubsystem* AKPCLNetworkManufacturerConnection::GetSinkSub()
{
	if( !IsValid( mSinkSub ) )
	{
		mSinkSub = AFGResourceSinkSubsystem::Get( GetWorld() );
	}
	return mSinkSub;
}

void AKPCLNetworkManufacturerConnection::SetNewRecipe( TSubclassOf< UFGRecipe > NewRecipe )
{
	if( IsValid( GetInventory() ) && IsSafeToModifyInventorys() )
	{
		if( IsValid( mManufacturer->GetInputInventory() ) && IsValid( mManufacturer->GetOutputInventory() ) )
		{
			mLastRecipe = NewRecipe;

			GetInventory()->Empty();
			int32 TotalSize = mManufacturer->GetInputInventory()->GetSizeLinear();
			TotalSize += mManufacturer->GetOutputInventory()->GetSizeLinear();
			GetInventory()->Resize( TotalSize );

			const TArray< FItemAmount > In = UFGRecipe::GetIngredients( mLastRecipe );
			const TArray< FItemAmount > Out = UFGRecipe::GetProducts( mLastRecipe );
			mCycleTimer.mTime = mManufacturer->GetProductionCycleTime();

			TArray< FItemAmount > Amounts = In;
			Amounts.Append( Out );

			for( int32 Idx = 0; Idx < Amounts.Num(); ++Idx )
			{
				if( GetInventory()->IsValidIndex( Idx ) )
				{
					GetInventory()->AddArbitrarySlotSize( Idx, Amounts[ Idx ].Amount * 2 );
					UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe( GetInventory(), Idx, Amounts[ Idx ].ItemClass );
				}
			}

			for( int32 Idx = 0; Idx < In.Num(); ++Idx )
			{
				mManufacturer->GetInputInventory()->AddArbitrarySlotSize( Idx, In[ Idx ].Amount * 2 );
			}

			for( int32 Idx = 0; Idx < Out.Num(); ++Idx )
			{
				mManufacturer->GetOutputInventory()->AddArbitrarySlotSize( Idx, Out[ Idx ].Amount * 2 );
			}

			mStartIndexOutput = In.Num();

			if( IsInGameThread() )
			{
				BP_OnRecipeChanged( NewRecipe );
			}
			else
			{
				// we want to dismantle it with our Manu
				FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, NewRecipe ]()
				{
					BP_OnRecipeChanged( NewRecipe );
				}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
			}
		}
	}
}

void AKPCLNetworkManufacturerConnection::OnRep_Manufacturer()
{
	BP_OnNewManufacturerSet( mManufacturer );
}

void AKPCLNetworkManufacturerConnection::OnRep_Recipe()
{
	BP_OnRecipeChanged( mLastRecipe );
}

void AKPCLNetworkManufacturerConnection::SetManufacturer( AFGBuildableManufacturer* NewManufacturer )
{
	mManufacturer = NewManufacturer;

	BP_OnNewManufacturerSet( mManufacturer );
}

bool AKPCLNetworkManufacturerConnection::GetIsAllowedToSinkOverflow() const
{
	return mSinkOverflow;
}

void AKPCLNetworkManufacturerConnection::SetIsAllowedToSinkOverflow( bool IsAllowed )
{
	if( HasAuthority() )
	{
		mSinkOverflow = IsAllowed;;
	}
	else if( UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get( GetWorld() ) )
	{
		RCO->Server_SetSinkOverflowItem( this, IsAllowed );
	}
}

bool AKPCLNetworkManufacturerConnection::IsSafeToModifyInventorys() const
{
	if( IsValid( mManufacturer ) )
	{
		if( ( mManufacturer->GetProductionProgress() > 0.0f && mManufacturer->GetProductionProgress() < 0.0f ) || !mManufacturer->IsProducing() )
		{
			return IsValid( mManufacturer->GetInputInventory() ) && IsValid( mManufacturer->GetOutputInventory() );
		}
	}
	return false;
}
