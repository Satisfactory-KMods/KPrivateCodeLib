#include "Structures/KPCLInventoryStructure.h"

#include "FGInventoryLibrary.h"
#include "KPrivateCodeLibModule.h"

FKPCLInventoryStructure::FKPCLInventoryStructure()
{
}

FKPCLInventoryStructure::FKPCLInventoryStructure( FName ComponentName )
{
	mComponentName = ComponentName;
}

FKPCLInventoryStructure::FKPCLInventoryStructure( UFGInventoryComponent* InventoryComponent )
{
	mInventoryComponent = InventoryComponent;
	if( GetInventory_Main() )
	{
		mComponentName = GetInventory_Main()->GetFName();
	}
}

void FKPCLInventoryStructure::InitInventory( AActor* Owner, FName ComponentName )
{
	if( ComponentName != FName() )
	{
		mComponentName = ComponentName;
	}

	if( !Owner )
	{
		UE_LOG( LogKPCL, Error, TEXT("InitInventory with invalid Owner!") )
		return;
	}

	if( Owner->HasAuthority() && !mInventoryComponent )
	{
		if( !TryFindComponentOnOwner( mInventoryComponent, Owner, mComponentName ) )
		{
			mInventoryComponent = UFGInventoryLibrary::CreateInventoryComponent( Owner, mComponentName );
		}
	}
	mComponentName = GetInventory_Main()->GetFName();

	ConfigureInventory();
}

void FKPCLInventoryStructure::ConfigureInventory()
{
	if( mInventorySize > 0 )
	{
		if( !mDontResizeOnBeginPlay && GetInventory() )
		{
			GetInventory()->Resize( mInventorySize );
			return;
		}

		if( !GetInventory() )
		{
			UE_LOG( LogKPCL, Error, TEXT("Skip resizing because invalid Inventory!?") )
			return;
		}
	}
	else
	{
		UE_LOG( LogKPCL, Error, TEXT("Ignore ConfigureInventory because mInventorySize <= 0 (%d)"), mInventorySize );
	}
}

UFGInventoryComponent* FKPCLInventoryStructure::GetInventory() const
{
	return IsReplicated() ? GetInventory_Detailed() : GetInventory_Main();
}

UFGInventoryComponent* FKPCLInventoryStructure::GetInventory_Detailed() const
{
	return mInventoryComponent_DetailedActor;
}

UFGInventoryComponent* FKPCLInventoryStructure::GetInventory_Main() const
{
	return mInventoryComponent;
}

AFGReplicationDetailActor* FKPCLInventoryStructure::GetReplicationDetailedActor() const
{
	if( IsReplicated() )
	{
		return mCachedReplicationDetailActor;
	}
	return nullptr;
}

void FKPCLInventoryStructure::SetReplicated( UFGInventoryComponent* ReplicationComponent )
{
	if( !ReplicationComponent )
	{
		UE_LOG( LogKPCL, Error, TEXT("Try to SetReplicated with invalid ReplicationComponent!") )
		return;
	}

	mInventoryComponent_DetailedActor = ReplicationComponent;
	mCachedReplicationDetailActor = mInventoryComponent_DetailedActor->GetOwner< AFGReplicationDetailActor >();
	FlushToReplication();
	GetInventory_Main()->SetLocked( true );
	UE_LOG( LogKPCL, Warning, TEXT("SetReplicated, Active Inventory: %s from %s"), *GetInventory()->GetName(), *mCachedReplicationDetailActor->GetName() )
}

void FKPCLInventoryStructure::SetInventorySize( int32 Size )
{
	if( Size > 0 )
	{
		mInventorySize = Size;
		if( GetInventory() )
		{
			GetInventory()->Resize( Size );
		}
	}
	else
	{
		UE_LOG( LogKPCL, Error, TEXT("Try to SetInventorySize wind invalid Size (<= 0)!") )
	}
}

