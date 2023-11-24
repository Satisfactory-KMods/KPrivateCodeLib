// Copyright Coffee Stain Studios. All Rights Reserved.


#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"

#include "AbstractInstanceManager.h"
#include "FGItemPickup_Spawnable.h"
#include "InstanceData.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableFactory.h"
#include "Components/KPCLColoredStaticMesh.h"
#include "ModLoading/ModLoadingLibrary.h"

void UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe( UFGInventoryComponent* Component, int32 Index, TSubclassOf< UFGItemDescriptor > ItemClass ) {
	if( Component ) {
		if( Component->GetAllowedItemOnIndex( Index ) != ItemClass ) {
			if( AFGBuildableFactory* Buildable = Cast< AFGBuildableFactory >( Component->GetOwner( ) ) ) {
				if( IsInGameThread( ) || !Buildable->GetReplicationDetailActor( false ) ) {
					Component->SetAllowedItemOnIndex( Index, ItemClass );
				}
				else {
					FFunctionGraphTask::CreateAndDispatchWhenReady( [Component, Index, ItemClass]( ) {
						Component->SetAllowedItemOnIndex( Index, ItemClass );
					}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
				}
			}
			else {
				if( IsInGameThread( ) ) {
					Component->SetAllowedItemOnIndex( Index, ItemClass );
				}
				else {
					FFunctionGraphTask::CreateAndDispatchWhenReady( [Component, Index, ItemClass]( ) {
						Component->SetAllowedItemOnIndex( Index, ItemClass );
					}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
				}
			}
		}
	}
}

UObject* UKPCLBlueprintFunctionLib::GetDefaultSilent( TSubclassOf< UObject > InClass ) {
	if( IsValid( InClass ) ) {
		return InClass.GetDefaultObject( );
	}
	return nullptr;
}

void UKPCLBlueprintFunctionLib::ResolveHitResult( UObject* Context, const FHitResult& InHitResult, FHitResult& OutHitResult ) {
	FInstanceHandle Handle;
	OutHitResult = InHitResult;
	AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager( Context->GetWorld( ) );
	if( IsValid( Manager ) ) {
		Manager->ResolveHit( InHitResult, Handle );
	}
}

void UKPCLBlueprintFunctionLib::ResolveOverlapResult( UObject* Context, const FOverlapResult& InOverlapResult, FOverlapResult& OutOverlapResult ) {
	FInstanceHandle Handle;
	OutOverlapResult = InOverlapResult;
	AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager( Context->GetWorld( ) );
	if( IsValid( Manager ) ) {
		Manager->ResolveOverlap( InOverlapResult, Handle );
	}
}
