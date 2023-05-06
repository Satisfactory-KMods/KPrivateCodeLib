// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/KPCLLevelingSubsystem.h"

#include "FGHealthComponent.h"
#include "FGProjectile.h"
#include "FGSchematic.h"
#include "FGWorkBench.h"
#include "HttpModule.h"
#include "BFL/KBFL_Player.h"
#include "BFL/KBFL_Util.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Creature/FGCreature.h"
#include "Equipment/FGEquipmentStunSpear.h"
#include "Equipment/FGHoverPack.h"
#include "Equipment/FGWeapon.h"
#include "Patching/NativeHookManager.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Subsystem/KPCLPatreonSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN( LevelingSubsystemLog, Log, All )

DEFINE_LOG_CATEGORY( LevelingSubsystemLog )

bool AKPCLLevelingSubsystem::ReplicateSubobjects( UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags )
{
	bool bWroteSomething = Super::ReplicateSubobjects( Channel, Bunch, RepFlags );

	//Replicate a Object of
	//bWroteSomething |= Channel->ReplicateSubobject(mPlayerSkillInformation, *Bunch, *RepFlags);

	//Replicate a list of
	bWroteSomething |= Channel->ReplicateSubobjectList( mPlayerSkillInformation, *Bunch, *RepFlags );

	TArray< UKPCLDebuff_Base* > Debuffs;
	for( FDebuff Debuff : mDebuffs )
	{
		if( Debuff.IsValid() )
		{
			Debuffs.Add( Debuff.mDebuff );
		}
	}

	bWroteSomething |= Channel->ReplicateSubobjectList( Debuffs, *Bunch, *RepFlags );

	return bWroteSomething;
}

AKPCLLevelingSubsystem::AKPCLLevelingSubsystem()
{
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
	PrimaryActorTick.bCanEverTick = true;

	mSchematicTierResearch.Add( 0, 5.f );
	mSchematicTierResearch.Add( 1, 10.f );
	mSchematicTierResearch.Add( 2, 15.f );
	mSchematicTierResearch.Add( 3, 25.f );
	mSchematicTierResearch.Add( 4, 30.f );
	mSchematicTierResearch.Add( 5, 35.f );
	mSchematicTierResearch.Add( 6, 60.f );
	mSchematicTierResearch.Add( 7, 70.f );
	mSchematicTierResearch.Add( 8, 100.f );
	mSchematicTierResearch.Add( 9, 200.f );
}

void AKPCLLevelingSubsystem::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AKPCLLevelingSubsystem, mDebuffs );
	DOREPLIFETIME( AKPCLLevelingSubsystem, mPlayerSkillInformation );
	DOREPLIFETIME( AKPCLLevelingSubsystem, mEventSubsystem );
}

void AKPCLLevelingSubsystem::BeginPlay()
{
	Super::BeginPlay();

#if !WITH_EDITOR
	InitHooks();
	if ( HasAuthority() )
	{
		InitAuthHooks();
	}
#endif

	if( HasAuthority() )
	{
		mEventSubsystem = AFGEventSubsystem::Get( GetWorld() );
		UKPCLLevelingSkillInformation* Information;
		GenerateDefaultsForPlayer( UKBFL_Player::GetFgPlayerState( GetWorld() ), Information );
		mBuildableSubsystem = AFGBuildableSubsystem::Get( GetWorld() );
		mSchematicManager = AFGSchematicManager::Get( GetWorld() );

		mBuildableSubsystem->BuildableConstructedGlobalDelegate.AddDynamic( this, &AKPCLLevelingSubsystem::BuildingConstructed );
		mSchematicManager->PurchasedSchematicDelegate.AddDynamic( this, &AKPCLLevelingSubsystem::OnSchematicUnlocked );

		// Create Default Debuffer
		UKPCLDebuffExp* Debuff = GetDebuffByClass< UKPCLDebuffExp >();
		if( !Debuff && mExpDebuffClass )
		{
			UKPCLDebuffExp* NewDebuff = NewObject< UKPCLDebuffExp >( this, mExpDebuffClass );
			if( NewDebuff )
			{
				UE_LOG( LevelingSubsystemLog, Warning, TEXT("AKPCLLevelingSubsystem create Debuff > %s"), *NewDebuff->GetName() );
				mDebuffs.Add( FDebuff( NewDebuff ) );
			}
		}

		UKPCLBuffExp* Buff = GetDebuffByClass< UKPCLBuffExp >();
		if( !Buff && mExpBuffClass )
		{
			UKPCLBuffExp* NewBuff = NewObject< UKPCLBuffExp >( this, mExpBuffClass );
			if( NewBuff )
			{
				UE_LOG( LevelingSubsystemLog, Warning, TEXT("AKPCLLevelingSubsystem create Buff > %s"), *NewBuff->GetName() );
				mDebuffs.Add( FDebuff( NewBuff ) );
			}
		}

		RefreshExpBuff();
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer( Handle, [&]()
		{
			RefreshExpBuff();
		}, 60.0f, true );
	}
}

