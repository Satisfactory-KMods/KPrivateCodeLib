// Fill out your copyright notice in the Description page of Project Settings.


#include "Description/KPCLLevelingSkillInformation.h"

#include "FGCharacterMovementComponent.h"
#include "FGHealthComponent.h"
#include "FGInventoryComponentEquipment.h"
#include "FGUnlockSubsystem.h"
#include "ReplicationGraphTypes.h"
#include "BFL/KBFL_Player.h"
#include "Equipment/FGEquipmentStunSpear.h"
#include "Equipment/FGHoverPack.h"
#include "Equipment/FGWeapon.h"
#include "Subsystem/KPCLLevelingSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN( LevelingSkillInformationLog, Log, All )

DEFINE_LOG_CATEGORY( LevelingSkillInformationLog )

void UKPCLLevelingSkillInformation::PostLoadGame_Implementation( int32 saveVersion, int32 gameVersion )
{
	IFGSaveInterface::PostLoadGame_Implementation( saveVersion, gameVersion );
}

UWorld* UKPCLLevelingSkillInformation::GetWorld() const
{
	if( mPlayerState )
	{
		return mPlayerState->GetWorld();
	}
	return UObject::GetWorld();
}

bool UKPCLLevelingSkillInformation::IAm( AFGPlayerState* PlayerState, bool& InvalidPlayerID )
{
	//UE_LOG(LogKLIB_Components, Warning, TEXT("IAm > %s"), *mPlayerId);
	if( PlayerState )
	{
		if( !PlayerState->IsInactive() )
		{
			if( mPlayerId == FString() || mPlayerId == "Invalid" )
			{
				InvalidPlayerID = true;
				return false;
			}

			if( PlayerState->GetOldPlayerName() == mPlayerName || PlayerState->GetPlayerName() == mPlayerName )
			{
				InvalidPlayerID = false;
				if( mPlayerState != PlayerState )
				{
					mPlayerState = PlayerState;
				}
				return true;
			}

			if( UKBFL_Player::GetPlayerId( PlayerState ) == mPlayerId )
			{
				InvalidPlayerID = false;
				if( mPlayerState != PlayerState )
				{
					mPlayerState = PlayerState;
				}
				return true;
			}

			if( PlayerState->GetUserID() == mPlayerId )
			{
				InvalidPlayerID = false;
				if( mPlayerState != PlayerState )
				{
					mPlayerState = PlayerState;
				}
				return true;
			}

			if( mPlayerState == PlayerState )
			{
				InvalidPlayerID = false;
				if( mPlayerState != PlayerState )
				{
					mPlayerState = PlayerState;
				}
				return true;
			}
		}
	}
	return false;
}

bool UKPCLLevelingSkillInformation::IAm( AFGPlayerState* PlayerState )
{
	bool fake;
	return IAm( PlayerState, fake );
}

void UKPCLLevelingSkillInformation::ThreadTick( float dt )
{
	if( mPlayerState && ( mPlayerId == FString() || mPlayerId == "Invalid" || mPlayerId == "UnknownId" ) )
	{
		if( !mPlayerState->IsInactive() )
		{
			if( UKBFL_Player::GetPlayerId( mPlayerState ) != "Invalid" )
			{
				if( UKBFL_Player::GetPlayerId( mPlayerState ) != "UnknownId" && mPlayerId != UKBFL_Player::GetPlayerId( mPlayerState ) )
				{
					mPlayerId = UKBFL_Player::GetPlayerId( mPlayerState );
				}
			}
		}
	}

	if( mPlayerState )
	{
		if( !mPlayerState->IsInactive() )
		{
			if( mPlayerName != mPlayerState->GetPlayerName() )
			{
				mPlayerName = mPlayerState->GetPlayerName();
			}
		}
	}
}

bool UKPCLLevelingSkillInformation::IsOnPlatform() const
{
	return bIsOnReskillPlatform;
}

