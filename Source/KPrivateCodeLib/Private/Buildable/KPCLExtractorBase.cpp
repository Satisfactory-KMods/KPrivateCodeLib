// ILikeBanas


#include "Buildable/KPCLExtractorBase.h"

#include "AbstractInstanceManager.h"
#include "FGCrate.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPlayerController.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
#include "KPrivateCodeLibModule.h"
#include "BFL/KBFL_Player.h"
#include "Components/KPCLBetterIndicator.h"

#include "Net/UnrealNetwork.h"

#include "Replication/KPCLDefaultRCO.h"
#include "Replication/KPCLReplicationActor_ExtractorBase.h"
#include "Structures/KPCLFunctionalStructure.h"

AKPCLExtractorBase::AKPCLExtractorBase( ): mFGPowerConnection( nullptr ), mCustomProductionStateIndicator( nullptr ) {
	mInventoryDatas.Add( FKPCLInventoryStructure( "Inv_Main" ) );

	bReplicates = true;
	mFactoryTickFunction.bCanEverTick = true;
	PrimaryActorTick.bCanEverTick = true;
}

#if WITH_EDITOR
void AKPCLExtractorBase::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent ) {
	Super::PostEditChangeProperty( PropertyChangedEvent );

	if( PropertyChangedEvent.GetPropertyName( ) == GET_MEMBER_NAME_CHECKED( AKPCLExtractorBase, mPowerOptions ) ) {
		mPowerConsumption = mPowerOptions.GetMaxPowerConsume( );
	}
}

void AKPCLExtractorBase::PostEditChangeChainProperty( FPropertyChangedChainEvent& PropertyChangedEvent ) {
	Super::PostEditChangeChainProperty( PropertyChangedEvent );

	if( PropertyChangedEvent.GetPropertyName( ) == GET_MEMBER_NAME_CHECKED( AKPCLExtractorBase, mPowerOptions ) ) {
		mPowerConsumption = mPowerOptions.GetMaxPowerConsume( );
	}
}
#endif

bool AKPCLExtractorBase::ShouldSave_Implementation( ) const {
	return true;
}

void AKPCLExtractorBase::InitInventorys( ) {
	if( !HasAuthority( ) ) {
		return;
	}

	for( int32 Idx = 0; Idx < mInventoryDatas.Num( ); ++Idx ) {
		if( !mInventoryDatasSaved.IsValidIndex( Idx ) ) {
			mInventoryDatasSaved.Add( mInventoryDatas[ Idx ] );
		}

		mInventoryDatasSaved[ Idx ].LoadDefaultData( mInventoryDatas[ Idx ] );

		PreInitInventoryIdex( Idx );
		mInventoryDatasSaved[ Idx ].InitInventory( this );
		PostInitInventoryIdex( Idx );
	}

	for( int32 Idx = 0; Idx < mInventoryDatasSaved.Num( ); ++Idx ) {
		if( !mInventoryDatas.IsValidIndex( Idx ) ) {
			mInventoryDatasSaved[ Idx ].RemoveInventory( );
		}
	}
	mInventoryDatasSaved.SetNum( mInventoryDatas.Num( ) );

	if( !GetInventorysAreValid( ) ) {
		GetWorldTimerManager( ).SetTimerForNextTick( this, &AKPCLExtractorBase::InitInventorys );
	}
	else {
		ReconfigureInventory( );
		SetBelts( );
	}
}

bool AKPCLExtractorBase::GetInventorysAreValid( ) const {
	for( int32 Idx = 0; Idx < mInventoryDatasSaved.Num( ); ++Idx ) {
		if( !IsValid( mInventoryDatasSaved[ Idx ].GetInventory( ) ) ) {
			return false;
		}
	}
	return true;
}

void AKPCLExtractorBase::SetBelts( ) {
	if( !IsValid( GetInventory( ) ) ) {
		UE_LOG( LogKPCL, Warning, TEXT("SetBelts, with invalid inventory! > SKIP!") )
		return;
	}

	TArray< UFGFactoryConnectionComponent* > BeltsToSet = GetAllConv( );
	if( BeltsToSet.Num( ) > 0 ) {
		for( UFGFactoryConnectionComponent* BeltConnection : BeltsToSet ) {
			if( BeltConnection ) {
				BeltConnection->SetInventory( GetInventory( ) );
			}
		}
	}

	TArray< UFGPipeConnectionFactory* > PipesToSet = GetAllPipes( );
	if( PipesToSet.Num( ) > 0 ) {
		for( UFGPipeConnectionFactory* PipeConnection : PipesToSet ) {
			if( PipeConnection ) {
				PipeConnection->SetInventory( GetInventory( ) );
			}
		}
	}
}