void AKPCLLevelingSubsystem::Tick( float DeltaSeconds )
{
	Super::Tick( DeltaSeconds );

	if( HasAuthority() )
	{
		// ParallelTickDebuffs
		if( mDebuffs.Num() > 0 )
		{
			ParallelFor( mDebuffs.Num(), [&]( int32 Idx )
			{
				if( mDebuffs.IsValidIndex( Idx ) )
				{
					if( mDebuffs[ Idx ].IsValid() )
					{
						UKPCLDebuff_Base* Debuff = mDebuffs[ Idx ].mDebuff;
						if( Debuff )
						{
							if( !Debuff->IsPendingKillOrUnreachable() )
							{
								Debuff->TickDebuff( DeltaSeconds );
							}
						}
					}
				}
			} );
		}

		if( mPlayerSkillInformation.Num() > 0 )
		{
			ParallelFor( mPlayerSkillInformation.Num(), [&]( int32 Idx )
			{
				if( mPlayerSkillInformation.IsValidIndex( Idx ) )
				{
					if( mPlayerSkillInformation[ Idx ] )
					{
						if( !mPlayerSkillInformation[ Idx ]->IsPendingKill() )
						{
							mPlayerSkillInformation[ Idx ]->ThreadTick( DeltaSeconds );
						}
					}
				}
			} );
		}

		if( mExpOvertimeInterval.Tick( DeltaSeconds ) )
		{
			GlobalExpGained( mExpOvertime, EExpType::Overtime );

			for( UKPCLLevelingSkillInformation* SkillData : mPlayerSkillInformation )
			{
				if( SkillData )
				{
					if( SkillData->mPlayerState )
					{
						if( !SkillData->mPlayerState->IsInactive() )
						{
							SkillData->ReapplyStatsToPlayer();
						}
					}
				}
			}
		}

		if( mCheckTimer.Tick( DeltaSeconds ) )
		{
			for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++ Iterator )
			{
				APlayerController* PlayerController = Iterator->Get();
				if( AFGPlayerController* FGPlayerController = Cast< AFGPlayerController >( PlayerController ) )
				{
					if( FGPlayerController->PlayerState )
					{
						if( AFGPlayerState* State = FGPlayerController->GetPlayerState< AFGPlayerState >() )
						{
							if( !State->IsInactive() )
							{
								if( !GetSkillDataFromPlayer( State ) )
								{
									AddPending( State );
								}
							}
						}
					}
				}
			}

			for( UKPCLLevelingSkillInformation* SkillData : mPlayerSkillInformation )
			{
				if( SkillData )
				{
					if( SkillData->mPlayerState )
					{
						if( !SkillData->mPlayerState->IsInactive() )
						{
							FKPCLLevelStepInformation LevelStep = GetLevelStep( SkillData->mPlayerState );
							if( mLevelStepMap.IsValidToLevelUp( SkillData->mPlayerLevelData.mLevel, SkillData->mPlayerLevelData.mTotalExp ) )
							{
								LEVELUP( SkillData->mPlayerState, LevelStep );
							}
						}
					}
				}
			}
		}

		if( mPendingStates.Num() > 0 )
		{
			for( AFGPlayerState* PlayerState : mPendingStates )
			{
				OnPlayerJoined( PlayerState );
			}
		}
	}

	if( mDebugCheckTimer.Tick( DeltaSeconds ) )
	{
		UE_LOG( LevelingSubsystemLog, Log, TEXT("Num of Skill data: %d | I have skill data: %d | MyId: %s <> %s | IAmHost: %d"), mPlayerSkillInformation.Num(), GetSkillDataFromPlayer(UKBFL_Player::GetFgPlayerState(this)) != nullptr, *UKBFL_Player::GetFgPlayerState(this)->GetUserID(), *UKBFL_Player::GetPlayerId(UKBFL_Player::GetFgPlayerState(this)), HasAuthority() );
		for( UKPCLLevelingSkillInformation* SkillInformation : mPlayerSkillInformation )
		{
			UE_LOG( LevelingSubsystemLog, Log, TEXT("Skill data ID: %s"), *SkillInformation->mPlayerId );
		}

		if( HasAuthority() )
		{
			for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++ Iterator )
			{
				APlayerController* PlayerController = Iterator->Get();
				if( AFGPlayerController* FGPlayerController = Cast< AFGPlayerController >( PlayerController ) ) UE_LOG( LevelingSubsystemLog, Log, TEXT("Skill data ID: %s"), *FGPlayerController->GetPlayerState<AFGPlayerState>()->GetUserID() );
			}
		}

		if( !HasAuthority() && GetSkillDataFromPlayer( UKBFL_Player::GetFgPlayerState( this ) ) == nullptr )
		{
			AddPending( UKBFL_Player::GetFgPlayerState( this ) );
		}
	}
}

void AKPCLLevelingSubsystem::RefreshExpBuff()
{
	if( UKPCLBuffExp* ExpBuff = GetExpBuff() )
	{
		FHttpModule& HttpModule = FModuleManager::LoadModuleChecked< FHttpModule >( "HTTP" );
		FHttpRequestRef HttpRequest = HttpModule.Get().CreateRequest();

		HttpRequest->OnProcessRequestComplete().BindLambda( [&, ExpBuff]( FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess )
		{
			if( bSuccess )
			{
				TSharedPtr< FJsonObject > JsonObject;
				if( UKPCLPatreonSubsystem::ParseApiQuery( Response, JsonObject ) )
				{
					FString Value;
					if( JsonObject->TryGetStringField( "BuffValue", Value ) )
					{
						UE_LOG( LogTemp, Error, TEXT("OnProcessRequestComplete") );
						ExpBuff->mExpBuffValue = FCString::Atof( *Value );
					}
				}
			}
		} );

		UKPCLPatreonSubsystem::QueryApi( HttpRequest, "LevelUp", { "Buff" } );
	}
}

