// ILikeBanas

#include "Buildable/Modular/KPCLModularExtractorBase.h"

#include "AkAcousticTextureSetComponent.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPlayerController.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
#include "Logging.h"

AKPCLModularExtractorBase::AKPCLModularExtractorBase()
{
}

FText AKPCLModularExtractorBase::GetLookAtDecription_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state ) const
{
	return mShouldUseUiFromMaster && GetMasterBuildable() != this ? Execute_GetLookAtDecription( GetMasterBuildable(), byCharacter, state ) : Super::GetLookAtDecription_Implementation( byCharacter, state );
}

bool AKPCLModularExtractorBase::IsUseable_Implementation() const
{
	return mShouldUseUiFromMaster && GetMasterBuildable() != this ? Execute_IsUseable( GetMasterBuildable() ) : Super::IsUseable_Implementation();
}

void AKPCLModularExtractorBase::OnUseStop_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& State )
{
	if( GetMasterBuildable() != this && mShouldUseUiFromMaster )
	{
		Execute_OnUseStop( mMasterBuilding.Get(), byCharacter, State );
		return;
	}
	Super::OnUseStop_Implementation( byCharacter, State );
}

void AKPCLModularExtractorBase::OnUse_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& State )
{
	if( GetMasterBuildable() != this && mShouldUseUiFromMaster )
	{
		Execute_OnUse( mMasterBuilding.Get(), byCharacter, State );
		return;
	}
	Super::OnUse_Implementation( byCharacter, State );
}

int32 AKPCLModularExtractorBase::GetModularIndex_Implementation()
{
	return mModularIndex;
}

void AKPCLModularExtractorBase::ApplyModularIndex_Implementation( int32 Index )
{
	mModularIndex = Index;
}

void AKPCLModularExtractorBase::SetAttachedActor_Implementation( AFGBuildable* Actor )
{
	if( Actor )
	{
		mMasterBuilding = Actor;
		OnMasterBuildingReceived( Actor );
	}
}

void AKPCLModularExtractorBase::RemoveAttachedActor_Implementation( AFGBuildable* Actor )
{
	if( mModularHandler )
	{
		mModularHandler->AttachedActorRemoved( Actor );
		OnModulesWasUpdated();
		if( OnModulesUpdated.IsBound() )
		{
			OnModulesUpdated.Broadcast();
		}
	}
	else
	{
		if( mMasterBuilding.IsValid() )
		{
			Execute_RemoveAttachedActor( mMasterBuilding.Get(), Actor );
		}
	}
}

bool AKPCLModularExtractorBase::AttachedActor_Implementation( AFGBuildable* Actor, TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform Location, float Distance )
{
	if( mModularHandler )
	{
		if( mModularHandler->AddNewActorToAttachment( Actor, Attachment, Location, Distance ) )
		{
			OnModulesWasUpdated();
			if( OnModulesUpdated.IsBound() )
			{
				OnModulesUpdated.Broadcast();
			}
			return true;
		}
	}
	return false;
}

void AKPCLModularExtractorBase::OnModulesUpdated_Implementation()
{
	OnMeshUpdate();
	Event_OnMeshUpdate();
}

void AKPCLModularExtractorBase::ReadyForVisuelUpdate()
{
	Super::ReadyForVisuelUpdate();
	OnMeshUpdate();
	Event_OnMeshUpdate();
}

void AKPCLModularExtractorBase::GetChildDismantleActors_Implementation( TArray< AActor* >& out_ChildDismantleActors ) const
{
	Super::GetChildDismantleActors_Implementation( out_ChildDismantleActors );

	UKPCLModularBuildingHandlerBase* Base = GetHandler< UKPCLModularBuildingHandlerBase >();
	if( IsValid( Base ) && mDismantleAllChilds )
	{
		TArray< AFGBuildable* > Buildables;
		Base->GetAttachedActors( Buildables );
		for( AFGBuildable* Buildable : Buildables )
		{
			if( IsValid( Buildable ) )
			{
				out_ChildDismantleActors.AddUnique( Buildable );
				Execute_GetChildDismantleActors( Buildable, out_ChildDismantleActors );
			}
		}
	}
}

void AKPCLModularExtractorBase::BeginPlay()
{
	Super::BeginPlay();

	if( HasAuthority() )
	{
		HandlePowerInit();
	}
}

void AKPCLModularExtractorBase::EndPlay( const EEndPlayReason::Type EndPlayReason )
{
	if( !mModularHandler )
	{
		if( mMasterBuilding.IsValid() )
		{
			Execute_RemoveAttachedActor( mMasterBuilding.Get(), this );
		}
	}

	Super::EndPlay( EndPlayReason );
}

void AKPCLModularExtractorBase::OnModulesWasUpdated_Implementation()
{
}

void AKPCLModularExtractorBase::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	//UI
	DOREPLIFETIME( AKPCLModularExtractorBase, mMasterBuilding );
	DOREPLIFETIME( AKPCLModularExtractorBase, mModularHandler );
}

void AKPCLModularExtractorBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	mModularHandler = FindComponentByClass< UKPCLModularBuildingHandlerBase >();
}