void AKPCLExtractorBase::HandleIndicator( ) {
	if( !GetHasPower( ) ) {
		mCurrentState = ENewProductionState::NoPower;
		return;
	}
	if( IsProductionPaused( ) ) {
		mCurrentState = ENewProductionState::Paused;
		return;
	}
	if( IsProducing( ) ) {
		mCurrentState = ENewProductionState::Producing;
		return;
	}
	mCurrentState = ENewProductionState::Idle;
}

void AKPCLExtractorBase::ApplyNewProductionState( ENewProductionState NewState ) {
	mLastState = NewState;
	mCurrentState = NewState;

	if( mCustomProductionStateIndicator ) {
		mCustomProductionStateIndicator->SetState( NewState );
	}

	bOneStateWasNotApplied = false;
	for( const int32 CustomIndicatorHandleIndex : mCustomIndicatorHandleIndexes ) {
		if( mInstanceHandles.IsValidIndex( CustomIndicatorHandleIndex ) ) {
			if( !mInstanceHandles[ CustomIndicatorHandleIndex ]->IsInstanced( ) ) {
				bOneStateWasNotApplied = true;
				continue;
			}
			AIO_UpdateCustomFloat( mDefaultIndex, mIntensity, CustomIndicatorHandleIndex, true );
			AIO_UpdateCustomFloatAsColor( mDefaultIndex + 1, mStateColors[ static_cast< uint8 >( NewState ) ], CustomIndicatorHandleIndex, true );
			AIO_UpdateCustomFloat( mDefaultIndex + 4, mPulsStates.Contains( NewState ), CustomIndicatorHandleIndex, true );
		}
	}
}

UFGFactoryConnectionComponent* AKPCLExtractorBase::GetConv( int Index, ECKPCLDirection Direction ) const {
	if( ( Direction == KPCLInput || Direction == KPCLAny ) && mConnectionMap[ EKPCLConnectionType::ConvIn ].IsValidIndex( Index ) ) {
		return Cast< UFGFactoryConnectionComponent >( mConnectionMap[ EKPCLConnectionType::ConvIn ][ Index ] );
	}

	if( ( Direction == KPCLOutput || Direction == KPCLAny ) && mConnectionMap[ EKPCLConnectionType::ConvOut ].IsValidIndex( Index ) ) {
		return Cast< UFGFactoryConnectionComponent >( mConnectionMap[ EKPCLConnectionType::ConvOut ][ Index ] );
	}

	return nullptr;
}

TArray< UFGFactoryConnectionComponent* > AKPCLExtractorBase::GetAllConv( ECKPCLDirection Direction ) const {
	TArray< UFGFactoryConnectionComponent* > Return;
	TArray< UFGConnectionComponent* > All;

	if( Direction == KPCLInput || Direction == KPCLAny ) {
		All.Append( mConnectionMap[ EKPCLConnectionType::ConvIn ] );
	}
	if( Direction == KPCLOutput || Direction == KPCLAny ) {
		All.Append( mConnectionMap[ EKPCLConnectionType::ConvOut ] );
	}

	for( UFGConnectionComponent* Conv : All ) {
		if( IsValid( Conv ) ) {
			Return.AddUnique( Cast< UFGFactoryConnectionComponent >( Conv ) );
		}
	}

	return Return;
}

UFGPipeConnectionFactory* AKPCLExtractorBase::GetPipe( int Index, ECKPCLDirection Direction ) const {
	if( ( Direction == KPCLInput || Direction == KPCLAny ) && mConnectionMap[ EKPCLConnectionType::PipeIn ].IsValidIndex( Index ) ) {
		return Cast< UFGPipeConnectionFactory >( mConnectionMap[ EKPCLConnectionType::PipeIn ][ Index ] );
	}

	if( ( Direction == KPCLOutput || Direction == KPCLAny ) && mConnectionMap[ EKPCLConnectionType::PipeOut ].IsValidIndex( Index ) ) {
		return Cast< UFGPipeConnectionFactory >( mConnectionMap[ EKPCLConnectionType::PipeOut ][ Index ] );
	}

	return nullptr;
}

TArray< UFGPipeConnectionFactory* > AKPCLExtractorBase::GetAllPipes( ECKPCLDirection Direction ) const {
	TArray< UFGPipeConnectionFactory* > Return;
	TArray< UFGConnectionComponent* > All;

	if( Direction == KPCLInput || Direction == KPCLAny ) {
		All.Append( mConnectionMap[ EKPCLConnectionType::PipeIn ] );
	}
	if( Direction == KPCLOutput || Direction == KPCLAny ) {
		All.Append( mConnectionMap[ EKPCLConnectionType::PipeOut ] );
	}

	for( UFGConnectionComponent* Conv : All ) {
		if( IsValid( Conv ) ) {
			Return.AddUnique( Cast< UFGPipeConnectionFactory >( Conv ) );
		}
	}

	return Return;
}