void AKPCLLevelingSubsystem::InitHooks()
{
	// Damage
	SUBSCRIBE_METHOD_VIRTUAL( AFGEquipment::Equip, GetMutableDefault<AFGEquipment>(), [&](CallScope<void(*)(AFGEquipment*, AFGCharacterPlayer*)>& scope, AFGEquipment* Equipment , AFGCharacterPlayer* character) {
	                          //UE_LOG(LogKPCL, Warning, TEXT("Call AFGEquipment::Equip Hook"))
	                          if(Equipment && !IsPendingKillOrUnreachable()) if(Equipment->HasAuthority() && character && !Equipment->IsPendingKillOrUnreachable()) { AFGPlayerState* PlayerState = character->GetPlayerState<AFGPlayerState>(); if(PlayerState) {
	                          /*if(AFGEquipmentStunSpear* StunSpear = Cast<AFGEquipmentStunSpear>(Equipment))
	                          {
		                          TSubclassOf<AFGEquipmentStunSpear> DefaultSubClass = StunSpear->GetClass();
		                          if(DefaultSubClass)
			                          if(AFGEquipmentStunSpear* DefaultClass = DefaultSubClass.GetDefaultObject())
				                          if(UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer(PlayerState))
					                          if(Skill->GetSkillByEnum(ESkillingDefaults::AttackMelee).mIsSkillActive)
						                          StunSpear->mDamage = DefaultClass->mDamage * Skill->GetSkillByEnum(ESkillingDefaults::AttackMelee).GetValue();
	                          }*/

	                          if(AFGWeapon* Weapon = Cast<AFGWeapon>(Equipment)) { TSubclassOf<AFGWeapon> DefaultSubClass = Weapon->GetClass(); if(DefaultSubClass) if(AFGWeapon* DefaultClass = DefaultSubClass.GetDefaultObject()) if(UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer(PlayerState)) { if(Skill->GetSkillByEnum(ESkillingDefaults::AttackRange).mIsSkillActive) Weapon->mWeaponDamageMultiplier = DefaultClass->mWeaponDamageMultiplier + (Skill-> GetSkillByEnum(ESkillingDefaults::AttackRange).GetValue() - 1.0f); if(Skill->GetSkillByEnum(ESkillingDefaults::ReloadSpeed).mIsSkillActive) Weapon->mReloadTime = DefaultClass->mReloadTime * (1.0f - Skill->GetSkillByEnum( ESkillingDefaults::ReloadSpeed).GetValue()); } } if(AFGHoverPack* HoverPack = Cast<AFGHoverPack>(Equipment)) { TSubclassOf<AFGHoverPack> DefaultSubClass = HoverPack->GetClass(); if(DefaultSubClass && HoverPack->HasAuthority()) if(AFGHoverPack* DefaultClass = DefaultSubClass.GetDefaultObject()) { if(UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer(PlayerState)) { if(Skill->GetSkillByEnum(ESkillingDefaults::HoverRange).mIsSkillActive) HoverPack->mPowerConnectionSearchRadius = DefaultClass->mPowerConnectionSearchRadius * Skill->GetSkillByEnum(ESkillingDefaults::HoverRange).GetValue(); if(Skill->GetSkillByEnum(ESkillingDefaults::HoverSpeed).mIsSkillActive) { HoverPack->mHoverSpeed = DefaultClass->mHoverSpeed * Skill->GetSkillByEnum( ESkillingDefaults::HoverSpeed).GetValue(); } } } } } } } );

	SUBSCRIBE_METHOD( UFGWorkBench::RemoveIngredientsAndAwardRewards, [&](auto& scope, UFGWorkBench* WorkBench, UFGInventoryComponent* inventory, TSubclassOf< UFGRecipe > recipe) { if(ensure(WorkBench)) if(AFGCharacterPlayer* Character = WorkBench->GetPlayerWorkingAtBench()) if(AFGPlayerState* State = Character->GetPlayerState<AFGPlayerState>()) { float CraftTime = WorkBench->GetCurrentDuration(); AddSkillExp(CraftTime * mExpPerHandCraft, State, EExpType::Crafting, false); } } );
}

