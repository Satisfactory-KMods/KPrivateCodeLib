// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Buildable/Holo/KPCLWireSwitchableHologram.h"

#include "FGRecipeManager.h"
#include "FGUnlockSubsystem.h"
#include "BFL/KBFL_Player.h"
#include "Equipment/FGBuildGunBuild.h"
#include "Hologram/FGPowerPoleWallHologram.h"
#include "Resources/FGBuildDescriptor.h"


// Sets default values
AKPCLWireSwitchableHologram::AKPCLWireSwitchableHologram()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AKPCLWireSwitchableHologram::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AKPCLWireSwitchableHologram, mWallHolos );
	DOREPLIFETIME( AKPCLWireSwitchableHologram, mPoleHolos );
}

void AKPCLWireSwitchableHologram::BeginPlay()
{
	mRecipeManager = AFGRecipeManager::Get( GetWorld() );

	GetWorldTimerManager().SetTimerForNextTick( this, &AKPCLWireSwitchableHologram::OnBuildModeChanged );

	Super::BeginPlay();
}

void AKPCLWireSwitchableHologram::OnBuildModeChanged()
{
	Super::OnBuildModeChanged();

	if( !HasAuthority() )
	{
		return;
	}

	if( mPowerPole )
	{
		mPowerPole->SetDisabled( true );
	}

	if( mPowerPoleWall )
	{
		mPowerPoleWall->SetDisabled( true );
	}

	mPowerPole = FindBestPoleHoloFor< AFGPowerPoleHologram >();
	mPowerPoleWall = FindBestPoleHoloFor< AFGPowerPoleWallHologram >();
}

void AKPCLWireSwitchableHologram::GetSupportedBuildModes_Implementation( TArray< TSubclassOf< UFGHologramBuildModeDescriptor > >& out_buildmodes ) const
{
	Super::GetSupportedBuildModes_Implementation( out_buildmodes );

	if( IsValid( mRecipeManager ) )
	{
		TArray< TSubclassOf< UFGRecipe > > Arr = Cast< AFGPowerPoleWallHologram >( GetActiveAutomaticPoleHologram() ) != nullptr ? GetWallRecipes() : mTierPoleRecipes;
		for( int32 Idx = 0; Idx < Arr.Num(); ++Idx )
		{
			if( mTierBuildMode.IsValidIndex( Idx ) && mRecipeManager->IsRecipeAvailable( Arr[ Idx ] ) )
			{
				out_buildmodes.AddUnique( mTierBuildMode[ Idx ] );
			}
		}
	}
}

void AKPCLWireSwitchableHologram::SpawnChildren( AActor* hologramOwner, FVector spawnLocation, APawn* hologramInstigator )
{
	Super::SpawnChildren( hologramOwner, spawnLocation, hologramInstigator );

	if( !HasAuthority() ) return;

	if( !mRecipeManager )
	{
		mRecipeManager = AFGRecipeManager::Get( GetWorld() );
	}

	// Set Num to Poles
	mPoleHolos.SetNum( mTierPoleRecipes.Num() );
	mWallHolos.SetNum( GetWallRecipes().Num() );

	// Create Tier Poles and Disable them
	for( int32 Idx = 0; Idx < mTierPoleRecipes.Num(); ++Idx )
	{
		if( ensureAlways( mTierPoleRecipes[Idx] ) )
		{
			if( mRecipeManager->IsRecipeAvailable( mTierPoleRecipes[ Idx ] ) )
			{
				if( AFGPowerPoleHologram* PoleHologram = Cast< AFGPowerPoleHologram >( SpawnChildHologramFromRecipe( this, mTierPoleRecipes[ Idx ], hologramOwner, spawnLocation, hologramInstigator ) ) )
				{
					PoleHologram->SetDisabled( true );

					if( !GetHologramChildren().Contains( PoleHologram ) )
					{
						AddChild( PoleHologram );
					}

					mPoleHolos[ Idx ] = PoleHologram;
				}
			}
		}
	}

	// Create Tier Wall Poles and Disable them
	for( int32 Idx = 0; Idx < GetWallRecipes().Num(); ++Idx )
	{
		if( ensureAlways( GetWallRecipes()[ Idx ] ) )
		{
			if( mRecipeManager->IsRecipeAvailable( GetWallRecipes()[ Idx ] ) )
			{
				if( AFGPowerPoleWallHologram* PoleHologram = Cast< AFGPowerPoleWallHologram >( SpawnChildHologramFromRecipe( this, GetWallRecipes()[ Idx ], hologramOwner, spawnLocation, hologramInstigator ) ) )
				{
					PoleHologram->SetDisabled( true );

					if( !GetHologramChildren().Contains( PoleHologram ) )
					{
						AddChild( PoleHologram );
					}

					mWallHolos[ Idx ] = PoleHologram;
				}
			}
		}
	}
}