void AKPCLExtractorBase::BeginPlay( ) {
	Super::BeginPlay( );

	InitComponents( );

	if( HasAuthority( ) ) {
		InitInventorys( );
		ReconfigureInventory( );
		HandlePowerInit( );
		SetBelts( );
	}

	if( !bDeferBeginPlay ) {
		ReadyForVisuelUpdate( );
	}

	InitAudioConfig( );
}

void AKPCLExtractorBase::OnBuildEffectFinished( ) {
	Super::OnBuildEffectFinished( );
	ReadyForVisuelUpdate( );
}

void AKPCLExtractorBase::OnBuildEffectActorFinished( ) {
	Super::OnBuildEffectActorFinished( );
	ReadyForVisuelUpdate( );
}

void AKPCLExtractorBase::ReadyForVisuelUpdate( ) {
	InitMeshOverwriteInformation( );
	ApplyNewProductionState( GetCurrentProductionState( ) );
}

void AKPCLExtractorBase::InitAudioConfig( ) {
	UConfigPropertyFloat* Property = mAudioConfig.GetPropertyAsType( GetWorld( ) );
	if( IsValid( Property ) ) {
		TArray< UAudioComponent* > AudioComponents;
		GetComponents( AudioComponents );

		if( AudioComponents.Num( ) > 0 ) {
			for( UAudioComponent* AudioComponent : AudioComponents ) {
				mAudioComponents.Add( FKPCLAudioComponent( AudioComponent ) );
			}
			Property->OnPropertyValueChanged.AddUniqueDynamic( this, &AKPCLExtractorBase::OnAudioConfigChanged_Native );
			OnAudioConfigChanged_Native( );
		}
	}
}

void AKPCLExtractorBase::OnAudioConfigChanged_Native( ) {
	for( FKPCLAudioComponent AudioComponent : mAudioComponents ) {
		AudioComponent.SetVolumePercent( mAudioConfig.GetValue( GetWorld( ) ) );
	}
	OnAudioConfigChanged( );
}

void AKPCLExtractorBase::StartIsLookedAtForDismantle_Implementation( AFGCharacterPlayer* byCharacter ) {
	UpdateInstancesForOutline( );
	Super::StartIsLookedAtForDismantle_Implementation( byCharacter );
}

void AKPCLExtractorBase::StartIsLookedAt_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state ) {
	UpdateInstancesForOutline( );
	Super::StartIsLookedAt_Implementation( byCharacter, state );
}

void AKPCLExtractorBase::StartIsAimedAtForColor_Implementation( AFGCharacterPlayer* byCharacter, bool isValid ) {
	UpdateInstancesForOutline( );
	Super::StartIsAimedAtForColor_Implementation( byCharacter, isValid );
}

void AKPCLExtractorBase::StartIsLookedAtForConnection( AFGCharacterPlayer* byCharacter, UFGCircuitConnectionComponent* overlappingConnection ) {
	UpdateInstancesForOutline( );
	Super::StartIsLookedAtForConnection( byCharacter, overlappingConnection );
}

void AKPCLExtractorBase::UpdateInstancesForOutline( ) const {
	TArray< UKPCLColoredStaticMesh* > ColoredStaticMeshes;
	GetComponents< UKPCLColoredStaticMesh >( ColoredStaticMeshes );
	for( UKPCLColoredStaticMesh* ColoredStaticMesh : ColoredStaticMeshes ) {
		if( ColoredStaticMesh ) {
			ColoredStaticMesh->ApplyTransformToComponent( );
		}
	}
}

void AKPCLExtractorBase::InitMeshOverwriteInformation( ) {
	if( !DoesContainLightweightInstances_Native( ) ) {
		return;
	}

	if( mInstanceHandles.Num( ) <= 0 ) {
		GetWorldTimerManager( ).SetTimerForNextTick( this, &AKPCLExtractorBase::InitMeshOverwriteInformation );
		return;
	}

	if( !mInstanceHandles[ 0 ]->IsInstanced( ) ) {
		GetWorldTimerManager( ).SetTimerForNextTick( this, &AKPCLExtractorBase::InitMeshOverwriteInformation );
		return;
	}

	for( int32 Idx = 0; Idx < mInstanceHandles.Num( ); ++Idx ) {
		ApplyMeshOverwriteInformation( Idx );

		FKPCLMeshOverwriteInformation Information;
		if( ShouldOverwriteIndexHandle( Idx, Information ) ) {
			ApplyMeshInformation( Information );
		}
	}
	ApplyNewProductionState( GetCurrentProductionState( ) );
}