void AKPCLLevelingSubsystem::InitAuthHooks()
{
	SUBSCRIBE_METHOD( AFGBuildable::PlayDismantleEffects, [&](CallScope<void(*)(AFGBuildable*)>& scope, AFGBuildable* Buildable) { if(Buildable) { BuildingDismantled(Buildable); } } );

	SUBSCRIBE_METHOD_VIRTUAL( AFGCharacterPlayer::AdjustDamage, GetMutableDefault<AFGCharacterPlayer>(), [&](CallScope<float(*)(AFGCharacterPlayer*, AActor*, float, const UDamageType*, AController *, AActor*)>& Scope, AFGCharacterPlayer* Player, AActor* damagedActor, float damageAmount, const UDamageType* damageType, AController* instigatedBy, AActor* damageCauser) { AFGPlayerController* Controller = Cast<AFGPlayerController>(Player->GetController()); if(Controller) { float DamageValue = Scope(Player, damagedActor, damageAmount, damageType, instigatedBy, damageCauser); UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer(Controller->GetPlayerState< AFGPlayerState>()); if(Skill) { float Res = Skill->GetResByTyp(Cast<UFGDamageType>(damageType)); if(Res != 1.0f) { DamageValue -= DamageValue * Skill->GetResByTyp(Cast<UFGDamageType>(damageType)); Scope.Override(DamageValue); } } } } );

	SUBSCRIBE_METHOD( AFGPlayerController::FinishRespawn, [&](CallScope<void(*)(AFGPlayerController*)>& scope, AFGPlayerController* PlayerController) {
	                  //UE_LOG(LogKPCL, Warning, TEXT("Call AFGPlayerController::PlayerControllerFinishRespawn Hook"))
	                  if(PlayerController->HasAuthority()) { AFGCharacterPlayer* Character = PlayerController->GetPawn<AFGCharacterPlayer>(); AFGPlayerState* PlayerState = PlayerController->GetPlayerState<AFGPlayerState>(); if(Character && PlayerState) {
	                  //UE_LOG(LogKPCL, Warning, TEXT("PlayerControllerFinishRespawn > Reapply"));
	                  if(UKPCLLevelingSkillInformation* SkillInformation = GetSkillDataFromPlayer(PlayerState)) { SkillInformation->ReapplyStatsToPlayer(); if(Character->GetHealthComponent()) { float MaxHealth = 100.f; if(SkillInformation->GetSkillByEnum(ESkillingDefaults::Health).mIsSkillActive) MaxHealth = SkillInformation->GetSkillByEnum(ESkillingDefaults::Health).GetValue(); if(SkillInformation->GetSkillByEnum(ESkillingDefaults::HealthSpawn).mIsSkillActive) if(Character->GetHealthComponent()->GetCurrentHealth() / MaxHealth < SkillInformation-> GetSkillByEnum(ESkillingDefaults::HealthSpawn).GetValue()) Character->GetHealthComponent()->ResetHealthFromDeath(); } } } } } );

	SUBSCRIBE_METHOD_VIRTUAL( AFGCreature::Died, GetMutableDefault<AFGCreature>(), [&](auto& scope, AFGCreature* FGCreature, AActor* died) { if(ensureAlways(died)) { AFGCreature* Creature = Cast<AFGCreature>(died); if(ensureAlways(Creature)) { UE_LOG(LevelingSubsystemLog, Log, TEXT("Died %s"), *Creature->GetName()); TSubclassOf<AFGCreature> Class = Creature->GetClass(); if(mCreatureExpMap.Contains(Class)) { UE_LOG(LevelingSubsystemLog, Log, TEXT("mCreatureExpMap %s"), *Creature->GetName()); for(TActorIterator<AFGCharacterPlayer> It(Creature->GetWorld(), AFGCharacterPlayer:: StaticClass()); It; ++It) { AFGCharacterPlayer* Player = *It; if(ensureAlways(Player)) { if(AFGPlayerState* State = Player->GetPlayerState<AFGPlayerState>()) { if(!State->IsInactive() && FVector::Distance(Creature->GetActorLocation(), Player-> GetActorLocation()) <= mCreatureExpApplyRange) { UE_LOG(LevelingSubsystemLog, Log, TEXT("AFGCreature add exp %f"), mCreatureExpMap[Class]); AddSkillExp(mCreatureExpMap[Class], State, EExpType::Kill, false); } } } } } } } } );

	/*SUBSCRIBE_METHOD_VIRTUAL(AFGProjectile::BeginPlay, GetMutableDefault<AFGProjectile>(), [&](CallScope<void(*)(AActor*)>& scope, AActor* Projectile)
	{
		//UE_LOG(LogKPCL, Warning, TEXT("Call AFGProjectile::BeginPlay Hook"))
	
		if(Projectile)
			if(Projectile->HasAuthority())
				if(AFGProjectile* ProjectileCast = Cast<AFGProjectile>(Projectile))
				{
					TArray<AActor*> ActorsToIgnore = TArray<AActor*>{ ProjectileCast };
					TArray<AActor*> OutActors;

					if(UKismetSystemLibrary::SphereOverlapActors(ProjectileCast, ProjectileCast->GetActorLocation(), 50, TArray<TEnumAsByte<EObjectTypeQuery>>{
						EObjectTypeQuery::ObjectTypeQuery1, EObjectTypeQuery::ObjectTypeQuery2
					}, AFGCharacterPlayer::StaticClass(), ActorsToIgnore, OutActors)
					) {
						if(AFGCharacterPlayer* character = Cast<AFGCharacterPlayer>(OutActors[0]))
						{
							AFGPlayerState* PlayerState = character->GetPlayerState<AFGPlayerState>();
							if(PlayerState)
								if(UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer(PlayerState))
								{
									if(Skill->GetSkillByEnum(ESkillingDefaults::AttackRange).mIsSkillActive)
									{
										ProjectileCast->mProjectileData.ExplosionDamage *= Skill->GetSkillByEnum(ESkillingDefaults::AttackRange).GetValue();
										ProjectileCast->mProjectileData.ImpactDamage *= Skill->GetSkillByEnum(ESkillingDefaults::AttackRange).GetValue();
									}
								}
						}
					}
				}
	});*/
}

float AKPCLLevelingSubsystem::GetRealExpGain( float InExp, AFGPlayerState* PlayerState, bool SkipDebuff, bool SkipBuff ) const
{
	float EXP = InExp * GetExpMultiplier();
	if( UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer( PlayerState ) )
	{
		TArray< UKPCLDebuff_Base* > Debuffs;
		GetAllDebuffsByUser( Skill, Debuffs );
		for( UKPCLDebuff_Base* Base : Debuffs )
		{
			if( Base->mIsExpDebuff )
			{
				if( Base->GetClass()->IsChildOf( UKPCLDebuffExp::StaticClass() ) )
				{
					if( !SkipDebuff )
					{
						EXP = Base->ValidateEXP( EXP, PlayerState->HasAuthority() );
					}
				}

				else if( Base->GetClass()->IsChildOf( UKPCLBuffExp::StaticClass() ) )
				{
					if( !SkipBuff )
					{
						EXP = Base->ValidateEXP( EXP, PlayerState->HasAuthority() );
					}
				}
			}
		}
	}
	return EXP;
}

