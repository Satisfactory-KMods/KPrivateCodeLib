// Fill out your copyright notice in the Description page of Project Settings.


#include "KPCLWorldModule.h"

#include "FGGameState.h"
#include "Description/KPCLRegistryObject.h"
#include "Engine/AssetManager.h"
#include "Registry/ModContentRegistry.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN( KBFLWorldModuleV2Log, Log, All );

DEFINE_LOG_CATEGORY( KBFLWorldModuleV2Log );

UKPCLWorldModule::UKPCLWorldModule( ) : Super( ) {}

void UKPCLWorldModule::PostInitPhase_Implementation( ) {
	Super::PostInitPhase_Implementation( );
	BindUnlockEvents( );
}

void UKPCLWorldModule::InitPhase_Implementation( ) {
	Super::InitPhase_Implementation( );

	if( IsEasyNodesEnabled( ) ) {
		ApplyEasyNodes( mResearchTrees );
	}

	UKBFLAssetDataSubsystem* Subsystem = UKBFLAssetDataSubsystem::Get( GetWorld( ) );
	UModContentRegistry* ModContentRegistry = UModContentRegistry::Get( GetWorld( ) );

	if( IsValid( Subsystem ) && IsValid( ModContentRegistry ) ) {
		//Register default content
		TArray< TSubclassOf< UKPCLRegistryObject > > Objects;
		Subsystem->GetObjectsOfChilds_Internal( { UKPCLRegistryObject::StaticClass( ) }, Objects );

		for( TSubclassOf< UKPCLRegistryObject > RegistryObjectClass : Objects ) {
			if( UKPCLRegistryObject::ShouldRegister( RegistryObjectClass ) ) {
				UKPCLRegistryObject* RegistryObject = RegistryObjectClass.GetDefaultObject( );

				//Register Schematics
				for( TSubclassOf< UFGSchematic > Schematic : RegistryObject->GetSchematicsToRegister( ) ) {
					if( !IsValid( Schematic ) ) {
						continue;
					}

					ModContentRegistry->RegisterSchematic( GetOwnerModReference( ), Schematic );
				}

				//Register Recipes
				for( TSubclassOf< UFGRecipe > Recipe : RegistryObject->GetRecipesToRegister( ) ) {
					if( !IsValid( Recipe ) ) {
						continue;
					}

					ModContentRegistry->RegisterRecipe( GetOwnerModReference( ), Recipe );
				}
			}
		}
	}
}

void UKPCLWorldModule::BindUnlockEvents( ) {
	if( AFGResearchManager* ResearchManager = AFGResearchManager::Get( GetWorld( ) ) ) {
		ResearchManager->ResearchResultsClaimedDelegate.AddUniqueDynamic( this, &UKPCLWorldModule::OnSchematicUnlocked );
		ResearchManager->ResearchTreeUnlockedDelegate.AddUniqueDynamic( this, &UKPCLWorldModule::OnResearchTreeAccessUnlocked );
	}

	if( AFGSchematicManager* SchematicManager = AFGSchematicManager::Get( GetWorld( ) ) ) {
		SchematicManager->PurchasedSchematicDelegate.AddUniqueDynamic( this, &UKPCLWorldModule::OnSchematicUnlocked );
	}
}

void UKPCLWorldModule::OnSchematicUnlocked( TSubclassOf< UFGSchematic > Schematic ) {
	const TSubclassOf< UFGMessageBase >* Message = mUnlockSchematic.Find( Schematic );
	if( Message && IsADAEnabled( ) ) {
		SendMessage( *Message );
	}
}

void UKPCLWorldModule::OnResearchTreeAccessUnlocked( TSubclassOf< UFGResearchTree > ResearchTree ) {
	const TSubclassOf< UFGMessageBase >* Message = mUnlockAccessResearchTree.Find( ResearchTree );
	if( Message && IsADAEnabled( ) ) {
		SendMessage( *Message );
	}
}

bool UKPCLWorldModule::IsEasyNodesEnabled_Implementation( ) {
	return mUseEasyNodes;
}

void UKPCLWorldModule::ApplyEasyNodes_Implementation( const TArray< TSubclassOf< UFGResearchTree > >& Nodes ) {}

bool UKPCLWorldModule::IsADAEnabled_Implementation( ) {
	return true;
}

void UKPCLWorldModule::SendMessage( const TSubclassOf< UFGMessageBase > Message ) const {
	AFGGameState* GameState = Cast< AFGGameState >( UGameplayStatics::GetGameState( GetWorld( ) ) );
	if( GameState && Message ) {
		GameState->SendMessageToAllPlayers( Message );
	}
}