void AKPCLExtractorBase::ReApplyColorForIndex( int32 Idx, const FFactoryCustomizationData& customizationData ) {
	if( !mInstanceHandles.IsValidIndex( Idx ) || !DoesContainLightweightInstances_Native( ) ) {
		return;
	}

	if( mInstanceHandles[ Idx ]->IsInstanced( ) && DoesContainLightweightInstances_Native( ) ) {
		TArray< float > Datas = customizationData.Data;
		Datas.SetNum( mInstanceDataCDO->GetInstanceData( )[ Idx ].NumCustomDataFloats );
		if( mCachedCustomData.Contains( Idx ) ) {
			for( TTuple< int, float > Result : mCachedCustomData[ Idx ] ) {
				Datas[ Result.Key ] = Result.Value;
			}
		}
		AAbstractInstanceManager::SetCustomPrimitiveDataOnHandle( mInstanceHandles[ Idx ], Datas, true );
	}
}

void AKPCLExtractorBase::ApplyCustomizationData_Native( const FFactoryCustomizationData& customizationData ) {
	Super::ApplyCustomizationData_Native( customizationData );

	if( DoesContainLightweightInstances_Native( ) ) {
		for( int32 Idx = 0; Idx < mInstanceHandles.Num( ); ++Idx ) {
			ReApplyColorForIndex( Idx, customizationData );
		}
	}
}

void AKPCLExtractorBase::SetCustomizationData_Native( const FFactoryCustomizationData& customizationData ) {
	Super::SetCustomizationData_Native( customizationData );

	if( DoesContainLightweightInstances_Native( ) ) {
		for( int32 Idx = 0; Idx < mInstanceHandles.Num( ); ++Idx ) {
			ReApplyColorForIndex( Idx, customizationData );
		}
	}
}

void AKPCLExtractorBase::ApplyMeshOverwriteInformation( int32 Idx ) {
	for( FKPCLMeshOverwriteInformation MeshOverwriteInformation : mDefaultMeshOverwriteInformations ) {
		if( MeshOverwriteInformation.mOverwriteHandleIndex == Idx ) {
			ApplyMeshInformation( MeshOverwriteInformation );
			return;
		}
	}

	for( FKPCLMeshOverwriteInformation MeshOverwriteInformation : mMeshOverwriteInformations ) {
		if( MeshOverwriteInformation.mOverwriteHandleIndex == Idx ) {
			ApplyMeshInformation( MeshOverwriteInformation );
			return;
		}
	}
}

void AKPCLExtractorBase::ApplyMeshInformation( FKPCLMeshOverwriteInformation Information ) {
	if( !Information.mUseCustomTransform ) {
		AIO_OverwriteInstanceData( Information.mOverwriteMesh, Information.mOverwriteHandleIndex );
	}
	else {
		AIO_OverwriteInstanceData_Transform( Information.mOverwriteMesh, Information.mCustomTransform, Information.mOverwriteHandleIndex );
	}
}

bool AKPCLExtractorBase::ShouldOverwriteIndexHandle( int32 Idx, FKPCLMeshOverwriteInformation& Information ) {
	return false;
}

bool AKPCLExtractorBase::AIO_OverwriteInstanceData( UStaticMesh* Mesh, int32 Idx ) {
	if( !IsInGameThread( ) ) {
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, Mesh, Idx ]( ) {
			AIO_OverwriteInstanceData( Mesh, Idx );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}

	if( Idx > INDEX_NONE && IsValid( Mesh ) ) {
		TArray< FInstanceData > Datas = mInstanceDataCDO->GetInstanceData( );
		if( Datas.IsValidIndex( Idx ) ) {
			return AIO_OverwriteInstanceData_Transform( Mesh, Datas[ Idx ].RelativeTransform, Idx );
		}
	}
	return false;
}

bool AKPCLExtractorBase::AIO_OverwriteInstanceData_Transform( UStaticMesh* Mesh, FTransform NewRelativTransform, int32 Idx ) {
	if( !IsInGameThread( ) ) {
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, Mesh, NewRelativTransform, Idx ]( ) {
			AIO_OverwriteInstanceData_Transform( Mesh, NewRelativTransform, Idx );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}

	if( Idx > INDEX_NONE && IsValid( Mesh ) ) {
		AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager( GetWorld( ) );
		TArray< FInstanceData > Datas = mInstanceDataCDO->GetInstanceData( );
		if( Datas.IsValidIndex( Idx ) && mInstanceHandles.IsValidIndex( Idx ) && IsValid( Manager ) ) {
			FInstanceData Data = Datas[ Idx ];
			Data.RelativeTransform = NewRelativTransform;
			Data.StaticMesh = Mesh;

			if( mInstanceHandles[ Idx ]->IsInstanced( ) ) {
				Manager->RemoveInstance( mInstanceHandles[ Idx ] );
			}
			mInstanceHandles[ Idx ] = new FInstanceHandle( );
			Manager->SetInstanced( this, GetActorTransform( ), Data, mInstanceHandles[ Idx ] );
			mCachedTransforms.Add( Idx, NewRelativTransform * GetActorTransform( ) );

			ApplyCustomizationData_Native( mCustomizationData );
			SetCustomizationData_Native( mCustomizationData );

			return true;
		}
	}

	return false;
}

