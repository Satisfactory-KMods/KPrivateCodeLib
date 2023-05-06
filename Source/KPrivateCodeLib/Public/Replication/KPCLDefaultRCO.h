// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "Components/KPCLNetworkPlayerComponent.h"
#include "OutlineSystem/KPCLOutlineSubsystem.h"
#include "Structures/KPCLLevelingStruc.h"
#include "Structures/KPCLNetworkStructures.h"
#include "Subsystem/KPCLLevelingSubsystem.h"
#include "Subsystem/KPCLSwatchSystem.h"
#include "KPCLDefaultRCO.generated.h"

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLDefaultRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION( BlueprintCallable, Category = "Networking", meta = (WorldContext = "WorldContext") )
	static UKPCLDefaultRCO* GetKPCLDefaultRCO( UObject* WorldContext ) { return Get( WorldContext ); }

	static UKPCLDefaultRCO* Get( UObject* WorldContext )
	{
		if( WorldContext )
		{
			if( AFGPlayerController* Controller = Cast< AFGPlayerController >( WorldContext->GetWorld()->GetFirstPlayerController() ) )
			{
				if( UKPCLDefaultRCO* RCO = Controller->GetRemoteCallObjectOfClass< UKPCLDefaultRCO >() )
				{
					return RCO;
				}
			}
		}
		return nullptr;
	};

	// Start Outline

	UFUNCTION( Server, BlueprintCallable, WithValidation, Reliable )
	void Server_CreateOutlineForActor( AKPCLOutlineSubsystem* Subsystem, FOutlineData Data );
	FORCEINLINE bool Server_CreateOutlineForActor_Validate( AKPCLOutlineSubsystem* Subsystem, FOutlineData Data )
	{
		return true;
	}

	UFUNCTION( Server, BlueprintCallable, WithValidation, Reliable )
	void Server_ClearOutlines( AKPCLOutlineSubsystem* Subsystem );
	FORCEINLINE bool Server_ClearOutlines_Validate( AKPCLOutlineSubsystem* Subsystem ) { return true; }

	UFUNCTION( Server, BlueprintCallable, WithValidation, Reliable )
	void Server_SetOutlineColor( AKPCLOutlineSubsystem* Subsystem, FLinearColor Color, EOutlineColorSlot ColorSlot );
	FORCEINLINE bool Server_SetOutlineColor_Validate( AKPCLOutlineSubsystem* Subsystem, FLinearColor Color, EOutlineColorSlot ColorSlot ) { return true; }

	UFUNCTION( Server, BlueprintCallable, WithValidation, Reliable )
	void Server_ClearOutlineForActor( AKPCLOutlineSubsystem* Subsystem, AActor* Actor );
	FORCEINLINE bool Server_ClearOutlineForActor_Validate( AKPCLOutlineSubsystem* Subsystem, AActor* Actor )
	{
		return true;
	}

	// End Outline

	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_DoSkill_Default( AKPCLLevelingSubsystem* Subsystem, ESkillingDefaults Skilling, UKPCLLevelingSkillInformation* SkillInformation, AFGCharacterPlayer* Char );

	bool Server_DoSkill_Default_Validate( AKPCLLevelingSubsystem* Subsystem, ESkillingDefaults Skilling, UKPCLLevelingSkillInformation* SkillInformation, AFGCharacterPlayer* Char )
	{
		return true;
	}

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_DoSkill_Once( AKPCLLevelingSubsystem* Subsystem, EOnceSkilled Skilling, UKPCLLevelingSkillInformation* SkillInformation, AFGCharacterPlayer* Char );

	bool Server_DoSkill_Once_Validate( AKPCLLevelingSubsystem* Subsystem, EOnceSkilled Skilling, UKPCLLevelingSkillInformation* SkillInformation, AFGCharacterPlayer* Char )
	{
		return true;
	}

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_LEVELUP( AKPCLLevelingSubsystem* Subsystem, AFGPlayerState* PlayerState, FKPCLLevelStepInformation LevelInformation );

	bool Server_LEVELUP_Validate( AKPCLLevelingSubsystem* Subsystem, AFGPlayerState* PlayerState, FKPCLLevelStepInformation LevelInformation ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_AddSkillExp( AKPCLLevelingSubsystem* Subsystem, int Points, AFGPlayerState* PlayerState, EExpType Type );

	bool Server_AddSkillExp_Validate( AKPCLLevelingSubsystem* Subsystem, int Points, AFGPlayerState* PlayerState, EExpType Type ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_AddPending( AKPCLLevelingSubsystem* Subsystem, AFGPlayerState* PlayerState );
	bool Server_AddPending_Validate( AKPCLLevelingSubsystem* Subsystem, AFGPlayerState* PlayerState ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_LocalExpGained( AKPCLLevelingSubsystem* Subsystem, float Exp, EExpType Type, AFGPlayerState* PlayerState );

	bool Server_LocalExpGained_Validate( AKPCLLevelingSubsystem* Subsystem, float Exp, EExpType Type, AFGPlayerState* PlayerState ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_GlobalExpGained( AKPCLLevelingSubsystem* Subsystem, float Exp, EExpType Type );
	bool Server_GlobalExpGained_Validate( AKPCLLevelingSubsystem* Subsystem, float Exp, EExpType Type ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_DoReskill( AKPCLLevelingSubsystem* Subsystem, UKPCLLevelingSkillInformation* SkillInformation, ESkillingDefaults Skilling, AFGCharacterPlayer* Char );
	bool Server_DoReskill_Validate( AKPCLLevelingSubsystem* Subsystem, UKPCLLevelingSkillInformation* SkillInformation, ESkillingDefaults Skilling, AFGCharacterPlayer* Char ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_DoCheat( AKPCLLevelingSubsystem* Subsystem, const FString& PlayerName, FKPCLCheating Cheat );
	bool Server_DoCheat_Validate( AKPCLLevelingSubsystem* Subsystem, const FString& PlayerName, FKPCLCheating Cheat ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_DoOnceReskill( AKPCLLevelingSubsystem* Subsystem, UKPCLLevelingSkillInformation* SkillInformation, EOnceSkilled Skilling, AFGCharacterPlayer* Char );
	bool Server_DoOnceReskill_Validate( AKPCLLevelingSubsystem* Subsystem, UKPCLLevelingSkillInformation* SkillInformation, EOnceSkilled Skilling, AFGCharacterPlayer* Char ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_FlushFluids( AFGBuildable* Building );
	bool Server_FlushFluids_Validate( AFGBuildable* Building ) { return true; }
	virtual void Server_FlushFluids_Implementation( AFGBuildable* Building );

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_SetManuellItemMax( class AKPCLNetworkConnectionBuilding* Building, int32 Value );
	bool Server_SetManuellItemMax_Validate( AKPCLNetworkConnectionBuilding* Building, int32 Value ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_SetGrabItem( AKPCLNetworkConnectionBuilding* Building, TSubclassOf< UFGItemDescriptor > Item );
	bool Server_SetGrabItem_Validate( AKPCLNetworkConnectionBuilding* Building, TSubclassOf< UFGItemDescriptor > Item ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_SetSinkOverflowItem( class AKPCLNetworkBuildingBase* Building, bool NewAllowed );
	bool Server_SetSinkOverflowItem_Validate( AKPCLNetworkBuildingBase* Building, bool NewAllowed ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_RemoveCustomSwatchData( AKPCLSwatchSystem* Target, int32 Idx );
	bool Server_RemoveCustomSwatchData_Validate( AKPCLSwatchSystem* Target, int32 Idx ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_AddCustomSwatchData( AKPCLSwatchSystem* Target, FCustomSwatchData Data );
	bool Server_AddCustomSwatchData_Validate( AKPCLSwatchSystem* Target, FCustomSwatchData Data ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_UpdateCustomSwatchData( AKPCLSwatchSystem* Target, FCustomSwatchData Data, int32 Idx );
	bool Server_UpdateCustomSwatchData_Validate( AKPCLSwatchSystem* Target, FCustomSwatchData Data, int32 Idx ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_Core_LootChest( class AKPCLLootChest* Target, AFGCharacterPlayer* Player );
	bool Server_Core_LootChest_Validate( class AKPCLLootChest* Target, AFGCharacterPlayer* Player ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_Core_SetMaxItemCount( class AKPCLNetworkCore* Target, TSubclassOf< UFGItemDescriptor > Item, int32 Max );
	bool Server_Core_SetMaxItemCount_Validate( class AKPCLNetworkCore* Target, TSubclassOf< UFGItemDescriptor > Item, int32 Max ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_MoveItemAmount( class UFGInventoryComponent* Source, int32 SourceIndex, UFGInventoryComponent* Target, FItemAmount Amount, bool ResizeToFit );
	bool Server_MoveItemAmount_Validate( class UFGInventoryComponent* Source, int32 SourceIndex, UFGInventoryComponent* Target, FItemAmount Amount, bool ResizeToFit ) { return true; }

	UFUNCTION( BlueprintCallable )
	static int32 MoveItemAmount( class UFGInventoryComponent* Source, int32 SourceIndex, UFGInventoryComponent* Target, FItemAmount Amount, bool ResizeToFit );


	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_RemoveRuleFromNetworkComponent( class UKPCLNetworkPlayerComponent* Target, int32 RuleIndex );
	bool Server_RemoveRuleFromNetworkComponent_Validate( class UKPCLNetworkPlayerComponent* Target, int32 RuleIndex ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_AddRuleFromNetworkComponent( class UKPCLNetworkPlayerComponent* Target, FKPCLPlayerInventoryRules Rule );
	bool Server_AddRuleFromNetworkComponent_Validate( class UKPCLNetworkPlayerComponent* Target, FKPCLPlayerInventoryRules Rule ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_EditRuleFromNetworkComponent( class UKPCLNetworkPlayerComponent* Target, int32 RuleIndex, FKPCLPlayerInventoryRules Rule );
	bool Server_EditRuleFromNetworkComponent_Validate( class UKPCLNetworkPlayerComponent* Target, int32 RuleIndex, FKPCLPlayerInventoryRules Rule ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_TeleportPlayer( class AKPCLNetworkTeleporter* Target, AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter );
	bool Server_TeleportPlayer_Validate( class AKPCLNetworkTeleporter* Target, AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_SetTeleporterData( class AKPCLNetworkTeleporter* Target, FTeleporterInformation NewData );
	bool Server_SetTeleporterData_Validate( class AKPCLNetworkTeleporter* Target, FTeleporterInformation NewData ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_UnlockEndlessShopItems( class AKPCLUnlockSubsystem* Target, AFGCharacterPlayer* Player, const TArray< TSubclassOf<UKPCLEndlessShopItem> >& UnlockingItems );
	bool Server_UnlockEndlessShopItems_Validate( class AKPCLUnlockSubsystem* Target, AFGCharacterPlayer* Player, const TArray< TSubclassOf<UKPCLEndlessShopItem> >& UnlockingItems ) { return true; }

	UFUNCTION( Server, WithValidation, Reliable, BlueprintCallable )
	void Server_SetNewActorData( class AKPCLBuildableDecorActor* Target, TSubclassOf<UKPCLDecorationRecipe> Data, AFGCharacterPlayer* Player );
	bool Server_SetNewActorData_Validate( class AKPCLBuildableDecorActor* Target, TSubclassOf<UKPCLDecorationRecipe> Data, AFGCharacterPlayer* Player ) { return true; }

	UPROPERTY( Replicated )
	bool mDummy = true;
};