bool UKPCLLevelingSkillInformation::PlayerHasEnoughItems( TArray< FInventoryStack > Stacks, AFGCharacterPlayer* Char ) const
{
	AFGCharacterPlayer* Player = Char ? Char : UKBFL_Player::GetFGCharacter( GetWorld() );
	if( Player )
	{
		for( FInventoryStack Stack : Stacks )
		{
			if( !Player->GetInventory()->HasItems( Stack.Item.GetItemClass(), Stack.NumItems ) )
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

bool UKPCLLevelingSkillInformation::RemoveItemsFromPlayer( TArray< FInventoryStack > Stacks, AFGCharacterPlayer* Char ) const
{
	if( PlayerHasEnoughItems( Stacks, Char ) )
	{
		if( Char )
		{
			for( FInventoryStack Stack : Stacks )
			{
				Char->GetInventory()->Remove( Stack.Item.GetItemClass(), Stack.NumItems );
			}
			return true;
		}
	}
	return false;
}

FString UKPCLLevelingSkillInformation::GetPlayerName() const
{
	if( mPlayerState )
	{
		if( !mPlayerState->IsInactive() )
		{
			return mPlayerState->GetPlayerName();
		}
	}
	return FString();
}

AFGCharacterPlayer* UKPCLLevelingSkillInformation::GetPlayerCharacter() const
{
	if( mPlayerState )
	{
		if( !mPlayerState->IsInactive() )
		{
			return Cast< AFGCharacterPlayer >( mPlayerState->GetOwnedPawn() );
		}
	}
	return nullptr;
}

AFGPlayerController* UKPCLLevelingSkillInformation::GetPlayerController() const
{
	if( mPlayerState )
	{
		if( !mPlayerState->IsInactive() )
		{
			return mPlayerState->GetOwningController();
		}
	}
	return nullptr;
}

AKPCLLevelingSubsystem* UKPCLLevelingSkillInformation::GetSubsystem() const
{
	return AKPCLLevelingSubsystem::Get( GetWorld() );
}

bool UKPCLLevelingSkillInformation::IsSupportedForNetworking() const
{
	return true;
}

void UKPCLLevelingSkillInformation::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	UObject::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UKPCLLevelingSkillInformation, mOnceSkills );
	DOREPLIFETIME( UKPCLLevelingSkillInformation, mSkills );
	DOREPLIFETIME( UKPCLLevelingSkillInformation, mPlayerState );
	DOREPLIFETIME( UKPCLLevelingSkillInformation, mPlayerLevelData );
	DOREPLIFETIME( UKPCLLevelingSkillInformation, mPlayerId );
	DOREPLIFETIME( UKPCLLevelingSkillInformation, bIsOnReskillPlatform );
}

void UKPCLLevelingSkillInformation::OnExpGain( float Exp, bool IsMaxLevel )
{
	if( !IsMaxLevel )
	{
		mPlayerLevelData.mTotalExp += Exp;
	}
}

void UKPCLLevelingSkillInformation::OnLevelUp()
{
	mPlayerLevelData.mLevel += 1;
}

void UKPCLLevelingSkillInformation::ReapplyStatsToPlayer()
{
	if( mPlayerState )
	{
		if( !mPlayerState->IsInactive() )
		{
			ReapplyDefaults();
			AKPCLLevelingSubsystem* Subsystem = GetSubsystem();
			if( Subsystem && mPlayerState->IsA( AFGPlayerState::StaticClass() ) && !mPlayerState->GetPlayerName().IsEmpty() )
			{
				if( AFGCharacterPlayer* PlayerChar = GetPlayerCharacter() )
				{
					// Force a Reskill if the tUnusedSkillPoints negative (update changes etc)
					if( GetUnusedSkillPoints() < 0 )
					{
						ForceReskill();
					}

					ConfigureEquipment( PlayerChar );


					//if(GetSkillByEnum(ESkillingDefaults::HealthReg).mIsSkillActive)
					//	PlayerChar->mHealthGenerationThreshold = GetSkillByEnum(ESkillingDefaults::HealthReg).GetValue();

					if( GetOnceSkillByEnum( EOnceSkilled::DoupleJump ).mIsSkillActive )
					{
						PlayerChar->JumpMaxCount = GetOnceSkillByEnum( EOnceSkilled::DoupleJump ).IsSkilled() ? 2 : 1;
					}

					if( UFGHealthComponent* HealthComponent = PlayerChar->GetHealthComponent() )
					{
						if( GetSkillByEnum( ESkillingDefaults::HealthSpawn ).mIsSkillActive )
						{
							HealthComponent->mMaxHealth = GetSkillByEnum( ESkillingDefaults::Health ).GetValue();
						}

						if( GetSkillByEnum( ESkillingDefaults::HealthSpawn ).mIsSkillActive )
						{
							HealthComponent->mRespawnHealthFactor = GetSkillByEnum( ESkillingDefaults::HealthSpawn ).GetValue();
						}
					}

					if( UFGCharacterMovementComponent* MovementComponent = Cast< UFGCharacterMovementComponent >( PlayerChar->GetMovementComponent() ) )
					{
						if( GetSkillByEnum( ESkillingDefaults::RunningSpeed ).mIsSkillActive )
						{
							MovementComponent->mMaxSprintSpeed = 900.f * GetSkillByEnum( ESkillingDefaults::RunningSpeed ).GetValue();
						}

						if( GetSkillByEnum( ESkillingDefaults::JumpHeight ).mIsSkillActive )
						{
							MovementComponent->JumpZVelocity = 500.f * GetSkillByEnum( ESkillingDefaults::JumpHeight ).GetValue();
						}

						if( GetSkillByEnum( ESkillingDefaults::ClimpSpeed ).mIsSkillActive )
						{
							MovementComponent->mClimbSpeed = 500.f * GetSkillByEnum( ESkillingDefaults::ClimpSpeed ).GetValue();
						}

						if( GetSkillByEnum( ESkillingDefaults::ZiplineSpeed ).mIsSkillActive )
						{
							MovementComponent->mZiplineSpeed = 1350.f * GetSkillByEnum( ESkillingDefaults::ZiplineSpeed ).GetValue();
						}

						if( GetSkillByEnum( ESkillingDefaults::SwimSpeed ).mIsSkillActive )
						{
							MovementComponent->MaxSwimSpeed = 300.f * GetSkillByEnum( ESkillingDefaults::SwimSpeed ).GetValue();
						}
					}

					if( AFGUnlockSubsystem* UnlockSubsystem = AFGUnlockSubsystem::Get( GetWorld() ) )
					{
						if( GetSkillByEnum( ESkillingDefaults::InventorySlot ).mIsSkillActive )
						{
							if( UFGInventoryComponent* PlayerInventory = PlayerChar->GetInventory() )
							{
								const int TotalInventoryCount = UnlockSubsystem->GetNumTotalInventorySlots() + GetSkillByEnum( ESkillingDefaults::InventorySlot ).GetValue();
								if( PlayerInventory->GetSizeLinear() != TotalInventoryCount )
								{
									PlayerInventory->Resize( TotalInventoryCount );
								}
							}
						}

						if( GetSkillByEnum( ESkillingDefaults::HandSlot ).mIsSkillActive )
						{
							if( UFGInventoryComponentEquipment* PlayerArmInventory = PlayerChar->GetEquipmentSlot( EEquipmentSlot::ES_ARMS ) )
							{
								const int TotalArmInventoryCount = UnlockSubsystem->GetNumTotalArmEquipmentSlots() + GetSkillByEnum( ESkillingDefaults::HandSlot ).GetValue();
								if( PlayerArmInventory->GetSizeLinear() != TotalArmInventoryCount )
								{
									PlayerArmInventory->Resize( TotalArmInventoryCount );
								}
							}
						}
					}

					Subsystem->ForceNetUpdate();
				}
			}
		}
	}
}

void UKPCLLevelingSkillInformation::ReapplyDefaults()
{
	AKPCLLevelingSubsystem* Subsystem = GetSubsystem();
	if( Subsystem )
	{
		UKPCLLevelingSkillInformation* Default = Subsystem->GetSkillDataObject< UKPCLLevelingSkillInformation >();

		uint8 MAX = ( uint8 )ESkillingDefaults::MAX;
		for( uint8 i = 0; i < MAX; ++i )
		{
			mSkills[ i ].ReApplyDefaults( Default->mSkills[ i ] );
		}

		MAX = ( uint8 )EOnceSkilled::MAX;
		for( uint8 i = 0; i < MAX; ++i )
		{
			mOnceSkills[ i ].ReApplyDefaults( Default->mOnceSkills[ i ] );
		}

		mToxicResDamageClasses = Default->mToxicResDamageClasses;
		mRadioResDamageClasses = Default->mRadioResDamageClasses;
		mHeatResDamageClasses = Default->mHeatResDamageClasses;
		mResDamageClasses = Default->mResDamageClasses;
	}
}

void UKPCLLevelingSkillInformation::ConfigureEquipment( AFGCharacterPlayer* PlayerCharacter )
{
	if( PlayerCharacter )
	{
		if( AFGEquipment* Equipment = PlayerCharacter->GetEquipmentInSlot( EEquipmentSlot::ES_BACK ) )
		{
			// Hoverpack
			if( AFGHoverPack* HoverPack = Cast< AFGHoverPack >( Equipment ) )
			{
				TSubclassOf< AFGHoverPack > DefaultSubClass = HoverPack->GetClass();
				if( DefaultSubClass )
				{
					if( AFGHoverPack* DefaultClass = DefaultSubClass.GetDefaultObject() )
					{
						if( GetSkillByEnum( ESkillingDefaults::HoverRange ).mIsSkillActive )
						{
							HoverPack->mPowerConnectionSearchRadius = DefaultClass->mPowerConnectionSearchRadius * GetSkillByEnum( ESkillingDefaults::HoverRange ).GetValue();
						}

						if( GetSkillByEnum( ESkillingDefaults::HoverSpeed ).mIsSkillActive )
						{
							HoverPack->mHoverSpeed = DefaultClass->mHoverSpeed * GetSkillByEnum( ESkillingDefaults::HoverSpeed ).GetValue();
							//HoverPack->mRailRoadSurfSpeed = DefaultClass->mRailRoadSurfSpeed * GetSkillByEnum(ESkillingDefaults::HoverSpeed).GetValue();
						}
					}
				}
			}

			// Stunspear
			/*if(AFGEquipmentStunSpear* StunSpear = Cast<AFGEquipmentStunSpear>(Equipment))
			{
				TSubclassOf<AFGEquipmentStunSpear> DefaultSubClass = StunSpear->GetClass();
				if(DefaultSubClass)
					if(AFGEquipmentStunSpear* DefaultClass = DefaultSubClass.GetDefaultObject())
					{
						if(GetSkillByEnum(ESkillingDefaults::AttackMelee).mIsSkillActive)
							StunSpear->mDamage = DefaultClass->mDamage * GetSkillByEnum(ESkillingDefaults::AttackMelee).GetValue();
					}
			}*/

			// Weapon
			if( AFGWeapon* GWeapon = Cast< AFGWeapon >( Equipment ) )
			{
				TSubclassOf< AFGWeapon > DefaultSubClass = GWeapon->GetClass();
				if( DefaultSubClass )
				{
					if( AFGWeapon* DefaultClass = DefaultSubClass.GetDefaultObject() )
					{
						if( GetSkillByEnum( ESkillingDefaults::AttackRange ).mIsSkillActive )
						{
							GWeapon->mWeaponDamageMultiplier = DefaultClass->mWeaponDamageMultiplier + ( GetSkillByEnum( ESkillingDefaults::AttackRange ).GetValue() - 1.0f );
						}

						if( GetSkillByEnum( ESkillingDefaults::ReloadSpeed ).mIsSkillActive )
						{
							GWeapon->mReloadTime = DefaultClass->mReloadTime * ( 1.0f - GetSkillByEnum( ESkillingDefaults::ReloadSpeed ).GetValue() );
						}

						/*if(GetSkillByEnum(ESkillingDefaults::FireRate).mIsSkillActive)
							GWeapon->mFireRate = DefaultClass->mFireRate * (1.0f - GetSkillByEnum(ESkillingDefaults::FireRate).GetValue());*/
					}
				}
			}
		}
	}
}

void UKPCLLevelingSkillInformation::DoReskill( ESkillingDefaults Skilling, AFGCharacterPlayer* Char )
{
	if( Skilling == ESkillingDefaults::All )
	{
		ForceReskill();
		return;
	}

	if( RemoveItemsFromPlayer( UKPCLLevelingStrucFunctionLib::GetReskillCosts( GetSkillByEnum( Skilling ), mReskillDivisor ), Char ) )
	{
		GetSkillByEnum( Skilling ).DoReskill();
		ReapplyStatsToPlayer();
	}
}

void UKPCLLevelingSkillInformation::DoOnceReskill( EOnceSkilled Skilling, AFGCharacterPlayer* Char )
{
	if( Skilling == EOnceSkilled::All )
	{
		ForceReskill();
		return;
	}

	if( RemoveItemsFromPlayer( GetOnceSkillByEnum( Skilling ).mSkillCostPerLevel, Char ) )
	{
		GetOnceSkillByEnum( Skilling ).DoReskill();
		ReapplyStatsToPlayer();
	}
}

bool UKPCLLevelingSkillInformation::DoSkill( ESkillingDefaults Skilling, AFGCharacterPlayer* Char )
{
	if( RemoveItemsFromPlayer( UKPCLLevelingStrucFunctionLib::GetSkillCosts( GetSkillByEnum( Skilling ) ), Char ) )
	{
		GetSkillByEnum( Skilling ).DoSkill();
		ReapplyStatsToPlayer();
	}
	return true;
}

bool UKPCLLevelingSkillInformation::DoOnceSkill( EOnceSkilled Skilling, AFGCharacterPlayer* Char )
{
	if( RemoveItemsFromPlayer( GetOnceSkillByEnum( Skilling ).mSkillCostPerLevel, Char ) )
	{
		GetOnceSkillByEnum( Skilling ).DoSkill();
		ReapplyStatsToPlayer();
	}
	return true;
}

void UKPCLLevelingSkillInformation::ForceReskill()
{
	AKPCLLevelingSubsystem* Subsystem = GetSubsystem();
	if( Subsystem )
	{
		uint8 MAX = ( uint8 )ESkillingDefaults::MAX;
		for( uint8 i = 0; i < MAX; ++i )
		{
			GetSkillByEnum( ( ESkillingDefaults )i ).DoReskill();
		}

		MAX = ( uint8 )EOnceSkilled::MAX;
		for( uint8 i = 0; i < MAX; ++i )
		{
			GetOnceSkillByEnum( ( EOnceSkilled )i ).DoReskill();
		}
	}
}

int UKPCLLevelingSkillInformation::GetSkillPoints() const
{
	if( mPlayerState )
	{
		AKPCLLevelingSubsystem* Subsystem = AKPCLLevelingSubsystem::Get( mPlayerState );
		if( Subsystem )
		{
			return Subsystem->GetSkillPointsByLevel( mPlayerLevelData.mLevel );
		}
	}
	return 0;
}

FKPCLSkillInformation& UKPCLLevelingSkillInformation::GetSkillByEnum( ESkillingDefaults Skilling )
{
	return mSkills[ ( uint8 )Skilling ];
}

FKPCLSkillInformation UKPCLLevelingSkillInformation::GetSkillConst( ESkillingDefaults Skilling ) const
{
	return mSkills[ ( uint8 )Skilling ];
}

FKPCLOnceSkillInformation& UKPCLLevelingSkillInformation::GetOnceSkillByEnum( EOnceSkilled Skilling )
{
	return mOnceSkills[ ( uint8 )Skilling ];
}

FKPCLOnceSkillInformation UKPCLLevelingSkillInformation::GettOnceSkillConst( ESkillingDefaults Skilling ) const
{
	return mOnceSkills[ ( uint8 )Skilling ];
}

float UKPCLLevelingSkillInformation::GetResByTyp( const UFGDamageType* DamageType )
{
	if( mToxicResDamageClasses.Contains( DamageType ) )
	{
		if( GetSkillByEnum( ESkillingDefaults::ResToxic ).mIsSkillActive )
		{
			return GetSkillByEnum( ESkillingDefaults::ResToxic ).GetValue();
		}
	}

	if( mResDamageClasses.Contains( DamageType ) )
	{
		if( GetSkillByEnum( ESkillingDefaults::ResNormal ).mIsSkillActive )
		{
			return GetSkillByEnum( ESkillingDefaults::ResNormal ).GetValue();
		}
	}

	if( mRadioResDamageClasses.Contains( DamageType ) )
	{
		if( GetSkillByEnum( ESkillingDefaults::ResRadio ).mIsSkillActive )
		{
			return GetSkillByEnum( ESkillingDefaults::ResRadio ).GetValue();
		}
	}

	if( mHeatResDamageClasses.Contains( DamageType ) )
	{
		if( GetSkillByEnum( ESkillingDefaults::ResHeat ).mIsSkillActive )
		{
			return GetSkillByEnum( ESkillingDefaults::ResHeat ).GetValue();
		}
	}

	return 1.0f;
}

bool UKPCLLevelingSkillInformation::IsValidToSkill( FKPCLSkillInformation SkillInfo )
{
	return SkillInfo.IsValidToSkill( GetUnusedSkillPoints() ) && IsOnPlatform() && PlayerHasEnoughItems( UKPCLLevelingStrucFunctionLib::GetSkillCosts( SkillInfo ) );
}

bool UKPCLLevelingSkillInformation::IsValidToReskill( FKPCLSkillInformation SkillInfo )
{
	return SkillInfo.mSkilled > 0 && PlayerHasEnoughItems( UKPCLLevelingStrucFunctionLib::GetReskillCosts( SkillInfo, mReskillDivisor ) );
}

bool UKPCLLevelingSkillInformation::IsOnceValidToSkill( FKPCLOnceSkillInformation SkillInfo )
{
	return SkillInfo.IsValidToSkill( GetUnusedSkillPoints() ) && PlayerHasEnoughItems( SkillInfo.mSkillCostPerLevel );
}

bool UKPCLLevelingSkillInformation::IsOnceValidToReskill( FKPCLOnceSkillInformation SkillInfo )
{
	return SkillInfo.mSkilled && PlayerHasEnoughItems( SkillInfo.mSkillCostPerLevel );
}

float UKPCLLevelingSkillInformation::GetSkillValue( FKPCLSkillInformation SkillInfo )
{
	return SkillInfo.GetValue();
}

int UKPCLLevelingSkillInformation::GetValueAsInt( FKPCLSkillInformation SkillInfo )
{
	return SkillInfo.GetValueAsInt();
}

int UKPCLLevelingSkillInformation::GetIsMaxValueReached( FKPCLSkillInformation SkillInfo )
{
	return SkillInfo.IsMaxValueReached();
}

int UKPCLLevelingSkillInformation::GetUnusedSkillPoints()
{
	int Skilled = 0;
	//UE_LOG(LogKLIB_Components, Warning, TEXT("GetUnusedSkillPoints BEFORE > %d"), Skilled);

	uint8 MAX = ( uint8 )ESkillingDefaults::MAX;
	for( uint8 i = 0; i < MAX; ++i )
	{
		mSkills[ i ].GetSkilledPoints( Skilled );
	}

	MAX = ( uint8 )EOnceSkilled::MAX;
	for( uint8 i = 0; i < MAX; ++i )
	{
		mOnceSkills[ i ].GetSkilledPoints( Skilled );
	}

	//UE_LOG(LogKLIB_Components, Warning, TEXT("GetUnusedSkillPoints AFTER %d - %d > %d"), GetSkillPoints() , Skilled, GetSkillPoints() - Skilled);
	return GetSkillPoints() - Skilled;
}