void FKPCLInventoryStructure::ClearReplication( bool PreventFlush )
{
	GetInventory_Main()->SetLocked( false );
	if( !PreventFlush )
	{
		UE_LOG( LogKPCL, Warning, TEXT("ClearReplication->FlushToOwner, Detailed is now %d ; Main is now %d"), GetInventory_Detailed() != nullptr, GetInventory_Main() != nullptr )
		FlushToOwner();
	}

	mInventoryComponent_DetailedActor = nullptr;
	UE_LOG( LogKPCL, Warning, TEXT("ClearReplication, Active Inventory: %s"), *GetInventory()->GetName() )
}

void FKPCLInventoryStructure::FlushToOwner()
{
	if( !HasAuthority() )
	{
		return;
	}

	if( GetInventory_Main() && GetInventory_Detailed() )
	{
		GetInventory_Main()->CopyFromOtherComponent( GetInventory_Detailed() );
	}
}

void FKPCLInventoryStructure::FlushToReplication()
{
	if( !HasAuthority() )
	{
		return;
	}

	if( GetInventory_Main() && GetInventory_Detailed() )
	{
		GetInventory_Detailed()->CopyFromOtherComponent( GetInventory_Main() );
	}
}

void FKPCLInventoryStructure::LoadDefaultData( const FKPCLInventoryStructure& InventoryData )
{
	mOverwriteSavedSize = InventoryData.mOverwriteSavedSize;
	mDontResizeOnBeginPlay = InventoryData.mDontResizeOnBeginPlay;

	if( mOverwriteSavedSize )
	{
		mInventorySize = InventoryData.mInventorySize;
	}

	if( InventoryData.mComponentName != mComponentName )
	{
		if( GetInventory_Main() )
		{
			UE_LOG( LogKPCL, Error, TEXT("RemoveInventory! because Component name dont match!") )
			RemoveInventory();
		}
		mComponentName = InventoryData.mComponentName;
	}
}

void FKPCLInventoryStructure::RemoveInventory()
{
	if( HasAuthority() )
	{
		if( GetInventory_Main() )
		{
			UE_LOG( LogKPCL, Warning, TEXT("RemoveInventory %s!"), *GetInventory_Main()->GetName() )
			GetInventory_Main()->DestroyComponent();
			mInventoryComponent = nullptr;
		}
	}
}

bool FKPCLInventoryStructure::IsValid() const
{
	if( !HasAuthority() )
	{
		return GetInventory() != nullptr;
	}
	return GetStrucOwner() != nullptr && GetInventory() != nullptr;
}

bool FKPCLInventoryStructure::IsReplicated() const
{
	return GetInventory_Detailed() != nullptr;
}

bool FKPCLInventoryStructure::HasAuthority() const
{
	if( !GetStrucOwner() ) return false;
	return GetStrucOwner()->HasAuthority();
}

AActor* FKPCLInventoryStructure::GetStrucOwner() const
{
	return GetInventory_Main()->GetOwner();
}

UFGInventoryComponent* UKPCLInventoryLibrary::GetInventory( FKPCLInventoryStructure InventoryData )
{
	return InventoryData.GetInventory();
}

UFGInventoryComponent* UKPCLInventoryLibrary::GetInventory_Main( FKPCLInventoryStructure InventoryData )
{
	return InventoryData.GetInventory_Main();
}

UFGInventoryComponent* UKPCLInventoryLibrary::GetInventory_Detailed( FKPCLInventoryStructure InventoryData )
{
	return InventoryData.GetInventory_Detailed();
}

AFGReplicationDetailActor* UKPCLInventoryLibrary::GetReplicationDetailActor( FKPCLInventoryStructure InventoryData )
{
	return InventoryData.GetReplicationDetailedActor();
}

bool UKPCLInventoryLibrary::IsReplicated( FKPCLInventoryStructure InventoryData )
{
	return InventoryData.IsReplicated();
}

bool UKPCLInventoryLibrary::IsValid( FKPCLInventoryStructure InventoryData )
{
	return InventoryData.IsValid();
}