void AKPCLLevelingSubsystem::RepairSkillDatasPlayer( AFGPlayerState* PlayerState )
{
	if( PlayerState )
	{
		TArray< UKPCLLevelingSkillInformation* > AllSkillInformationByPlayer = {};
		float HExp = -1.f;
		UKPCLLevelingSkillInformation* HSkill = nullptr;
		for( UKPCLLevelingSkillInformation* PlayerSkillData : mPlayerSkillInformation )
		{
			if( PlayerSkillData )
			{
				if( PlayerSkillData->IAm( PlayerState ) )
				{
					AllSkillInformationByPlayer.AddUnique( PlayerSkillData );
					if( HExp < PlayerSkillData->mPlayerLevelData.mTotalExp )
					{
						HSkill = PlayerSkillData;
						HExp = PlayerSkillData->mPlayerLevelData.mTotalExp;
					}
				}
			}
		}

		for( UKPCLLevelingSkillInformation* SkillInformation : AllSkillInformationByPlayer )
		{
			if( SkillInformation )
			{
				if( SkillInformation != HSkill )
				{
					if( mPlayerSkillInformation.Contains( SkillInformation ) )
					{
						mPlayerSkillInformation.Remove( SkillInformation );
						SkillInformation->MarkPendingKill();
						UE_LOG( LevelingSubsystemLog, Log, TEXT("SkillInformation->MarkPendingKill() > %s"), *PlayerState->GetPlayerName() );
					}
				}
			}
		}
	}
}

void AKPCLLevelingSubsystem::PlayerEnterSkillPlatform( AActor* Player )
{
	if( HasAuthority() && Cast< AFGCharacterPlayer >( Player ) )
	{
		if( AFGPlayerState* State = Cast< AFGCharacterPlayer >( Player )->GetPlayerState< AFGPlayerState >() )
		{
			if( UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer( State ) )
			{
				Skill->bIsOnReskillPlatform = true;
			}
		}
	}
}

void AKPCLLevelingSubsystem::PlayerLeaveSkillPlatform( AActor* Player )
{
	if( HasAuthority() && Cast< AFGCharacterPlayer >( Player ) )
	{
		if( AFGPlayerState* State = Cast< AFGCharacterPlayer >( Player )->GetPlayerState< AFGPlayerState >() )
		{
			if( UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer( State ) )
			{
				Skill->bIsOnReskillPlatform = false;
			}
		}
	}
}

UKPCLDefaultRCO* AKPCLLevelingSubsystem::GetRcoObject() const
{
	AFGPlayerController* Controller = UKBFL_Player::GetFGController( GetWorld() );
	if( Controller )
	{
		return Controller->GetRemoteCallObjectOfClass< UKPCLDefaultRCO >();
	}
	return nullptr;
}

void AKPCLLevelingSubsystem::OnSchematicUnlocked( TSubclassOf< UFGSchematic > Schematic )
{
	if( Schematic && HasAuthority() )
	{
		int Tier = UFGSchematic::GetTechTier( Schematic );
		if( mSchematicTierResearch.Contains( Tier ) )
		{
			GlobalExpGained( mSchematicTierResearch[ Tier ], EExpType::Researching );
		}
	}
}

AKPCLLevelingSubsystem* AKPCLLevelingSubsystem::Get( UObject* worldContext )
{
	return Cast< AKPCLLevelingSubsystem >( UKBFL_Util::GetSubsystemFromChild( worldContext, StaticClass() ) );
}

bool AKPCLLevelingSubsystem::GenerateDefaultsForPlayer( AFGPlayerState* PlayerState, UKPCLLevelingSkillInformation*& OutSkill )
{
	RepairSkillDatasPlayer( PlayerState );

	OutSkill = GetSkillDataFromPlayer( PlayerState );
	if( !OutSkill )
	{
		//UE_LOG(LevelingSubsystemLog, Log, TEXT("Generate SkillDataFromPlayer"));

		OutSkill = NewObject< UKPCLLevelingSkillInformation >( this, GetSkillDataClass() );
		OutSkill->mPlayerState = PlayerState;
		OutSkill->mPlayerId = UKBFL_Player::GetPlayerId( PlayerState );
		mPlayerSkillInformation.Add( OutSkill );

		if( OutSkill ) UE_LOG( LevelingSubsystemLog, Log, TEXT("Player Added: %s"), *PlayerState->GetPlayerName() )
		else UE_LOG( LevelingSubsystemLog, Log, TEXT("Player CANNOT! Added: %s"), *PlayerState->GetPlayerName() )
		OutSkill->ReapplyStatsToPlayer();
	}
	else
	{
		UE_LOG( LevelingSubsystemLog, Log, TEXT("ReapplyStatsToPlayer Exsisting") );
		OutSkill->ReapplyStatsToPlayer();
	}

	ForceNetUpdate();
	return OutSkill != nullptr;
}

void AKPCLLevelingSubsystem::OnPlayerJoined( AFGPlayerState* PlayerState )
{
	if( HasAuthority() && PlayerState )
	{
		if( !PlayerState->IsInactive() )
		{
			FString PlayerID = UKBFL_Player::GetPlayerId( PlayerState );
			if( PlayerID == "Invalid" || PlayerID == "UnknownId" )
			{
				UE_LOG( LevelingSubsystemLog, Log, TEXT("Invalid Player ID found: %s"), *PlayerID );
				return;
			}

			UKPCLLevelingSkillInformation* SkillInformation;
			if( GenerateDefaultsForPlayer( PlayerState, SkillInformation ) )
			{
				UE_LOG( LevelingSubsystemLog, Log, TEXT("OnPlayerJoined SkillInformation valid: %s - %s"), *PlayerState->GetPlayerName(), *PlayerID );
				PlayerState->GetOwningController()->OnFinishRespawn.AddUniqueDynamic( SkillInformation, &UKPCLLevelingSkillInformation::ReapplyStatsToPlayer );

				if( mPendingStates.Contains( PlayerState ) )
				{
					mPendingStates.Remove( PlayerState );
					UE_LOG( LevelingSubsystemLog, Log, TEXT("Remove Pending after create Defaults > ForceNetUpdate") );
				}
				ForceNetUpdate();
			}
		}
	}
}