bool AKPCLExtractorBase::AIO_UpdateCustomFloat( int32 FloatIndex, float Data, int32 InstanceIdx, bool MarkDirty ) {
	if( !IsInGameThread( ) ) {
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, FloatIndex, Data, InstanceIdx, MarkDirty ]( ) {
			AIO_UpdateCustomFloat( FloatIndex, Data, InstanceIdx, MarkDirty );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}

	if( mInstanceHandles.IsValidIndex( InstanceIdx ) ) {
		if( mInstanceHandles[ InstanceIdx ]->IsInstanced( ) ) {
			//UE_LOG( LogKPCL, Error, TEXT("mInstanceHandles[ %d ]->SetPrimitiveDataByID( %f, %d, %d ); IsValid(%d), Component(%d), Owner(%d)"), InstanceIdx, Data, FloatIndex, MarkDirty, mInstanceHandles[ InstanceIdx ]->IsValid(), IsValid( mInstanceHandles[ InstanceIdx ]->GetInstanceComponent() ), mInstanceHandles[ InstanceIdx ]->GetOwner() == this )
			mInstanceHandles[ InstanceIdx ]->SetPrimitiveDataByID( Data/** float that we want to set */, FloatIndex /** Index where we want to set */, true );
			if( mCachedCustomData.Contains( InstanceIdx ) ) {
				mCachedCustomData[ InstanceIdx ].Add( FloatIndex, Data );
			}
			else {
				TMap< int32, float > Map;
				Map.Add( FloatIndex, Data );
				mCachedCustomData.Add( InstanceIdx, Map );
			}
			return true;
		}
	}

	return false;
}

bool AKPCLExtractorBase::AIO_UpdateCustomFloatAsColor( int32 StartFloatIndex, FLinearColor Data, int32 InstanceIdx, bool MarkDirty ) {
	if( !IsInGameThread( ) ) {
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, StartFloatIndex, Data, InstanceIdx, MarkDirty ]( ) {
			AIO_UpdateCustomFloatAsColor( StartFloatIndex, Data, InstanceIdx, MarkDirty );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}

	AIO_UpdateCustomFloat( StartFloatIndex, Data.R, InstanceIdx, MarkDirty );
	AIO_UpdateCustomFloat( StartFloatIndex + 1, Data.G, InstanceIdx, MarkDirty );
	return AIO_UpdateCustomFloat( StartFloatIndex + 2, Data.B, InstanceIdx, MarkDirty );
}

bool AKPCLExtractorBase::AIO_SetInstanceHidden( int32 InstanceIdx, bool IsHidden ) {
	if( !IsInGameThread( ) ) {
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, InstanceIdx, IsHidden ]( ) {
			AIO_SetInstanceHidden( InstanceIdx, IsHidden );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}

	if( mInstanceHandles.IsValidIndex( InstanceIdx ) ) {
		if( mInstanceHandles[ InstanceIdx ]->IsInstanced( ) ) {
			FTransform T = mCachedTransforms.Contains( InstanceIdx ) ? mCachedTransforms[ InstanceIdx ] : mInstanceDataCDO->GetInstanceData( )[ InstanceIdx ].RelativeTransform * GetActorTransform( );
			T.SetScale3D( !IsHidden ? T.GetScale3D( ) : FVector( 0.001f ) );
			T.SetLocation( !IsHidden ? T.GetLocation( ) : FVector( 0.001f ) );

			mInstanceHandles[ InstanceIdx ]->UpdateTransform( T );
			return true;
		}
	}

	return false;
}

bool AKPCLExtractorBase::AIO_SetInstanceWorldTransform( int32 InstanceIdx, FTransform Transform ) {
	if( !IsInGameThread( ) ) {
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, InstanceIdx, Transform ]( ) {
			AIO_SetInstanceWorldTransform( InstanceIdx, Transform );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}

	if( mInstanceHandles.IsValidIndex( InstanceIdx ) ) {
		if( mInstanceHandles[ InstanceIdx ]->IsInstanced( ) ) {
			mInstanceHandles[ InstanceIdx ]->UpdateTransform( Transform );
			mCachedTransforms.Add( InstanceIdx, Transform );
			return true;
		}
	}

	return false;
}

