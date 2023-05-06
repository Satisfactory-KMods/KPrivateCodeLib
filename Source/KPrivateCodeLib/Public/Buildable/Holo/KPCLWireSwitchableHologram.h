// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGRecipeManager.h"
#include "Hologram/FGPowerPoleWallHologram.h"
#include "Hologram/FGWireHologram.h"
#include "KPCLWireSwitchableHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLWireSwitchableHologram : public AFGWireHologram
{
	GENERATED_BODY()

public:
	AKPCLWireSwitchableHologram();

	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;
	virtual void BeginPlay() override;
	virtual void OnBuildModeChanged() override;
	virtual void GetSupportedBuildModes_Implementation( TArray< TSubclassOf< UFGHologramBuildModeDescriptor > >& out_buildmodes ) const override;
	virtual void SpawnChildren( AActor* hologramOwner, FVector spawnLocation, APawn* hologramInstigator ) override;
	virtual void Tick( float DeltaSeconds ) override;

	TArray< TSubclassOf< UFGRecipe > > GetWallRecipes() const;

	template< class T >
	T* FindBestPoleHoloFor();
	void PoleReplacement();

	UPROPERTY()
	AFGRecipeManager* mRecipeManager;

	UPROPERTY( EditDefaultsOnly, Category = "KMods" )
	TArray< TSubclassOf< UFGRecipe > > mTierPoleRecipes;

	UPROPERTY( EditDefaultsOnly, Category = "KMods" )
	TArray< TSubclassOf< UFGRecipe > > mTierPoleWallRecipes;

	UPROPERTY( EditDefaultsOnly, Category = "KMods" )
	TArray< TSubclassOf< UFGRecipe > > mTierPoleWallDoubleRecipes;

	UPROPERTY( EditDefaultsOnly, Category = "KMods" )
	TSubclassOf< UFGRecipe > mFallbackPoleRecipe;

	UPROPERTY( EditDefaultsOnly, Category = "KMods" )
	TSubclassOf< UFGRecipe > mFallbackWallPoleRecipe;

	UPROPERTY( EditDefaultsOnly, Category = "Power pole" )
	TArray< TSubclassOf< UFGHologramBuildModeDescriptor > > mTierBuildMode;

	// Holos
	UPROPERTY( Replicated )
	TArray< AFGPowerPoleWallHologram* > mWallHolos;

	UPROPERTY( Replicated )
	TArray< AFGPowerPoleHologram* > mPoleHolos;

	bool bDelayNextFrames = false;
	int32 DelayedFrames = 0;
};

template< class T >
T* AKPCLWireSwitchableHologram::FindBestPoleHoloFor()
{
	const int32 StartIdx = mTierBuildMode.IndexOfByKey( GetCurrentBuildMode() );
	AFGPowerPoleHologram* BestHolo = nullptr;

	if( T::StaticClass()->IsChildOf( AFGPowerPoleWallHologram::StaticClass() ) )
	{
		for( int32 Idx = StartIdx; Idx >= 0; --Idx )
		{
			if( !mWallHolos.IsValidIndex( Idx ) ) continue;

			if( IsValid( mWallHolos[ Idx ] ) )
			{
				BestHolo = mWallHolos[ Idx ];
				break;
			}
		}
	}
	else
	{
		for( int32 Idx = StartIdx; Idx >= 0; --Idx )
		{
			if( !mPoleHolos.IsValidIndex( Idx ) ) continue;

			if( IsValid( mPoleHolos[ Idx ] ) )
			{
				BestHolo = mPoleHolos[ Idx ];
				break;
			}
		}
	}

	return Cast< T >( BestHolo );
}