void AKPCLWireSwitchableHologram::Tick( float DeltaSeconds )
{
	Super::Tick( DeltaSeconds );

	if( !IsValid( GetCurrentBuildMode() ) || !HasAuthority() ) return;

	bool MustSnap = GetActiveAutomaticPoleHologram() != nullptr;
	bool SnapToWallHolo = false;
	if( MustSnap )
	{
		SnapToWallHolo = Cast< AFGPowerPoleWallHologram >( GetActiveAutomaticPoleHologram() ) != nullptr;
	}

	if( IsValid( mPowerPole ) ) mDefaultPowerPoleRecipe = mPowerPole->GetRecipe();

	if( IsValid( mPowerPoleWall ) ) mDefaultPowerPoleWallRecipe = mPowerPoleWall->GetRecipe();

	// we need to delay frames because 1-2 frames are the double invalid!
	if( bDelayNextFrames )
	{
		DelayedFrames++;

		if( DelayedFrames > 3 )
		{
			bDelayNextFrames = false;
		}

		return;
	}

	// We dont want a double wall holo if we dont target a wall. so we change them down.
	// Note: what works only if the Arrs are even!
	if( !SnapToWallHolo )
	{
		int32 CurIndex = mTierBuildMode.IndexOfByKey( GetCurrentBuildMode() );

		if( CurIndex >= mTierPoleRecipes.Num() )
		{
			int32 Diff = GetWallRecipes().Num() - mTierPoleRecipes.Num();
			CurIndex -= Diff;

			if( mTierBuildMode.IsValidIndex( CurIndex ) )
			{
				if( IsValid( mTierBuildMode[ CurIndex ] ) )
				{
					SetBuildMode( mTierBuildMode[ CurIndex ] );
					return;
				}
			}
		}
	}
}

TArray< TSubclassOf< UFGRecipe > > AKPCLWireSwitchableHologram::GetWallRecipes() const
{
	TArray< TSubclassOf< UFGRecipe > > Arr;
	Arr.Append( mTierPoleWallRecipes );
	Arr.Append( mTierPoleWallDoubleRecipes );
	return Arr;
}

void AKPCLWireSwitchableHologram::PoleReplacement()
{
	bool MustSnap = GetActiveAutomaticPoleHologram() != nullptr;
	bool SnapToWallHolo = false;
	if( MustSnap )
	{
		SnapToWallHolo = Cast< AFGPowerPoleWallHologram >( GetActiveAutomaticPoleHologram() ) != nullptr;
	}

	if( MustSnap )
	{
		SetActiveAutomaticPoleHologram( SnapToWallHolo ? mPowerPoleWall : mPowerPole );

		if( SnapToWallHolo && IsValid( mPowerPoleWall ) )
		{
			mPowerPoleWall->SetDisabled( false );
		}
		else if( IsValid( mPowerPole ) )
		{
			mPowerPole->SetDisabled( false );
		}
	}

	bDelayNextFrames = true;
	DelayedFrames = 0;
}
