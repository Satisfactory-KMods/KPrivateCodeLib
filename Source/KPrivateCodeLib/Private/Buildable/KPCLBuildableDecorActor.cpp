// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Buildable/KPCLBuildableDecorActor.h"

#include "AbstractInstanceManager.h"
#include "FGCharacterPlayer.h"
#include "BFL/KBFL_Player.h"
#include "Description/Decor/KPCLDecorationActorData.h"
#include "Replication/KPCLDefaultRCO.h"


AKPCLBuildableDecorActor::AKPCLBuildableDecorActor( const FObjectInitializer& ObjectInitializer ) : Super( ObjectInitializer )
{
}

void AKPCLBuildableDecorActor::StartLookingForDecoration_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player )
{
	OnRep_DecorationData();
	
	if( IsValid( Player ) && IsValid( Player->GetOutline() ) )
	{
		Player->GetOutline()->ShowOutline( this, Execute_IsDecorationDataAllowed( this, DecorationData, Player ) ? EOutlineColor::OC_USABLE : EOutlineColor::OC_RED );
	}
}

void AKPCLBuildableDecorActor::EndLookingForDecoration_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player )
{
	mTempDecorationData = nullptr;
	
	if( IsValid( Player ) && IsValid( Player->GetOutline() ) )
	{
		Player->GetOutline()->HideOutline();
	}
	
	OnRep_DecorationData();
}

bool AKPCLBuildableDecorActor::IsDecorationDataAllowed_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player )
{
	bool HasEnoughInInventory = false;

	if( IsValid( Player ) && IsValid( Player->GetInventory() ) )
	{
		HasEnoughInInventory = true;
		for ( const FItemAmount Ingredient : UFGRecipe::GetIngredients( DecorationData ) )
		{
			if( !Player->GetInventory()->HasItems( Ingredient.ItemClass, Ingredient.Amount ) )
			{
				HasEnoughInInventory = false;
				break;
			}
		}
	}
	
	return mAllowedDecorationData.Contains( UKPCLDecorationRecipe::GetActorData( DecorationData ) ) && UKPCLDecorationRecipe::GetActorData( DecorationData ) != GetDecorationData() && HasEnoughInInventory;
}

void AKPCLBuildableDecorActor::ApplyDecorationData_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player )
{
	if( mTempDecorationData != UKPCLDecorationRecipe::GetActorData( DecorationData ) )
	{
		mTempDecorationData = UKPCLDecorationRecipe::GetActorData( DecorationData );
		OnRep_DecorationData();
	
		if( IsValid( Player ) && IsValid( Player->GetOutline() ) )
		{
			Player->GetOutline()->ShowOutline( this, Execute_IsDecorationDataAllowed( this, DecorationData, Player ) ? EOutlineColor::OC_USABLE : EOutlineColor::OC_RED );
		}
	}
}

bool AKPCLBuildableDecorActor::SetDecorationData_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player )
{
	if( mDecorationData != UKPCLDecorationRecipe::GetActorData( DecorationData ) )
	{
		mTempDecorationData = nullptr;
		SetNewActorData( DecorationData, Player );
	
		if( IsValid( Player ) && IsValid( Player->GetOutline() ) )
		{
			Player->GetOutline()->ShowOutline( this, Execute_IsDecorationDataAllowed( this, DecorationData, Player ) ? EOutlineColor::OC_USABLE : EOutlineColor::OC_RED );
		}
		
		return true;
	}
	
	if( IsValid( Player ) && IsValid( Player->GetOutline() ) )
	{
		Player->GetOutline()->ShowOutline( this, Execute_IsDecorationDataAllowed( this, DecorationData, Player ) ? EOutlineColor::OC_USABLE : EOutlineColor::OC_RED );
	}
	return false;
}

void AKPCLBuildableDecorActor::GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	
	DOREPLIFETIME(AKPCLBuildableDecorActor, mDecorationData);
	DOREPLIFETIME(AKPCLBuildableDecorActor, mTempDecorationData);
}

TSubclassOf<UKPCLDecorationActorData> AKPCLBuildableDecorActor::GetDecorationData() const
{
	if( IsValid( mTempDecorationData ) )
	{
		return mTempDecorationData;
	}
	
	if( IsValid( mDecorationData ) )
	{
		return mDecorationData;
	}
	
	if( IsValid( mDefaultDecorationData ) )
	{
		return mDefaultDecorationData;
	}
	
	return UKPCLDecorationActorData::StaticClass();
}

