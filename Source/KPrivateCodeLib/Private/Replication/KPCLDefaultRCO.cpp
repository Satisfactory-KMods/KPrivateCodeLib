// Fill out your copyright notice in the Description page of Project Settings.
#include "Replication/KPCLDefaultRCO.h"

#include "Buildable/KPCLBuildableDecorActor.h"
#include "Buildable/KPCLExtractorBase.h"
#include "Buildable/KPCLProducerBase.h"
#include "Looting/KPCLLootChest.h"

#include "Net/UnrealNetwork.h"

#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/Buildings/KPCLNetworkTeleporter.h"
#include "Subsystem/KPCLUnlockSubsystem.h"


// Start Outline


void UKPCLDefaultRCO::Server_CreateOutlineForActor_Implementation( AKPCLOutlineSubsystem* Subsystem, FOutlineData Data ) {
	if( Subsystem && Data.mActorToOutline ) {
		Subsystem->MultiCast_CreateOutlineForActor( Data );
		Subsystem->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_ClearOutlines_Implementation( AKPCLOutlineSubsystem* Subsystem ) {
	if( Subsystem ) {
		Subsystem->MultiCast_ClearOutlines( );
		Subsystem->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_SetOutlineColor_Implementation( AKPCLOutlineSubsystem* Subsystem, FLinearColor Color, EOutlineColorSlot ColorSlot ) {
	if( Subsystem ) {
		Subsystem->MultiCast_SetOutlineColor( Color, ColorSlot );
		Subsystem->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_ClearOutlineForActor_Implementation( AKPCLOutlineSubsystem* Subsystem, AActor* Actor ) {
	if( Subsystem && Actor ) {
		Subsystem->MultiCast_ClearOutlinesForActor( Actor );
		Subsystem->ForceNetUpdate( );
	}
}


// End Outline

void UKPCLDefaultRCO::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const {
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UKPCLDefaultRCO, mDummy );
}

void UKPCLDefaultRCO::Server_FlushFluids_Implementation( AFGBuildable* Building ) {
	if( AKPCLProducerBase* Producer = Cast< AKPCLProducerBase >( Building ) ) {
		Producer->FlushFluids( );
		Producer->ForceNetUpdate( );
		return;
	}

	if( AKPCLExtractorBase* Extractor = Cast< AKPCLExtractorBase >( Building ) ) {
		Extractor->FlushFluids( );
		Extractor->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_SetSinkOverflowItem_Implementation( AKPCLNetworkBuildingBase* Building, bool NewAllowed ) {
	AKPCLNetworkConnectionBuilding* ConnectionBuilding = Cast< AKPCLNetworkConnectionBuilding >( Building );
	if( ensure( ConnectionBuilding ) ) {
		ConnectionBuilding->SetIsAllowedToSinkOverflow( NewAllowed );
		ConnectionBuilding->ForceNetUpdate( );
		return;
	}

	AKPCLNetworkBuildingAttachment* ManufacturerConnection = Cast< AKPCLNetworkBuildingAttachment >( Building );
	if( ensure( ManufacturerConnection ) ) {
		//ManufacturerConnection->SetIsAllowedToSinkOverflow( NewAllowed );
		ManufacturerConnection->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_Core_LootChest_Implementation( AKPCLLootChest* Target, AFGCharacterPlayer* Player ) {
	if( ensure( Target ) ) {
		Target->Loot( Player );
	}
}

int32 UKPCLDefaultRCO::MoveItemAmount( UFGInventoryComponent* Source, int32 SourceIndex, UFGInventoryComponent* Target, FItemAmount Amount, bool ResizeToFit ) {
	if( ensure( Source && Target && Amount.ItemClass && Amount.Amount > 0 ) ) {
		FInventoryStack Stack;
		if( Source->GetStackFromIndex( SourceIndex, Stack ) ) {
			if( Stack.NumItems >= Amount.Amount || ResizeToFit ) {
				if( ResizeToFit && Stack.NumItems < Amount.Amount ) {
					Amount.Amount = Stack.NumItems;
				}

				int32 GrabedAmount = Target->AddStack( FInventoryStack( Amount.Amount, Amount.ItemClass ) );

				if( GrabedAmount > 0 ) {
					Source->RemoveFromIndex( SourceIndex, GrabedAmount );
				}

				return GrabedAmount;
			}
		}
	}
	return 0;
}

void UKPCLDefaultRCO::Server_SetNewActorData_Implementation( AKPCLBuildableDecorActor* Target, TSubclassOf< UKPCLDecorationRecipe > Data, AFGCharacterPlayer* Player ) {
	if( IsValid( Target ) ) {
		Target->SetNewActorData( Data, Player );
	}
}

void UKPCLDefaultRCO::Server_UnlockEndlessShopItems_Implementation( AKPCLUnlockSubsystem* Target, AFGCharacterPlayer* Player, const TArray< TSubclassOf< UKPCLEndlessShopItem > >& UnlockingItems ) {
	if( IsValid( Target ) ) {
		Target->UnlockEndlessShopItems( Player, UnlockingItems );
	}
}

void UKPCLDefaultRCO::Server_SetTeleporterData_Implementation( AKPCLNetworkTeleporter* Target, FTeleporterInformation NewData ) {
	if( IsValid( Target ) ) {
		Target->SetTeleporterData( NewData );
		Target->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_TeleportPlayer_Implementation( AKPCLNetworkTeleporter* Target, AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter ) {
	if( IsValid( Target ) ) {
		Target->TeleportPlayer( Character, OtherTeleporter );
	}
}

void UKPCLDefaultRCO::Server_EditRuleFromNetworkComponent_Implementation( UKPCLNetworkPlayerComponent* Target, int32 RuleIndex, FKPCLPlayerInventoryRules Rule ) {
	if( IsValid( Target ) ) {
		Target->EditRule( RuleIndex, Rule );
		Target->GetOwner( )->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_AddRuleFromNetworkComponent_Implementation( UKPCLNetworkPlayerComponent* Target, FKPCLPlayerInventoryRules Rule ) {
	if( IsValid( Target ) ) {
		Target->AddRule( Rule );
		Target->GetOwner( )->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_RemoveRuleFromNetworkComponent_Implementation( UKPCLNetworkPlayerComponent* Target, int32 RuleIndex ) {
	if( IsValid( Target ) ) {
		Target->RemoveRule( RuleIndex );
		Target->GetOwner( )->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_MoveItemAmount_Implementation( UFGInventoryComponent* Source, int32 SourceIndex, UFGInventoryComponent* Target, FItemAmount Amount, bool ResizeToFit ) {
	MoveItemAmount( Source, SourceIndex, Target, Amount, ResizeToFit );
}

void UKPCLDefaultRCO::Server_Core_SetMaxItemCount_Implementation( AKPCLNetworkCore* Target, TSubclassOf< UFGItemDescriptor > Item, int32 Max ) {
	if( ensure( Target ) ) {
		Target->Core_SetMaxItemCount( Item, Max );
		Target->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_UpdateCustomSwatchData_Implementation( AKPCLSwatchSystem* Target, FCustomSwatchData Data, int32 Idx ) {
	if( ensureMsgf( Target, TEXT("Cant Found AKPCLSwatchSystem Server_AddCustomSwatchData") ) ) {
		Target->UpdateCustomSwatchData( Data, Idx );
		Target->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_AddCustomSwatchData_Implementation( AKPCLSwatchSystem* Target, FCustomSwatchData Data ) {
	if( ensureMsgf( Target, TEXT("Cant Found AKPCLSwatchSystem Server_AddCustomSwatchData") ) ) {
		Target->AddCustomSwatchData( Data );
		Target->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_RemoveCustomSwatchData_Implementation( AKPCLSwatchSystem* Target, int32 Idx ) {
	if( ensureMsgf( Target, TEXT("Cant Found AKPCLSwatchSystem Server_RemoveCustomSwatchData") ) ) {
		Target->RemoveCustomSwatchData( Idx );
		Target->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_SetGrabItem_Implementation( AKPCLNetworkConnectionBuilding* Building, TSubclassOf< UFGItemDescriptor > Item ) {
	if( ensure( Building ) ) {
		Building->SetGrabItem( Item );
		Building->ForceNetUpdate( );
	}
}

void UKPCLDefaultRCO::Server_SetManuellItemMax_Implementation( AKPCLNetworkConnectionBuilding* Building, int32 Value ) {
	if( ensure( Building ) ) {
		Building->SetManuellItemMax( Value );
		Building->ForceNetUpdate( );
	}
}
