// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGPlayerController.h"
#include "Structures/KPCLLevelingStruc.h"
#include "UObject/Object.h"
#include "KPCLLevelingSkillInformation.generated.h"

UENUM( BlueprintType )
enum class ESkillingDefaults : uint8
{
	Health      = 0 UMETA( DisplayName = "Health" ),
	HealthSpawn = 1 UMETA( DisplayName = "HealthSpawn" ),
	HealthReg   = 2 UMETA( DisplayName = "HealthReg" ),
	ResNormal   = 3 UMETA( DisplayName = "ResNormal" ),
	ResRadio    = 4 UMETA( DisplayName = "ResRadio" ),
	ResToxic    = 5 UMETA( DisplayName = "ResToxic" ),
	ResHeat     = 6 UMETA( DisplayName = "ResHeat" ),
	JumpHeight  = 7 UMETA( DisplayName = "JumpHeight" ),
	//DoupleJump = 8 UMETA(DisplayName = "DoupleJump"),
	RunningSpeed  = 8 UMETA( DisplayName = "RunningSpeed" ),
	SwimSpeed     = 9 UMETA( DisplayName = "SwimSpeed" ),
	ZiplineSpeed  = 10 UMETA( DisplayName = "ZiplineSpeed" ),
	ClimpSpeed    = 11 UMETA( DisplayName = "ClimpSpeed" ),
	InventorySlot = 12 UMETA( DisplayName = "InventorySlot" ),
	HandSlot      = 13 UMETA( DisplayName = "HandSlot" ),
	AttackMelee   = 14 UMETA( DisplayName = "Melee Damage" ),
	AttackRange   = 15 UMETA( DisplayName = "Range Damage" ),
	HoverRange    = 16 UMETA( DisplayName = "Hover Range" ),
	HoverSpeed    = 17 UMETA( DisplayName = "Hover Speed" ),
	ReloadSpeed   = 18 UMETA( DisplayName = "ReloadSpeed" ),
	FireRate      = 19 UMETA( DisplayName = "FireRate" ),
	Placeholder2  = 20 UMETA( DisplayName = "Placeholder" ),
	Placeholder3  = 21 UMETA( DisplayName = "Placeholder" ),
	Placeholder4  = 22 UMETA( DisplayName = "Placeholder" ),
	Placeholder5  = 23 UMETA( DisplayName = "Placeholder" ),
	Placeholder6  = 24 UMETA( DisplayName = "Placeholder" ),
	Placeholder7  = 25 UMETA( DisplayName = "Placeholder" ),
	Placeholder8  = 26 UMETA( DisplayName = "Placeholder" ),
	Placeholder9  = 27 UMETA( DisplayName = "Placeholder" ),
	Placeholder10 = 28 UMETA( DisplayName = "Placeholder" ),
	Placeholder11 = 29 UMETA( DisplayName = "Placeholder" ),
	MAX           = 20 UMETA( Hidden ),
	All           = 255 UMETA( DisplayName = "All" )};

UENUM( BlueprintType )
enum class EOnceSkilled : uint8
{
	DoupleJump   = 0 UMETA( DisplayName = "DoupleJump" ),
	Placeholder0 = 1 UMETA( DisplayName = "Placeholder" ),
	Placeholder1 = 2 UMETA( DisplayName = "Placeholder" ),
	Placeholder2 = 3 UMETA( DisplayName = "Placeholder" ),
	Placeholder3 = 4 UMETA( DisplayName = "Placeholder" ),
	Placeholder4 = 5 UMETA( DisplayName = "Placeholder" ),
	Placeholder5 = 6 UMETA( DisplayName = "Placeholder" ),
	Placeholder6 = 7 UMETA( DisplayName = "Placeholder" ),
	Placeholder7 = 8 UMETA( DisplayName = "Placeholder" ),
	Placeholder8 = 9 UMETA( DisplayName = "Placeholder" ),
	MAX          = 5 UMETA( Hidden ),
	All          = 255 UMETA( DisplayName = "All" )};

/**
 * 
 */