void AKPCLLevelingSubsystem::BuildingConstructedNew( TArray< TSubclassOf< AActor > > Buildables, AFGPlayerState* PlayerState )
{
	float Exp = 0.0f;
	if( Buildables.Num() > 0 )
	{
		for( auto Buildable : Buildables )
		{
			if( Buildable )
			{
				bool wasFound = false;
				//UE_LOG(LevelingSubsystemLog, Log, TEXT("Build: %s"), *Buildable->GetName());
				for( auto ExpMap : mBuildingClassMap )
				{
					if( Buildable->IsChildOf( ExpMap.Key ) )
					{
						Exp += ExpMap.Value;
						wasFound = true;
					}
				}

				if( wasFound )
				{
					continue;
				}
				wasFound = false;


				for( auto ExpMap : mGeneralClassMap )
				{
					if( Buildable->IsChildOf( ExpMap.Key ) )
					{
						Exp += ExpMap.Value;
						wasFound = true;
					}
				}
				if( wasFound )
				{
					continue;
				}

				if( Buildable->IsChildOf( AFGBuildable::StaticClass() ) )
				{
					Exp += mGeneralClassMap[ AActor::StaticClass() ];
				}
			}
		}
	}

	if( Exp > 0.0f )
	{
		LocalExpGained( Exp, EExpType::Build, PlayerState );
	}
}

void AKPCLLevelingSubsystem::BuildingConstructed( AFGBuildable* buildable )
{
	if( buildable )
	{
		UClass* Class = buildable->GetClass();
		GlobalExpGained( GetExpByClass( Class ), EExpType::Build );
	}
}

void AKPCLLevelingSubsystem::BuildingDismantled( AFGBuildable* buildable )
{
	//UE_LOG(LevelingSubsystemLog, Warning, TEXT("BuildingDismantled > %s"), *buildable->GetClass()->GetName());
	if( buildable && HasAuthority() )
	{
		if( UKPCLDebuffExp* Debuff = GetDebuffByClass< UKPCLDebuffExp >() )
		{
			//UE_LOG(LevelingSubsystemLog, Warning, TEXT("BuildingDismantled > Debuff > %s | Exp: %f"), *Debuff->GetName(), GetExpByClass(buildable->GetClass()) * GetExpMultiplier());
			Debuff->OnExpLost( GetRealExpGain( GetExpByClass( buildable->GetClass() ), UKBFL_Player::GetFgPlayerState( GetWorld() ), true ) );
		}
	}
}

float AKPCLLevelingSubsystem::GetExpByClass( UClass* Class ) const
{
	// Check is Exactly the Class
	for( auto ExpMap : mBuildingClassMap )
	{
		if( Class == ExpMap.Key )
		{
			return ExpMap.Value;
		}
	}

	// if not is Child of
	for( auto ExpMap : mBuildingClassMap )
	{
		if( Class->IsChildOf( ExpMap.Key ) )
		{
			return ExpMap.Value;
		}
	}

	// if not and is a Buildable
	if( Class->IsChildOf( AFGBuildableManufacturer::StaticClass() ) )
	{
		return mDefaultManuEXP;
	}

	// if not and is a Buildable
	if( Class->IsChildOf( AFGBuildableFactory::StaticClass() ) )
	{
		return mDefaultFactoryBuildable;
	}

	// if not and is a Buildable
	if( Class->IsChildOf( AFGBuildable::StaticClass() ) )
	{
		return mDefaultBuildable;
	}

	for( auto ExpMap : mGeneralClassMap )
	{
		if( Class->IsChildOf( ExpMap.Key ) )
		{
			return ExpMap.Value;
		}
	}

	return 0.0f;
}

void AKPCLLevelingSubsystem::LocalExpGained( float Exp, EExpType Type, AFGPlayerState* PlayerState )
{
	if( HasAuthority() )
	{
		if( UKPCLLevelingSkillInformation* SkillData = GetSkillDataFromPlayer( PlayerState ) )
		{
			if( SkillData )
			{
				if( SkillData->mPlayerState )
				{
					AddSkillExp( Exp, PlayerState, Type, false );
				}
			}
		}
	}
	else
	{
		if( GetRcoObject() )
		{
			GetRcoObject()->Server_LocalExpGained( this, Exp, Type, PlayerState );
		}
	}
}

void AKPCLLevelingSubsystem::GlobalExpGained( float Exp, EExpType Type )
{
	if( HasAuthority() )
	{
		for( UKPCLLevelingSkillInformation* SkillData : mPlayerSkillInformation )
		{
			if( SkillData )
			{
				if( SkillData->mPlayerState )
				{
					AddSkillExp( Exp, SkillData->mPlayerState, Type );
				}
			}
		}
	}
	else
	{
		if( GetRcoObject() )
		{
			GetRcoObject()->Server_GlobalExpGained( this, Exp, Type );
		}
	}
}