void AKPCLExtractorBase::Factory_Tick( float dt ) {
	Super::Factory_Tick( dt );

	if( bCustomFactoryTickPreCustomLogic ) {
		if( HasAuthority( ) ) {
			Factory_TickAuthOnly( dt );
		}
		else {
			Factory_TickClientOnly( dt );
		}
	}

	if( HasAuthority( ) ) {
		BeltPipeGrab( dt );
		HandleUiTick( dt );

		if( mProductionHandle.TickHandle( dt, IsProducing( ) ) ) {
			EndProductionTime( );
			onProducingFinal( );
		}

		if( !mPowerOptions.bWasInit ) {
			HandlePowerInit( );
		}

		if( GetPowerInfo( ) && mPowerOptions.bWasInit ) {
			HandlePower( dt );
		}
	}

	if( mCustomProductionStateIndicator || mCustomIndicatorHandleIndexes.Num( ) > 0 ) {
		if( HasAuthority( ) ) {
			HandleIndicator( );
		}
		if( GetCurrentCompProductionState( ) != GetCurrentProductionState( ) || bOneStateWasNotApplied ) {
			ApplyNewProductionState( GetCurrentProductionState( ) );
		}
	}

	if( !bCustomFactoryTickPreCustomLogic ) {
		if( HasAuthority( ) ) {
			Factory_TickAuthOnly( dt );
		}
		else {
			Factory_TickClientOnly( dt );
		}
	}
}

bool AKPCLExtractorBase::CanProduce_Implementation( ) const {
	if( IsPlayingBuildEffect( ) ) {
		return false;
	}

	return IsProductionPaused( );
}

void AKPCLExtractorBase::EndProductionTime( ) {
	if( GetCurrentPotential( ) != GetPendingPotential( ) ) {
		SetCurrentPotential( GetPendingPotential( ) );
	}
}

void AKPCLExtractorBase::BeltPipeGrab( float dt ) {}

void AKPCLExtractorBase::HandlePower( float dt ) {
	mPowerOptions.bHasPower = HasPower( );
	mPowerOptions.StructureTick( dt, IsProducing( ) );
	GetPowerInfo( )->SetTargetConsumption( mPowerOptions.mIsProducer ? FMath::Max( mPowerOptions.mForcePowerConsume, 0.1f ) : mPowerOptions.GetPowerConsume( ) );
	GetPowerInfo( )->SetMaximumTargetConsumption( mPowerOptions.mIsProducer ? FMath::Max( mPowerOptions.mForcePowerConsume, 0.1f ) : mPowerOptions.GetMaxPowerConsume( ) );
	GetPowerInfo( )->SetBaseProduction( !mPowerOptions.mIsProducer || ( !mPowerOptions.mIsProducer && mPowerOptions.mIsDynamicProducer ) ? 0.0f : mPowerOptions.GetMaxPowerConsume( ) );
	GetPowerInfo( )->SetDynamicProductionCapacity( !mPowerOptions.mIsProducer || !mPowerOptions.mIsDynamicProducer ? 0.0f : mPowerOptions.GetMaxPowerConsume( ) );
	GetPowerInfo( )->SetFullBlast( ( !mPowerOptions.mIsProducer || !mPowerOptions.mIsDynamicProducer ) );
}

void AKPCLExtractorBase::HandlePowerInit( ) {
	mPowerOptions.Init( );
}

void AKPCLExtractorBase::HandleUiTick( float dt ) {}

void AKPCLExtractorBase::InitComponents( ) {
	mCustomProductionStateIndicator = FindComponentByClass< UKPCLBetterIndicator >( );

	mConnectionMap.Add( EKPCLConnectionType::ConvIn, { } );
	mConnectionMap.Add( EKPCLConnectionType::ConvOut, { } );
	mConnectionMap.Add( EKPCLConnectionType::PipeIn, { } );
	mConnectionMap.Add( EKPCLConnectionType::PipeOut, { } );

	TArray< UFGPowerConnectionComponent* > Infos;
	GetComponents< UFGPowerConnectionComponent >( Infos );
	for( UFGPowerConnectionComponent* Info : Infos ) {
		if( UFGPowerConnectionComponent* AsPower = ExactCast< UFGPowerConnectionComponent >( Info ) ) {
			mFGPowerConnection = AsPower;
		}
	}

	TArray< UActorComponent* > Components;
	GetComponents( Components );

	for( UActorComponent* Component : Components ) {
		// Belts
		if( UFGFactoryConnectionComponent* ConveyorConnection = Cast< UFGFactoryConnectionComponent >( Component ) ) {
			if( ConveyorConnection->GetDirection( ) == EFactoryConnectionDirection::FCD_INPUT ) {
				mConnectionMap[ EKPCLConnectionType::ConvIn ].Add( ConveyorConnection );
			}
			else if( ConveyorConnection->GetDirection( ) == EFactoryConnectionDirection::FCD_OUTPUT ) {
				mConnectionMap[ EKPCLConnectionType::ConvOut ].Add( ConveyorConnection );
			}
		}

		// Pipes
		if( UFGPipeConnectionFactory* PipeConnection = Cast< UFGPipeConnectionFactory >( Component ) ) {
			if( PipeConnection->GetPipeConnectionType( ) == EPipeConnectionType::PCT_CONSUMER ) {
				mConnectionMap[ EKPCLConnectionType::PipeIn ].Add( PipeConnection );
			}
			else if( PipeConnection->GetPipeConnectionType( ) == EPipeConnectionType::PCT_PRODUCER ) {
				mConnectionMap[ EKPCLConnectionType::PipeOut ].Add( PipeConnection );
			}

			// make sure that this building use AdditionalPressure!!!
			if( !PipeConnection->mApplyAdditionalPressure ) {
				PipeConnection->mApplyAdditionalPressure = true;
			}
		}
	}
}

