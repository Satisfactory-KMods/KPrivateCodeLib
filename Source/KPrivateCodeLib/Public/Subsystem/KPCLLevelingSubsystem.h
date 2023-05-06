// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "FGBuildableSubsystem.h"
#include "FGSaveInterface.h"
#include "FGSchematicManager.h"
#include "Creature/FGCreature.h"
#include "Description/KPCLDebuff.h"
#include "Description/KPCLLevelingSkillInformation.h"
#include "Structures/KPCLFunctionalStructure.h"
#include "Subsystem/ModSubsystem.h"

#include "KPCLLevelingSubsystem.generated.h"

UENUM( BlueprintType )
enum class EExpType : uint8
{
	Cheat UMETA( DisplayName = "Cheat" ),
	Build UMETA( DisplayName = "Build" ),
	Overtime UMETA( DisplayName = "Exp Overtime" ),
	Kill UMETA( DisplayName = "Kill Mob" ),
	Crafting UMETA( DisplayName = "Crafting" ),
	Researching UMETA( DisplayName = "Researching" )};

USTRUCT( BlueprintType )
struct FDebuff
{
	GENERATED_BODY()

	FDebuff() = default;

	FDebuff( UKPCLDebuff_Base* NewDebuff )
	{
		mDebuff = NewDebuff;
	}

	FDebuff( UKPCLDebuff_Base* NewDebuff, UKPCLLevelingSkillInformation* Bound )
	{
		mDebuff = NewDebuff;
		mBoundedSkill = Bound;
	}

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	UKPCLLevelingSkillInformation* mBoundedSkill = nullptr;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	UKPCLDebuff_Base* mDebuff = nullptr;

	bool IsValid() const
	{
		return mDebuff != nullptr;
	}

	bool IsDebuffGlobal() const
	{
		if( IsValid() )
		{
			return mDebuff->mIsDebuffGlobal && !mBoundedSkill;
		}
		return false;
	}

	bool operator ==( const TSubclassOf< UKPCLDebuff_Base > Other ) const
	{
		if( IsValid() )
		{
			return mDebuff->IsA( Other );
		}
		return false;
	}

	bool operator ==( const UKPCLLevelingSkillInformation* Other ) const
	{
		if( IsValid() )
		{
			if( IsDebuffGlobal() )
			{
				return true;
			}
			return mBoundedSkill == Other;
		}
		return false;
	}
};

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams( FOnGainExp, float, Points, class AFGPlayerState*, PlayerState, EExpType, type, bool, IsGlobal );

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams( FOnGetLevelUp, const FKPCLLevelStepInformation&, Information, AFGPlayerState*, PlayerState, int, NewLevel );

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FOnPlayerHaveSkilled, AFGPlayerState*, PlayerState, bool, WasSuccessful );

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FOnPlayerHaveReskilled, AFGPlayerState*, PlayerState, bool, HasReskilledAll );

UCLASS( Blueprintable, BlueprintType )
class KPRIVATECODELIB_API AKPCLLevelingSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

	AKPCLLevelingSubsystem();

	// Subsystem should Save
	virtual bool ShouldSave_Implementation() const override { return true; }

