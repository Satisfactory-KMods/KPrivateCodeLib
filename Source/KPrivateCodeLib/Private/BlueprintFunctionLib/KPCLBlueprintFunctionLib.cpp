// Copyright Coffee Stain Studios. All Rights Reserved.


#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"

#include "AbstractInstanceManager.h"
#include "FGItemPickup_Spawnable.h"
#include "Buildables/FGBuildable.h"
#include "InstanceData.h"
#include "Buildables/FGBuildableFactory.h"
#include "Components/KPCLColoredStaticMesh.h"
#include "ModLoading/ModLoadingLibrary.h"

void UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe( UFGInventoryComponent* Component, int32 Index, TSubclassOf< UFGItemDescriptor > ItemClass )
{
	if( Component )
	{
		if( Component->GetAllowedItemOnIndex( Index ) != ItemClass )
		{
			if( AFGBuildableFactory* Buildable = Cast< AFGBuildableFactory >( Component->GetOwner() ) )
			{
				if( IsInGameThread() || !Buildable->GetReplicationDetailActor( false ) )
				{
					Component->SetAllowedItemOnIndex( Index, ItemClass );
				}
				else
				{
					FFunctionGraphTask::CreateAndDispatchWhenReady( [Component, Index, ItemClass]()
					{
						Component->SetAllowedItemOnIndex( Index, ItemClass );
					}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
				}
			}
			else
			{
				if( IsInGameThread() )
				{
					Component->SetAllowedItemOnIndex( Index, ItemClass );
				}
				else
				{
					FFunctionGraphTask::CreateAndDispatchWhenReady( [Component, Index, ItemClass]()
					{
						Component->SetAllowedItemOnIndex( Index, ItemClass );
					}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
				}
			}
		}
	}
}

UObject* UKPCLBlueprintFunctionLib::GetDefaultSilent( TSubclassOf< UObject > InClass )
{
	if( IsValid( InClass ) )
	{
		return InClass.GetDefaultObject();
	}
	return nullptr;
}

void UKPCLBlueprintFunctionLib::ValidateBuildingModDep( UObject* Building, TArray< FString > ModRefs )
{
	AFGBuildable* Buildable = Cast< AFGBuildable >( Building );
	if( !Buildable )
	{
		return;
	}

	if( !Buildable->HasAuthority() )
	{
		return;
	}


	UModLoadingLibrary* ModLoadingLibrary = GEngine->GetEngineSubsystem< UModLoadingLibrary >();
	if( ensureMsgf( ModLoadingLibrary, TEXT("Cannot find ModLoadingLibrary!!!") ) )
	{
		bool Found = true;
		for( FString ModRefsToCheck : ModRefs )
		{
			if( !ModLoadingLibrary->IsModLoaded( ModRefsToCheck ) )
			{
				Found = false;
				break;
			}
		}

		if( !Found )
		{
			TArray< FInventoryStack > Stacks;
			IFGDismantleInterface::Execute_GetDismantleRefund( Buildable, Stacks );
			AFGCrate* Crate;
			AFGItemPickup_Spawnable::SpawnInventoryCrate( Buildable->GetWorld(), Stacks, Buildable->GetActorLocation(), { Buildable }, Crate );
			IFGDismantleInterface::Execute_Dismantle( Buildable );
		}
	}
}

void UKPCLBlueprintFunctionLib::ResolveHitResult( UObject* Context, const FHitResult& InHitResult, FHitResult& OutHitResult )
{
	FInstanceHandle Handle;
	OutHitResult = InHitResult;
	AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager( Context->GetWorld() );
	if( IsValid( Manager ) )
	{
		if( Manager->ResolveHit( InHitResult, Handle ) )
		{
			OutHitResult.Actor = Handle.GetOwner();
		}
	}
}

void UKPCLBlueprintFunctionLib::ResolveOverlapResult( UObject* Context, const FOverlapResult& InOverlapResult, FOverlapResult& OutOverlapResult )
{
	FInstanceHandle Handle;
	OutOverlapResult = InOverlapResult;
	AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager( Context->GetWorld() );
	if( IsValid( Manager ) )
	{
		if( Manager->ResolveOverlap( InOverlapResult, Handle ) )
		{
			OutOverlapResult.Actor = Handle.GetOwner();
		}
	}
}