void AKPCLLevelingSubsystem::Cheat_SetData( FString PlayerName, FKPCLCheating Cheat )
{
	if( HasAuthority() )
	{
		UE_LOG( LevelingSubsystemLog, Log, TEXT("Cheat_SetData > cheat for user %s"), *PlayerName );
		UKPCLLevelingSkillInformation* SkillInformation = nullptr;
		for( UKPCLLevelingSkillInformation* Skill : mPlayerSkillInformation )
		{
			if( Skill->mPlayerState )
			{
				if( !Skill->mPlayerState->IsInactive() )
				{
					if( Skill->mPlayerState->GetPlayerName().Equals( PlayerName, ESearchCase::IgnoreCase ) )
					{
						SkillInformation = Skill;
						UE_LOG( LevelingSubsystemLog, Log, TEXT("Cheat_SetData > Found skill data for user %s"), *PlayerName );
						break;
					}
				}
			}
		}

		if( SkillInformation )
		{
			if( Cheat.mType == ELevelCheat::Exp )
			{
				LocalExpGained( Cheat.mExp, EExpType::Cheat, SkillInformation->mPlayerState );
			}
			else if( Cheat.mType == ELevelCheat::Level )
			{
				SkillInformation->mPlayerLevelData.mTotalExp = mLevelStepMap.GetCurrentNeededExp( Cheat.mLevel );
				SkillInformation->mPlayerLevelData.mLevel = FMath::Clamp( Cheat.mLevel, mLevelStepMap.mMinLevel, mLevelStepMap.mMaxLevel );
				SkillInformation->ForceReskill();
			}
		}
	}
	else
	{
		if( GetRcoObject() )
		{
			GetRcoObject()->Server_DoCheat( this, PlayerName, Cheat );
		}
	}
}

void AKPCLLevelingSubsystem::AddPending( AFGPlayerState* PlayerState )
{
	if( HasAuthority() )
	{
		if( mPendingStates.AddUnique( PlayerState ) >= 0 ) UE_LOG( LevelingSubsystemLog, Log, TEXT("Add Pending") );
	}
	else
	{
		if( GetRcoObject() )
		{
			GetRcoObject()->Server_AddPending( this, PlayerState );
		}
	}
}

// DEPARTED!
float AKPCLLevelingSubsystem::GetExpMultiplier() const
{
	return 1.0f;
	if( HasAuthority() )
	{
		if( mEventSubsystem )
		{
			TArray< EEvents > Events = mEventSubsystem->GetCurrentEvents();
			if( Events.Num() > 0 )
			{
				if( mEventBuffs.Contains( Events[ 0 ] ) )
				{
					return FMath::Clamp( mEventBuffs[ Events[ 0 ] ], 0.5f, 100.f );
				}
			}
		}
	}

	return FMath::Clamp( mDefaultExpBuff, 0.5f, 100.f );
}

int AKPCLLevelingSubsystem::GetSkillPointsByLevel( int Level ) const
{
	return mLevelStepMap.GetSkillPointsByLevel( Level );
}

void AKPCLLevelingSubsystem::DoSkill_Default( ESkillingDefaults Skilling, UKPCLLevelingSkillInformation* SkillInformation, AFGCharacterPlayer* Char )
{
	if( HasAuthority() )
	{
		UE_LOG( LevelingSubsystemLog, Warning, TEXT("Do skill called in host > %s"), *SkillInformation->mPlayerName )
		if( SkillInformation )
		{
			MultiCastSkilled( SkillInformation->mPlayerState, SkillInformation->DoSkill( Skilling, Char ) );
		}
	}
	else
	{
		UE_LOG( LevelingSubsystemLog, Warning, TEXT("Do skill called in client > %s"), *SkillInformation->mPlayerName )
		if( GetRcoObject() )
		{
			GetRcoObject()->Server_DoSkill_Default( this, Skilling, SkillInformation, Char );
		}
	}
}

void AKPCLLevelingSubsystem::DoSkill_Once( EOnceSkilled Skilling, UKPCLLevelingSkillInformation* SkillInformation, AFGCharacterPlayer* Char )
{
	if( HasAuthority() )
	{
		if( SkillInformation )
		{
			MultiCastSkilled( SkillInformation->mPlayerState, SkillInformation->DoOnceSkill( Skilling, Char ) );
		}
	}
	else
	{
		if( GetRcoObject() )
		{
			GetRcoObject()->Server_DoSkill_Once( this, Skilling, SkillInformation, Char );
		}
	}
}

void AKPCLLevelingSubsystem::AddSkillExp( float Points, AFGPlayerState* PlayerState, EExpType Type, bool IsGlobal )
{
	if( PlayerState )
	{
		if( HasAuthority() )
		{
			if( Points > 0 && !PlayerState->IsInactive() )
			{
				UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer( PlayerState );
				Points = GetRealExpGain( Points, PlayerState, true, false );
				Points = GetRealExpGain( Points, PlayerState, false, true );
				if( Skill && Points > 0.0f )
				{
					if( !Skill->mPlayerState->IsInactive() && Skill->mPlayerState->HasAuthority() )
					{
						if( !Skill->mPlayerState->GetPlayerName().IsEmpty() )
						{
							//UE_LOG(LevelingSubsystemLog, Log, TEXT("AddSkillExp: %f > %s"), Points, *Skill->mPlayerState->GetPlayerName());
							// Set XP to 0 (dont gain any XP after max)
							FKPCLLevelStepInformation NextInfo = GetLevelStep( PlayerState );
							Skill->OnExpGain( Points, NextInfo.mIsMaxLevel );
							ForceNetUpdate();

							if( !NextInfo.mIsMaxLevel )
							{
								MultiCastAddSkillExp( Points, PlayerState, Type, IsGlobal );
							}
						}
					}
				}
			}
		}
		else
		{
			if( GetRcoObject() )
			{
				GetRcoObject()->Server_AddSkillExp( this, Points, PlayerState, Type );
			}
		}
	}
}


void AKPCLLevelingSubsystem::DoReskill( UKPCLLevelingSkillInformation* SkillInformation, ESkillingDefaults Skilling, AFGCharacterPlayer* Char )
{
	if( SkillInformation )
	{
		if( SkillInformation->bIsOnReskillPlatform )
		{
			if( HasAuthority() )
			{
				SkillInformation->DoReskill( Skilling, Char );
				ForceNetUpdate();
			}
			else
			{
				if( GetRcoObject() )
				{
					GetRcoObject()->Server_DoReskill( this, SkillInformation, Skilling, Char );
				}
			}
		}
	}
}

