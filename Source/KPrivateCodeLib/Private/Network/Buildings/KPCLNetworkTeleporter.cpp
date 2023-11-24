// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkTeleporter.h"

#include "KPrivateCodeLibModule.h"
#include "BFL/KBFL_Player.h"

#include "Net/UnrealNetwork.h"

#include "Network/KPCLNetwork.h"
#include "Network/Buildings/KPCLNetworkCore.h"

AKPCLNetworkTeleporter::AKPCLNetworkTeleporter( ) {
	PrimaryActorTick.bCanEverTick = 1;
	PrimaryActorTick.TickInterval = .1f;
	mInventoryDatas.Empty( );

	mPlayerSphere = CreateDefaultSubobject< USphereComponent >( TEXT( "PlayerSphere" ) );
	mTriggerBox = CreateDefaultSubobject< UBoxComponent >( TEXT( "TriggerBox" ) );

	mPlayerSphere->SetupAttachment( GetRootComponent( ) );
	mTriggerBox->SetupAttachment( GetRootComponent( ) );
}

void AKPCLNetworkTeleporter::OnUse_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state ) {
	if( !IsValid( byCharacter ) ) {
		Super::OnUse_Implementation( byCharacter, state );
	}

	if( mPlayerSphere->IsOverlappingActor( byCharacter ) ) {
		Super::OnUse_Implementation( byCharacter, state );
	}
}

FText AKPCLNetworkTeleporter::GetLookAtDecription_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state ) const {
	if( !IsValid( byCharacter ) ) {
		return Super::GetLookAtDecription_Implementation( byCharacter, state );
	}

	if( !mPlayerSphere->IsOverlappingActor( byCharacter ) ) {
		return mToFarAwayText;
	}

	return Super::GetLookAtDecription_Implementation( byCharacter, state );
}

void AKPCLNetworkTeleporter::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const {
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AKPCLNetworkTeleporter, mTeleporterData );
}

void AKPCLNetworkTeleporter::TeleportPlayer( AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter ) {
	if( HasAuthority( ) ) {
		if( CanTeleportToTeleporter( Character, OtherTeleporter ) ) {
			TArray< FItemAmount > OutCosts;
			GetCostToOtherTeleporter( OtherTeleporter, OutCosts );
			if( HasCore_Internal( ) ) {
				if( GetCore_Implementation( )->RemoveFromCore( OutCosts ) ) {
					Multicast_TeleportPlayer( Character, OtherTeleporter );
				}
			}
		}
	}
	else if( UKPCLDefaultRCO* RCO = GetRCO< UKPCLDefaultRCO >( ) ) {
		RCO->Server_TeleportPlayer( this, Character, OtherTeleporter );
	}
}

void AKPCLNetworkTeleporter::Multicast_TeleportPlayer_Implementation( AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter ) {
	Character->SetActorLocation( OtherTeleporter->GetTeleportLocation( ) );
	if( mOnPlayerTeleported.IsBound( ) ) {
		mOnPlayerTeleported.Broadcast( Character );
	}
}

void AKPCLNetworkTeleporter::SetTeleporterData( FTeleporterInformation NewData ) {
	if( HasAuthority( ) ) {
		mTeleporterData = NewData;
		OnRep_TeleporterName( );
	}
	else if( UKPCLDefaultRCO* RCO = GetRCO< UKPCLDefaultRCO >( ) ) {
		RCO->Server_SetTeleporterData( this, NewData );
	}
}

FTeleporterInformation AKPCLNetworkTeleporter::GetTeleporterData( ) {
	return mTeleporterData;
}

bool AKPCLNetworkTeleporter::CanTeleportToTeleporter( AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter ) {
	if( !IsValid( OtherTeleporter ) || !IsValid( Character ) || !IsProducing( ) ) {
		return false;
	}

	TArray< AKPCLNetworkTeleporter* > OtherTeleporters;
	GetAllOtherTeleporter( OtherTeleporters );
	if( OtherTeleporters.Contains( OtherTeleporter ) && Character->GetInventory( ) ) {
		AKPCLNetworkCore* Core = Execute_GetCore( this );
		if( IsValid( Core ) ) {
			TArray< FItemAmount > OutCosts;
			GetCostToOtherTeleporter( OtherTeleporter, OutCosts );
			return Core->CanRemoveFromCore( OutCosts );
		}
	}

	return false;
}

FVector AKPCLNetworkTeleporter::GetTeleportLocation( ) const {
	return mPlayerSphere->GetComponentLocation( );
}

void AKPCLNetworkTeleporter::OnRep_TeleporterName( ) {
	if( mOnTeleporterDataUpdated.IsBound( ) ) {
		mOnTeleporterDataUpdated.Broadcast( mTeleporterData );
	}
}

void AKPCLNetworkTeleporter::GetCostToOtherTeleporter( AKPCLNetworkTeleporter* OtherTeleporter, TArray< FItemAmount >& OutCosts ) {
	if( !IsValid( OtherTeleporter ) ) {
		return;
	}

	OutCosts = mCosts;

	const float Multiplier = FVector::Distance( OtherTeleporter->GetActorLocation( ), GetActorLocation( ) ) / mMultiplierPerRange;
	for( FItemAmount& OutCost : OutCosts ) {
		OutCost.Amount = FMath::Max( FMath::TruncToInt( OutCost.Amount * Multiplier ), OutCost.Amount );
	}
}

void AKPCLNetworkTeleporter::GetAllOtherTeleporter( TArray< AKPCLNetworkTeleporter* >& OtherTeleporter ) {
	OtherTeleporter.Empty( );

	UKPCLNetwork* Network = Execute_GetNetwork( this );
	if( IsValid( Network ) ) {
		Network->GetAllTeleporter( OtherTeleporter );
	}

	OtherTeleporter.Remove( this );
}