float AKPCLExtractorBase::GetMaxPowerConsume( ) const {
	return mPowerOptions.GetMaxPowerConsume( );
}

float AKPCLExtractorBase::GetPowerConsume( ) const {
	return mPowerOptions.GetPowerConsume( );
}

bool AKPCLExtractorBase::GetHasPower( ) const {
	return mPowerOptions.bHasPower;
}

UFGPowerConnectionComponent* AKPCLExtractorBase::GetPowerConnection( ) const {
	return mFGPowerConnection;
}

float AKPCLExtractorBase::GetCurrentPowerMultiplier( ) const {
	return mPowerOptions.mPowerMultiplier;
}

void AKPCLExtractorBase::SetPowerMultiplier( float NewMultiplier ) {
	mPowerOptions.mPowerMultiplier = NewMultiplier;
}

float AKPCLExtractorBase::GetDefaultProductionCycleTime( ) const {
	return mProductionHandle.mProductionTime;
}

float AKPCLExtractorBase::GetProductionProgress( ) const {
	return mProductionHandle.mCurrentTime / mProductionHandle.mProductionTime;
}

void AKPCLExtractorBase::SetPendingPotential( float NewPendingPotential ) {
	Super::SetPendingPotential( NewPendingPotential );
	mProductionHandle.mPendingPotential = NewPendingPotential;
}

float AKPCLExtractorBase::CalcProductionCycleTimeForPotential( float potential ) const {
	return mProductionHandle.mProductionTime / ( FMath::Max( mProductionHandle.mExtraPotential, mProductionHandle.mPendingExtraPotential ) + potential );
}

float AKPCLExtractorBase::GetProducingPowerConsumptionBase( ) const {
	return mPowerOptions.mNormalPowerConsume;
}

void AKPCLExtractorBase::Factory_CollectInput_Implementation( ) {
	Super::Factory_CollectInput_Implementation( );

	if( HasAuthority( ) ) {
		CollectBelts( );
	}
}

void AKPCLExtractorBase::Factory_PullPipeInput_Implementation( float dt ) {
	Super::Factory_PullPipeInput_Implementation( dt );

	if( HasAuthority( ) ) {
		CollectAndPushPipes( dt, false );
	}
}

void AKPCLExtractorBase::Factory_PushPipeOutput_Implementation( float dt ) {
	Super::Factory_PushPipeOutput_Implementation( dt );

	if( HasAuthority( ) ) {
		CollectAndPushPipes( dt, true );
	}
}

void AKPCLExtractorBase::OnReplicationDetailActorCreated( ) {
	Super::OnReplicationDetailActorCreated( );

	RevalidateInventoryStateForReplication( );
}

void AKPCLExtractorBase::OnReplicationDetailActorRemoved( ) {
	Super::OnReplicationDetailActorRemoved( );

	RevalidateInventoryStateForReplication( );
}

void AKPCLExtractorBase::OnBuildableReplicationDetailStateChange( bool newStateIsActive ) {
	Super::OnBuildableReplicationDetailStateChange( newStateIsActive );

	RevalidateInventoryStateForReplication( );
}

void AKPCLExtractorBase::CollectBelts( ) {}

void AKPCLExtractorBase::CollectAndPushPipes( float dt, bool IsPush ) {}

void AKPCLExtractorBase::ResetProduction( ) {
	mProductionHandle.Reset( );
}

void AKPCLExtractorBase::SetProductionTime( float NewTime, bool ShouldResetProduction ) {
	mProductionHandle.SetNewTime( NewTime, ShouldResetProduction );
}

void AKPCLExtractorBase::SetPowerOption( FPowerOptions NewPowerOption ) {
	mPowerOptions.OverWritePowerOptions( NewPowerOption );
}

FPowerOptions AKPCLExtractorBase::GetPowerOption( ) const {
	return mPowerOptions;
}

FPowerOptions& AKPCLExtractorBase::GetPowerOptionRef( ) {
	return mPowerOptions;
}

UKPCLDefaultRCO* AKPCLExtractorBase::GetDefaultKModRCO( ) const {
	AFGPlayerController* Controller = UKBFL_Player::GetFGController( GetWorld( ) );
	if( Controller ) {
		return Cast< UKPCLDefaultRCO >( Controller->GetRemoteCallObjectOfClass( GetRCOClass( ) ) );
	}
	return nullptr;
}