void AKPCLBuildableDecorActor::SetNewActorData( TSubclassOf< UKPCLDecorationRecipe > Data, AFGCharacterPlayer* Player )
{
	if( HasAuthority() )
	{
		if( Execute_IsDecorationDataAllowed( this, Data, Player ) )
		{
			for ( const FItemAmount Ingredient : UFGRecipe::GetIngredients( Data ) )
			{
				Player->GetInventory()->Remove( Ingredient.ItemClass, Ingredient.Amount );
			}
			
			mDecorationData = UKPCLDecorationRecipe::GetActorData( Data );
			OnRep_DecorationData();
		}
	}
	else if( UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get( GetWorld() ) )
	{
		RCO->Server_SetNewActorData( this, Data, Player );
	}
}

void AKPCLBuildableDecorActor::OnRep_DecorationData()
{
	if( !DoesContainLightweightInstances_Native() )
	{
		return;
	}
	
	TArray< FKPCLInstanceDataOverwrite > DecorOverwriteData = UKPCLDecorationActorData::GetInstanceData( GetDecorationData() );
	TArray<FInstanceData> DefaultInstanceData = GetLightweightInstanceData()->GetInstanceData();
	for ( FKPCLInstanceDataOverwrite OverwriteData : DecorOverwriteData )
	{
		int32 InstanceIndex = OverwriteData.InstanceIndex;
		if( DefaultInstanceData.IsValidIndex( InstanceIndex ) && mInstanceHandles.IsValidIndex( InstanceIndex ) ) {
			if( !OverwriteData.OverwriteMaterials && !OverwriteData.OverwriteMesh )
			{
				AIO_SetInstanceWorldTransform( InstanceIndex, GetActorTransform() * OverwriteData.RelativeLocation );
				ReApplyColorForIndex( InstanceIndex, mCustomizationData );
			}
			else
			{
				FInstanceData CachedInstanceData = DefaultInstanceData[ InstanceIndex ];
				TArray< FInstanceHandle* > Handles = { mInstanceHandles[ InstanceIndex ] };
				AAbstractInstanceManager::RemoveInstances( GetWorld(), Handles, false );
				mInstanceHandles[ InstanceIndex ] = new FInstanceHandle();

				if( OverwriteData.OverwriteLocation )
				{
					CachedInstanceData.RelativeTransform = OverwriteData.RelativeLocation;
				}

				if( OverwriteData.OverwriteMesh && IsValid( OverwriteData.Mesh ) )
				{
					CachedInstanceData.StaticMesh = OverwriteData.Mesh;
				}

				if( OverwriteData.OverwriteMaterials && IsValid( CachedInstanceData.StaticMesh ) && ( OverwriteData.MaterialOverwriteMap.Num() > 0 || OverwriteData.MaterialIndexOverwriteMap.Num() > 0 ) )
				{
					TArray< FStaticMaterial > DefaultMaterials = CachedInstanceData.StaticMesh->StaticMaterials;
					for( int32 Idx = 0; Idx < DefaultMaterials.Num(); ++Idx )
					{
						UMaterialInterface* Material = DefaultMaterials[ Idx ].MaterialInterface;
						if( OverwriteData.MaterialIndexOverwriteMap.Contains( Idx ) )
						{
							Material = OverwriteData.MaterialIndexOverwriteMap[ Idx ];
						}
						
						if( OverwriteData.MaterialOverwriteMap.Contains( Material ) )
						{
							Material = OverwriteData.MaterialOverwriteMap[ Material ];
						}
						
						if( !CachedInstanceData.OverridenMaterials.IsValidIndex( Idx ) )
						{
							CachedInstanceData.OverridenMaterials.Add( Material );
							continue;
						}
						CachedInstanceData.OverridenMaterials[ Idx ] = Material;
					}
				}
				
				AAbstractInstanceManager::SetInstanceFromDataStatic( this, GetActorTransform(), CachedInstanceData, mInstanceHandles[ InstanceIndex ] );
				ReApplyColorForIndex( InstanceIndex, mCustomizationData );
			}
		}
	}
}

AKPCLBuildableDecorActorLight::AKPCLBuildableDecorActorLight( const FObjectInitializer& ObjectInitializer ) : Super( ObjectInitializer.DoNotCreateDefaultSubobject( TEXT("BuildingMeshProxy") ) )
{
}