void AKPCLLevelingSubsystem::DoOnceReskill( UKPCLLevelingSkillInformation* SkillInformation, EOnceSkilled Skilling, AFGCharacterPlayer* Char )
{
	if( SkillInformation )
	{
		if( SkillInformation->bIsOnReskillPlatform )
		{
			if( HasAuthority() )
			{
				SkillInformation->DoOnceReskill( Skilling, Char );
				ForceNetUpdate();
			}
			else
			{
				if( GetRcoObject() )
				{
					GetRcoObject()->Server_DoOnceReskill( this, SkillInformation, Skilling, Char );
				}
			}
		}
	}
}

void AKPCLLevelingSubsystem::MultiCastLevelUp_Implementation( AFGPlayerState* PlayerState, FKPCLLevelStepInformation LevelInformation, int NewLevel )
{
	OnGetLevelUp.Broadcast( LevelInformation, PlayerState, NewLevel );
}

void AKPCLLevelingSubsystem::MultiCastAddSkillExp_Implementation( float Points, AFGPlayerState* PlayerState, EExpType Type, bool IsGlobal )
{
	OnGainExp.Broadcast( Points, PlayerState, Type, IsGlobal );
}

void AKPCLLevelingSubsystem::LEVELUP( AFGPlayerState* PlayerState, FKPCLLevelStepInformation LevelInformation )
{
	if( HasAuthority() )
	{
		UKPCLLevelingSkillInformation* Skill = GetSkillDataFromPlayer( PlayerState );
		if( Skill )
		{
			Skill->OnLevelUp();
			ForceNetUpdate();
			UE_LOG( LevelingSubsystemLog, Log, TEXT("LEVELUP Level: %d / SkillPoints: %d"), Skill->mPlayerLevelData.mLevel, LevelInformation.mSkillPoints );
			MultiCastLevelUp( PlayerState, LevelInformation, Skill->mPlayerLevelData.mLevel );
		}
	}
	else
	{
		if( GetRcoObject() )
		{
			GetRcoObject()->Server_LEVELUP( this, PlayerState, LevelInformation );
		}
	}
}

UKPCLDebuff_Base* AKPCLLevelingSubsystem::GetDebuffByClass( TSubclassOf< UKPCLDebuff_Base > DebuffClass ) const
{
	for( FDebuff Debuff : mDebuffs )
	{
		if( Debuff == DebuffClass )
		{
			return Debuff.mDebuff;
		}
	}
	return nullptr;
}

void AKPCLLevelingSubsystem::GetAllDebuffsByUser( UKPCLLevelingSkillInformation* Skill, TArray< UKPCLDebuff_Base* >& Out ) const
{
	for( FDebuff Debuff : mDebuffs )
	{
		if( Debuff == Skill )
		{
			if( Debuff.mDebuff->IsActive() )
			{
				//UE_LOG(LevelingSubsystemLog, Log, TEXT("GetAllDebuffsByUser add %s"), *Debuff.mDebuff->GetName());
				Out.Add( Debuff.mDebuff );
			}
		}
	}
}

void AKPCLLevelingSubsystem::MultiCastSkilled_Implementation( AFGPlayerState* PlayerState, bool WasSuccessful )
{
	OnPlayerHaveSkilled.Broadcast( PlayerState, WasSuccessful );
}

TSubclassOf< UKPCLLevelingSkillInformation > AKPCLLevelingSubsystem::GetSkillDataClass() const
{
	return mDefaults;
}

FKPCLLevelStepInformation AKPCLLevelingSubsystem::GetLevelStep( AFGPlayerState* PlayerState ) const
{
	FKPCLLevelStepInformation returnValue = FKPCLLevelStepInformation();
	UKPCLLevelingSkillInformation* SkillInformation = GetSkillDataFromPlayer( PlayerState );
	if( SkillInformation )
	{
		int Level = SkillInformation->mPlayerLevelData.mLevel;
		if( mLevelStepMap.IsInRange( Level ) )
		{
			returnValue.mExp = mLevelStepMap.GetCurrentNeededExp( Level );
			returnValue.mSkillPoints = mLevelStepMap.GetFixedSkillPointsByLevel( Level + 1 );
			returnValue.mIsMaxLevel = mLevelStepMap.IsMaxLevel( Level );
			returnValue.mIsValid = true;
		}
	}
	return returnValue;
}

UKPCLLevelingSkillInformation* AKPCLLevelingSubsystem::GetSkillDataFromPlayer( AFGPlayerState* PlayerState ) const
{
	// if not found Try slow way
	for( UKPCLLevelingSkillInformation* PlayerSkillData : mPlayerSkillInformation )
	{
		if( PlayerSkillData )
		{
			bool InvalidPlayerID = false;
			if( PlayerSkillData->IAm( PlayerState, InvalidPlayerID ) )
			{
				return PlayerSkillData;
			}
			if( InvalidPlayerID && HasAuthority() )
			{
				InvalidPlayerID = false;
				UE_LOG( LevelingSubsystemLog, Log, TEXT("Found SkillData with Invalid PlayerID") );
				PlayerSkillData->ThreadTick( 0 );
				if( PlayerSkillData->IAm( PlayerState, InvalidPlayerID ) )
				{
					return PlayerSkillData;
				}
				if( InvalidPlayerID ) UE_LOG( LevelingSubsystemLog, Log, TEXT("Still Invalid") );
			}
		}
	}

	// Still not? then it isn't exsists
	return nullptr;
}

void AKPCLLevelingSubsystem::MultiCastReskilled_Implementation( AFGPlayerState* PlayerState, bool AllReskilled )
{
	OnPlayerHaveReskilled.Broadcast( PlayerState, AllReskilled );
}