void AKPCLExtractorBase::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const {
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	// Production
	DOREPLIFETIME( AKPCLExtractorBase, mProductionHandle );
	DOREPLIFETIME( AKPCLExtractorBase, mPowerOptions );
	DOREPLIFETIME( AKPCLExtractorBase, mCurrentState );
	DOREPLIFETIME( AKPCLExtractorBase, mMeshOverwriteInformations );
	DOREPLIFETIME( AKPCLExtractorBase, mInventoryDatasSaved );
}

UFGInventoryComponent* AKPCLExtractorBase::GetInventoryFromIndex( int32 Idx ) const {
	if( !DoInventoryDataExsists( Idx ) ) {
		return nullptr;
	}
	return mInventoryDatasSaved[ Idx ].GetInventory( );
}

bool AKPCLExtractorBase::GetInventoryData( int32 Idx, FKPCLInventoryStructure& Out ) const {
	if( DoInventoryDataExsists( Idx ) ) {
		Out = mInventoryDatasSaved[ Idx ];
		return true;
	}
	return false;
}

bool AKPCLExtractorBase::GetStackFromInventory( int32 Idx, int32 InventoryIdx, FInventoryStack& Stack ) const {
	UFGInventoryComponent* Inv = GetInventoryFromIndex( Idx );
	if( IsValid( Inv ) ) {
		return Inv->GetStackFromIndex( InventoryIdx, Stack );
	}
	return false;
}

bool AKPCLExtractorBase::DoInventoryDataExsists( int32 Idx ) const {
	return mInventoryDatasSaved.IsValidIndex( Idx );
}

bool AKPCLExtractorBase::AreAllInventorysValid( ) const {
	for( int32 Idx = 0; Idx < mInventoryDatasSaved.Num( ); ++Idx ) {
		if( !IsValid( mInventoryDatasSaved[ Idx ].GetInventory_Detailed( ) ) ) {
			return false;
		}
	}
	return true;
}

void AKPCLExtractorBase::ResizeInventory( int32 Idx, int32 Size ) {
	if( DoInventoryDataExsists( Idx ) ) {
		mInventoryDatasSaved[ Idx ].SetInventorySize( Size );
	}
}

void AKPCLExtractorBase::OnRep_ReplicationDetailActor( ) {
	Super::OnRep_ReplicationDetailActor( );

	if( !HasAuthority( ) && mReplicationDetailActor ) {
		AKPCLReplicationActor_ExtractorBase* DetailActor = Cast< AKPCLReplicationActor_ExtractorBase >( mReplicationDetailActor );
		DetailActor->SetOwningBuildable( this );
		if( DetailActor->HasCompletedInitialReplication( ) ) {
			for( int32 Idx = 0; Idx < mInventoryDatasSaved.Num( ); ++Idx ) {
				OnReplicatedInventoryIndex( Idx );
			}
		}
		else {
			GetWorldTimerManager( ).SetTimerForNextTick( this, &AKPCLExtractorBase::OnRep_ReplicationDetailActor );
		}
	}
}

float AKPCLExtractorBase::GetProductionTime( ) const {
	return mProductionHandle.GetProductionTime( );
}

float AKPCLExtractorBase::GetPendingProductionTime( ) const {
	return mProductionHandle.GetPendingProductionTime( );
}

FFullProductionHandle AKPCLExtractorBase::GetProductionHandle( ) const {
	return mProductionHandle;
}

void AKPCLExtractorBase::FlushFluids( ) {
	if( HasAuthority( ) ) {
		Server_DoFlush( );
	}
	else {
		if( UKPCLDefaultRCO* RCO = GetDefaultKModRCO( ) ) {
			RCO->Server_FlushFluids( this );
		}
	}
}

float AKPCLExtractorBase::GetProductionCycleTime( ) const {
	return mProductionHandle.GetProductionTime( );
}

UClass* AKPCLExtractorBase::GetReplicationDetailActorClass( ) const {
	return AKPCLReplicationActor_ExtractorBase::StaticClass( );
}

UFGInventoryComponent* AKPCLExtractorBase::GetInventory( ) const {
	return GetInventoryFromIndex( 0 );
}

void AKPCLExtractorBase::ReconfigureInventory( ) {
	if( GetInventory( ) ) {
		GetInventory( )->OnItemAddedDelegate.AddUniqueDynamic( this, &AKPCLExtractorBase::OnInputItemAdded );
		GetInventory( )->OnItemRemovedDelegate.AddUniqueDynamic( this, &AKPCLExtractorBase::OnInputItemRemoved );

		if( !GetInventory( )->mItemFilter.IsBoundToObject( this ) ) {
			GetInventory( )->mItemFilter.BindUObject( this, &AKPCLExtractorBase::FilterInputInventory );
		}
		if( !GetInventory( )->mFormFilter.IsBoundToObject( this ) ) {
			GetInventory( )->mFormFilter.BindUObject( this, &AKPCLExtractorBase::FormFilterInputInventory );
		}
	}

	SetBelts( );
}