// Called when the game starts or when spawned
void AKPCLBuildableDecorActor::BeginPlay()
{
	Super::BeginPlay();

	if( !bDeferBeginPlay )
	{
		OnRep_DecorationData();
	}
}


void AKPCLBuildableDecorActor::ReApplyColorForIndex( int32 Idx, const FFactoryCustomizationData& customizationData )
{
	if( !mInstanceHandles.IsValidIndex( Idx ) || !DoesContainLightweightInstances_Native() )
	{
		return;
	}
	
	if( mInstanceHandles[ Idx ]->IsInstanced() && DoesContainLightweightInstances_Native() )
	{
		TArray< float > Datas = customizationData.Data;
		Datas.SetNum( mInstanceDataCDO->GetInstanceData()[ Idx ].NumCustomDataFloats );
		if( mCachedCustomData.Contains( Idx ) )
		{
			for ( TTuple<int, float> Result : mCachedCustomData[ Idx ] )
			{
				Datas[ Result.Key ] = Result.Value;
			}
		}
		AAbstractInstanceManager::SetCustomPrimitiveDataOnHandle( mInstanceHandles[ Idx ], Datas, true );
	}
}

void AKPCLBuildableDecorActor::ApplyCustomizationData_Native( const FFactoryCustomizationData& customizationData )
{
	Super::ApplyCustomizationData_Native( customizationData );

	for( int32 Idx = 0; Idx < mInstanceHandles.Num(); ++Idx )
	{
		ReApplyColorForIndex( Idx, customizationData );
	}
}

void AKPCLBuildableDecorActor::SetCustomizationData_Native( const FFactoryCustomizationData& customizationData )
{
	Super::SetCustomizationData_Native( customizationData );

	if( DoesContainLightweightInstances_Native() )
	{
		for( int32 Idx = 0; Idx < mInstanceHandles.Num(); ++Idx )
		{
			ReApplyColorForIndex( Idx, customizationData );
		}
	}
}

void AKPCLBuildableDecorActor::OnBuildEffectFinished()
{
	Super::OnBuildEffectFinished();

	OnRep_DecorationData();
}

void AKPCLBuildableDecorActor::OnBuildEffectActorFinished()
{
	Super::OnBuildEffectActorFinished();

	OnRep_DecorationData();
}

bool AKPCLBuildableDecorActor::AIO_OverwriteInstanceData( UStaticMesh* Mesh, int32 Idx )
{
	if( !IsInGameThread() )
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, Mesh, Idx ]()
		{
			AIO_OverwriteInstanceData( Mesh, Idx );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}
	
	if( Idx > INDEX_NONE && IsValid( Mesh ) )
	{
		TArray< FInstanceData > Datas = mInstanceDataCDO->GetInstanceData();
		if( Datas.IsValidIndex( Idx ) )
		{
			return AIO_OverwriteInstanceData_Transform( Mesh, Datas[Idx].RelativeTransform, Idx );
		}
	}
	return false;
}

bool AKPCLBuildableDecorActor::AIO_OverwriteInstanceData_Transform( UStaticMesh* Mesh, FTransform NewRelativTransform, int32 Idx )
{
	if( !IsInGameThread() )
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, Mesh, NewRelativTransform, Idx ]()
		{
			AIO_OverwriteInstanceData_Transform( Mesh, NewRelativTransform, Idx );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}
	
	if( Idx > INDEX_NONE && IsValid( Mesh ) )
	{
		AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager( GetWorld() );
		TArray< FInstanceData > Datas = mInstanceDataCDO->GetInstanceData();
		if( Datas.IsValidIndex( Idx ) && mInstanceHandles.IsValidIndex( Idx ) && IsValid( Manager ) )
		{
			FInstanceData Data = Datas[ Idx ];
			Data.RelativeTransform = NewRelativTransform;
			Data.StaticMesh = Mesh;

			if( mInstanceHandles[ Idx ]->IsInstanced() )
			{
				Manager->RemoveInstance( mInstanceHandles[ Idx ] );
			}
			mInstanceHandles[ Idx ] = new FInstanceHandle();
			Manager->SetInstanced( this, GetActorTransform(), Data, mInstanceHandles[ Idx ] );
			mCachedTransforms.Add( Idx, NewRelativTransform * GetActorTransform() );
			
			ApplyCustomizationData_Native( mCustomizationData );
			SetCustomizationData_Native( mCustomizationData );
			
			return true;
		}
	}
	
	return false;
}