public:
	/** Start Replication */
	// We want to replicate the skills to and need to Replicate Subobjects then!
	virtual bool ReplicateSubobjects( UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags ) override;
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;
	/** End Replication */

	virtual void BeginPlay() override;
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION()
	virtual void RefreshExpBuff();

	virtual void InitHooks();
	virtual void InitAuthHooks();

	// Native Function 
	virtual bool GenerateDefaultsForPlayer( AFGPlayerState* State, UKPCLLevelingSkillInformation*& OutSkill );
	virtual void OnPlayerJoined( AFGPlayerState* PlayerState );
	virtual void AddSkillExp( float Points, AFGPlayerState* PlayerState, EExpType Type, bool IsGlobal = true );
	virtual float GetRealExpGain( float InExp, AFGPlayerState* PlayerState, bool SkipDebuff = false, bool SkipBuff = false ) const;
	void RepairSkillDatasPlayer( AFGPlayerState* PlayerState );

	UFUNCTION( BlueprintCallable )
	virtual void PlayerEnterSkillPlatform( AActor* Player );

	UFUNCTION( BlueprintCallable )
	virtual void PlayerLeaveSkillPlatform( AActor* Player );

	UFUNCTION( BlueprintCallable )
	virtual void AddPending( AFGPlayerState* PlayerState );

	UFUNCTION( BlueprintCallable )
	class UKPCLDefaultRCO* GetRcoObject() const;

	// Get the SkillInformationClass can overwrite to use a other Subclass of them (to make your own)
	virtual TSubclassOf< UKPCLLevelingSkillInformation > GetSkillDataClass() const;

	/** Begin EXP Stuff */
	UFUNCTION()
	void OnSchematicUnlocked( TSubclassOf< UFGSchematic > Schematic );

	UFUNCTION()
	void BuildingConstructedNew( TArray< TSubclassOf< AActor > > Buildables, AFGPlayerState* PlayerState );

	UFUNCTION()
	void BuildingConstructed( AFGBuildable* buildable );
	void BuildingDismantled( AFGBuildable* buildable );

	float GetExpByClass( UClass* Class ) const;

	/** [Client & Host] one Users Gain a value of EXP */
	UFUNCTION( BlueprintCallable )
	void LocalExpGained( float Exp, EExpType Type, AFGPlayerState* PlayerState );

	/** [Client & Host] All Users Gain a value of EXP */
	UFUNCTION( BlueprintCallable )
	void GlobalExpGained( float Exp, EExpType Type );

	/** [Client & Host] CHEAT add Exp to a Player wid a given value */
	UFUNCTION( BlueprintCallable )
	void Cheat_SetData( FString PlayerName, FKPCLCheating Cheat );
	/** End EXP Stuff */

	/** [Client & Host] Get the Leveling Subsystem (make no different between the Classed (can also be a Child of AKPCLLevelingSubsystem!) */
	UFUNCTION( BlueprintPure, Category = "Subsystem", DisplayName = "GetLevelingSubsystem", meta = ( DefaultToSelf = "WorldContext" ) )
	static AKPCLLevelingSubsystem* Get( UObject* worldContext );

	/** [Client & Host] Reskill a Skill */
	UFUNCTION( BlueprintCallable )
	void DoReskill( UKPCLLevelingSkillInformation* SkillInformation, ESkillingDefaults Skilling, AFGCharacterPlayer* Char );

	/** [Client & Host] Reskill a Skill */
	UFUNCTION( BlueprintCallable )
	void DoOnceReskill( UKPCLLevelingSkillInformation* SkillInformation, EOnceSkilled Skilling, AFGCharacterPlayer* Char );

	/** [Client & Host] Get the Current EXP multiplier (Client side get only the Default value; Host the True value) */
	UFUNCTION( BlueprintPure, Category="KMods|Level Subsystem" )
	virtual float GetExpMultiplier() const;

	/** [Client & Host] Get Skillpoints by Level (all not only unused) */
	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	virtual int GetSkillPointsByLevel( int Level ) const;

	/** [Client & Host] Do Skill a Skill and trigger the Skilled event if it was successful or not!  */
	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	virtual void DoSkill_Default( ESkillingDefaults Skilling, UKPCLLevelingSkillInformation* SkillInformation, AFGCharacterPlayer* Char );

	/** [Client & Host] Do Skill a Skill and trigger the Skilled event if it was successful or not!  */
	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	virtual void DoSkill_Once( EOnceSkilled Skilling, UKPCLLevelingSkillInformation* SkillInformation, AFGCharacterPlayer* Char );

	/** [Client & Host] Called if the Player get a Level up and trigger the Level Up Event */
	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	virtual void LEVELUP( AFGPlayerState* PlayerState, FKPCLLevelStepInformation LevelInformation );

	// Native Helper to get Default SkillDataObject (to make sure in the SkillInformation that new Defaults will be applied)
	template< class T >
	T* GetSkillDataObject() const
	{
		return Cast< T >( GetSkillDataClass().GetDefaultObject() );
	};

	/** [Client & Host] Get a Debuff by Class as Object if they Exsists */
	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	UKPCLDebuff_Base* GetDebuffByClass( TSubclassOf< UKPCLDebuff_Base > DebuffClass ) const;

	/** [Client & Host] Get a Debuff by Class as Object if they Exsists */
	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	void GetAllDebuffsByUser( UKPCLLevelingSkillInformation* Skill, TArray< UKPCLDebuff_Base* >& Out ) const;

	/** [Client & Host] Get the EXP Debuff Object */
	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	FORCEINLINE UKPCLDebuffExp* GetExpDebuff() const { return GetDebuffByClass< UKPCLDebuffExp >(); }

	/** [Client & Host] Get the EXP Debuff Object */
	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	FORCEINLINE UKPCLBuffExp* GetExpBuff() const { return GetDebuffByClass< UKPCLBuffExp >(); }

	UFUNCTION( BlueprintCallable, Category="KMods|Level Subsystem" )
	FORCEINLINE TArray< UKPCLDebuff_Base* > BP_GetDebuffsByClass() const
	{
		return GetDebuffsByClass< UKPCLDebuff_Base >();
	}

	template< class T >
	T* GetDebuffByClass() const
	{
		return Cast< T >( GetDebuffByClass( T::StaticClass() ) );
	};

	template< class T >
	TArray< T* > GetDebuffsByClass() const
	{
		TArray< T* > Debuffs;
		for( FDebuff Debuff : mDebuffs )
		{
			if( Debuff == T::StaticClass() )
			{
				Debuffs.Add( Cast< T >( Debuff.mDebuff ) );
			}
		}
		return Debuffs;
	};

	/** [Client & Host] Get the Next Levelstate for the PlayerState */
	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Level Subsystem" )
	virtual FKPCLLevelStepInformation GetLevelStep( AFGPlayerState* PlayerState ) const;

	/** [Client & Host] Get the SkillInformation for the PlayerState */
	UFUNCTION( BlueprintCallable, BlueprintPure, Category="KMods|Level Subsystem" )
	virtual UKPCLLevelingSkillInformation* GetSkillDataFromPlayer( AFGPlayerState* PlayerState ) const;

	// Native version from GetSkillDataFromPlayer
	template< typename T >
	T* GetSkillDataFromPlayer( AFGPlayerState* PlayerState ) const
	{
		return Cast< T >( GetSkillDataFromPlayer( PlayerState ) );
	}

	/** [Client & Host] Get the Widget that should created (Overlay) */
	UFUNCTION( BlueprintPure, Category="KMods|Level Subsystem" )
	virtual TSubclassOf< UUserWidget > GetWidgetClass() const { return mWidgetClass; };


	// ---------- Start UPROPERTY --------------- //
	UPROPERTY( SaveGame, Replicated, BlueprintReadOnly, Category="KMods" )
	TArray< UKPCLLevelingSkillInformation* > mPlayerSkillInformation;

	UPROPERTY( SaveGame, Replicated, BlueprintReadOnly, Category="KMods|Debuffs" )
	TArray< FDebuff > mDebuffs;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Debuffs" )
	TSubclassOf< UKPCLDebuffExp > mExpDebuffClass;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Debuffs" )
	TSubclassOf< UKPCLBuffExp > mExpBuffClass;

	UPROPERTY( EditDefaultsOnly, Category="KMods" )
	TSubclassOf< UUserWidget > mWidgetClass;

	UPROPERTY( EditDefaultsOnly, Category="KMods" )
	TSubclassOf< UKPCLLevelingSkillInformation > mDefaults;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps" )
	FKPCLLevelRangeInformation mLevelStepMap;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps" )
	float mExpOvertime = 0.1f;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps" )
	float mExpPerHandCraft = 0.5f;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps" )
	TMap< TSubclassOf< AFGCreature >, float > mCreatureExpMap;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps" )
	float mCreatureExpApplyRange = 2000.f;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps|DefaultExpValues" )
	float mDefaultManuEXP = 0.75f;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps|DefaultExpValues" )
	float mDefaultBuildable = 0.15f;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps|DefaultExpValues" )
	float mDefaultFactoryBuildable = 0.25f;

	UPROPERTY( EditDefaultsOnly, Category="KMods|Jobs" )
	FSmartTimer mExpOvertimeInterval = FSmartTimer( 5.0f );

	UPROPERTY( EditDefaultsOnly, Category="KMods|Jobs" )
	FSmartTimer mHealthTimer = FSmartTimer( 5.0f );

	UPROPERTY( EditDefaultsOnly, Category="KMods|Jobs" )
	FSmartTimer mCheckTimer = FSmartTimer( 2.0f );

	UPROPERTY( EditDefaultsOnly, Category="KMods|Jobs" )
	FSmartTimer mDebugCheckTimer = FSmartTimer( 60.0f );

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps" )
	TMap< TSubclassOf< UObject >, float > mGeneralClassMap;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps" )
	TMap< TSubclassOf< AFGBuildable >, float > mBuildingClassMap;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpMaps" )
	TMap< int, float > mSchematicTierResearch;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpBuffs" )
	TMap< EEvents, float > mEventBuffs;

	UPROPERTY( EditDefaultsOnly, Category="KMods|ExpBuffs" )
	float mDefaultExpBuff = 1.0f;

	UPROPERTY( Transient )
	TArray< AFGPlayerState* > mPendingStates;

	UPROPERTY( Transient )
	AFGBuildableSubsystem* mBuildableSubsystem;

	UPROPERTY( Transient )
	AFGSchematicManager* mSchematicManager;

	UPROPERTY( Transient, Replicated )
	AFGEventSubsystem* mEventSubsystem;

	/** Start Events */
	UPROPERTY( BlueprintAssignable, Category="KMods" )
	FOnGainExp OnGainExp;

	UPROPERTY( BlueprintAssignable, Category="KMods" )
	FOnGetLevelUp OnGetLevelUp;

	UPROPERTY( BlueprintAssignable, Category="KMods" )
	FOnPlayerHaveSkilled OnPlayerHaveSkilled;

	UPROPERTY( BlueprintAssignable, Category="KMods" )
	FOnPlayerHaveReskilled OnPlayerHaveReskilled;

	UFUNCTION( NetMulticast, Reliable )
	void MultiCastLevelUp( AFGPlayerState* PlayerState, FKPCLLevelStepInformation LevelInformation, int NewLevel );

	UFUNCTION( NetMulticast, Unreliable )
	void MultiCastAddSkillExp( float Points, AFGPlayerState* PlayerState, EExpType Type, bool IsGlobal = true );

	UFUNCTION( NetMulticast, Unreliable )
	void MultiCastSkilled( AFGPlayerState* PlayerState, bool WasSuccessful );

	UFUNCTION( NetMulticast, Unreliable )
	void MultiCastReskilled( AFGPlayerState* PlayerState, bool AllReskilled );
	/** End Events */
};