UCLASS( Blueprintable, BlueprintType, Abstract )
class KPRIVATECODELIB_API UKPCLLevelingSkillInformation : public UObject, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Util" )
	bool IAm( AFGPlayerState* PlayerState, bool& InvalidPlayerID );
	bool IAm( AFGPlayerState* PlayerState );

	virtual void PostLoadGame_Implementation( int32 saveVersion, int32 gameVersion ) override;

	virtual UWorld* GetWorld() const override;
	virtual void ThreadTick( float dt );

	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Util" )
	bool IsOnPlatform() const;

	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Util" )
	bool PlayerHasEnoughItems( TArray< FInventoryStack > Stacks, AFGCharacterPlayer* Char = nullptr ) const;

	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Util" )
	bool RemoveItemsFromPlayer( TArray< FInventoryStack > Stacks, AFGCharacterPlayer* Char ) const;

	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Util" )
	FString GetPlayerName() const;

	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Util" )
	AFGCharacterPlayer* GetPlayerCharacter() const;

	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Util" )
	AFGPlayerController* GetPlayerController() const;

	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Util" )
	class AKPCLLevelingSubsystem* GetSubsystem() const;

	virtual bool IsSupportedForNetworking() const override;
	virtual bool ShouldSave_Implementation() const override { return true; }
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

	virtual void OnExpGain( float Exp, bool IsMaxLevel );
	virtual void OnLevelUp();

	virtual void ReapplyStatsToPlayer();
	virtual void ReapplyDefaults();
	virtual void ConfigureEquipment( AFGCharacterPlayer* PlayerCharacter );
	virtual void DoReskill( ESkillingDefaults Skilling, AFGCharacterPlayer* Char );
	virtual void DoOnceReskill( EOnceSkilled Skilling, AFGCharacterPlayer* Char );
	virtual bool DoSkill( ESkillingDefaults Skilling, AFGCharacterPlayer* Char );
	virtual bool DoOnceSkill( EOnceSkilled Skilling, AFGCharacterPlayer* Char );

	/** Reskill all Skills (can also use by Client side call the Reskill function in the Subsystem */
	UFUNCTION( BlueprintCallable, Category="KMods|LevelingSkillInformation" )
	virtual void ForceReskill();

	/** Get all Skill Points */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	int GetSkillPoints() const;

	/** Get Skill Information from Selected Enum \n\n
	 * @Note: return mHealth if Enum invalid
	 */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	FKPCLSkillInformation& GetSkillByEnum( ESkillingDefaults Skilling );

	/** Get Skill Information from Selected Enum \n\n
	 * @Note: return mHealth if Enum invalid
	 */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	FKPCLSkillInformation GetSkillConst( ESkillingDefaults Skilling ) const;

	/** Get Skill Information from Selected Enum \n\n
	 * @Note: return mHealth if Enum invalid
	 */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	FKPCLOnceSkillInformation& GetOnceSkillByEnum( EOnceSkilled Skilling );

	/** Get Skill Information from Selected Enum \n\n
	 * @Note: return mHealth if Enum invalid
	 */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	FKPCLOnceSkillInformation GettOnceSkillConst( ESkillingDefaults Skilling ) const;

	/** Get Resistence (percent) by Damagetype */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	float GetResByTyp( const UFGDamageType* DamageType );

	/** Is this Skill Valid to Skill? */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	bool IsValidToSkill( FKPCLSkillInformation SkillInfo );

	/** Is this Skill Valid to Skill? */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	bool IsValidToReskill( FKPCLSkillInformation SkillInfo );

	/** Is this Skill Valid to Skill? */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	bool IsOnceValidToSkill( FKPCLOnceSkillInformation SkillInfo );

	/** Is this Skill Valid to Skill? */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	bool IsOnceValidToReskill( FKPCLOnceSkillInformation SkillInfo );

	/** Get the Value from the Skill */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	static float GetSkillValue( FKPCLSkillInformation SkillInfo );

	/** Get the Value from the Skill As Int */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	static int GetValueAsInt( FKPCLSkillInformation SkillInfo );

	/** Get is Max value reached (1 if yes) */
	UFUNCTION( BlueprintPure, Category="KMods|LevelingSkillInformation" )
	static int GetIsMaxValueReached( FKPCLSkillInformation SkillInfo );

	/** Get all UNUSED Skill Points */
	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|LevelingSkillInformation" )
	int GetUnusedSkillPoints();

	UPROPERTY( EditDefaultsOnly, Replicated, SaveGame, Category="KMods", meta=(ArraySizeEnum="ESkillingDefaults") )
	FKPCLSkillInformation mSkills[ ESkillingDefaults::MAX ];

	UPROPERTY( EditDefaultsOnly, Replicated, SaveGame, Category="KMods", meta=(ArraySizeEnum="EOnceSkilled") )
	FKPCLOnceSkillInformation mOnceSkills[ EOnceSkilled::MAX ];

	UPROPERTY( SaveGame, Replicated, BlueprintReadWrite, Category="Kmods|Level" )
	FKPCLLevelInformation mPlayerLevelData;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="Kmods|Res" )
	int32 mReskillDivisor = 4;

	UPROPERTY( EditDefaultsOnly, Category="Kmods|Res" )
	FKPCLDamageType mResDamageClasses;

	UPROPERTY( EditDefaultsOnly, Category="Kmods|Res" )
	FKPCLDamageType mHeatResDamageClasses;

	UPROPERTY( EditDefaultsOnly, Category="Kmods|Res" )
	FKPCLDamageType mRadioResDamageClasses;

	UPROPERTY( EditDefaultsOnly, Category="Kmods|Res" )
	FKPCLDamageType mToxicResDamageClasses;

	// Helper
	UPROPERTY( SaveGame, Replicated )
	AFGPlayerState* mPlayerState;

	UPROPERTY( SaveGame, Replicated )
	FString mPlayerName;

	UPROPERTY( SaveGame, Replicated )
	FString mPlayerId = FString();

	UPROPERTY( SaveGame, Replicated )
	bool bIsOnReskillPlatform = false;
};