bool AKPCLBuildableDecorActor::AIO_UpdateCustomFloat( int32 FloatIndex, float Data, int32 InstanceIdx, bool MarkDirty )
{
	if( !IsInGameThread() )
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, FloatIndex, Data, InstanceIdx, MarkDirty ]()
		{
			AIO_UpdateCustomFloat( FloatIndex, Data, InstanceIdx, MarkDirty );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}
	
	if( mInstanceHandles.IsValidIndex( InstanceIdx ) )
	{
		if( mInstanceHandles[ InstanceIdx ]->IsInstanced() )
		{
			//UE_LOG( LogKPCL, Error, TEXT("mInstanceHandles[ %d ]->SetPrimitiveDataByID( %f, %d, %d ); IsValid(%d), Component(%d), Owner(%d)"), InstanceIdx, Data, FloatIndex, MarkDirty, mInstanceHandles[ InstanceIdx ]->IsValid(), IsValid( mInstanceHandles[ InstanceIdx ]->GetInstanceComponent() ), mInstanceHandles[ InstanceIdx ]->GetOwner() == this )
			mInstanceHandles[ InstanceIdx ]->SetPrimitiveDataByID( Data/** float that we want to set */, FloatIndex /** Index where we want to set */, true );
			if( mCachedCustomData.Contains( InstanceIdx ) )
			{
				mCachedCustomData[ InstanceIdx ].Add( FloatIndex, Data );
			}
			else
			{
				TMap< int32, float > Map;
				Map.Add( FloatIndex, Data );
				mCachedCustomData.Add( InstanceIdx, Map );
			}
			return true;
		}
	}

	return false;
}

bool AKPCLBuildableDecorActor::AIO_UpdateCustomFloatAsColor( int32 StartFloatIndex, FLinearColor Data, int32 InstanceIdx, bool MarkDirty )
{
	if( !IsInGameThread() )
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, StartFloatIndex, Data, InstanceIdx, MarkDirty ]()
		{
			AIO_UpdateCustomFloatAsColor( StartFloatIndex, Data, InstanceIdx, MarkDirty );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}
	
	AIO_UpdateCustomFloat( StartFloatIndex, Data.R, InstanceIdx, MarkDirty );
	AIO_UpdateCustomFloat( StartFloatIndex + 1, Data.G, InstanceIdx, MarkDirty );
	return AIO_UpdateCustomFloat( StartFloatIndex + 2, Data.B, InstanceIdx, MarkDirty );
}

bool AKPCLBuildableDecorActor::AIO_SetInstanceHidden( int32 InstanceIdx, bool IsHidden )
{
	if( !IsInGameThread() )
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, InstanceIdx, IsHidden ]()
		{
			AIO_SetInstanceHidden( InstanceIdx, IsHidden );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}
	
	if( mInstanceHandles.IsValidIndex( InstanceIdx ) )
	{
		if( mInstanceHandles[ InstanceIdx ]->IsInstanced() )
		{
			FTransform T = mCachedTransforms.Contains( InstanceIdx ) ? mCachedTransforms[ InstanceIdx ] : mInstanceDataCDO->GetInstanceData()[ InstanceIdx ].RelativeTransform * GetActorTransform();
			T.SetScale3D( !IsHidden ? T.GetScale3D() : FVector( 0.001f ) );
			T.SetLocation( !IsHidden ? T.GetLocation() : FVector( 0.001f ) );
			
			mInstanceHandles[ InstanceIdx ]->UpdateTransform( T );
			return true;
		}
	}

	return false;
}

bool AKPCLBuildableDecorActor::AIO_SetInstanceWorldTransform( int32 InstanceIdx, FTransform Transform )
{
	if( !IsInGameThread() )
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady( [ &, InstanceIdx, Transform ]()
		{
			AIO_SetInstanceWorldTransform( InstanceIdx, Transform );
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
		return true;
	}
	
	if( mInstanceHandles.IsValidIndex( InstanceIdx ) )
	{
		if( mInstanceHandles[ InstanceIdx ]->IsInstanced() )
		{
			mInstanceHandles[ InstanceIdx ]->UpdateTransform( Transform );
			mCachedTransforms.Add( InstanceIdx, Transform );
			return true;
		}
	}

	return false;
}

