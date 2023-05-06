// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGPowerInfoComponent.h"
#include "KPCLNetworkInfoComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam( FCoreStateChanged, bool );
DECLARE_MULTICAST_DELEGATE( FMaxTransferChanged );
/**
 * Component to store and set Bytes
 */
UCLASS( ClassGroup = ( Custom ), meta = ( BlueprintSpawnableComponent ) )
class KPRIVATECODELIB_API UKPCLNetworkInfoComponent : public UFGPowerInfoComponent
{
	GENERATED_BODY()

	// START: UObject
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;
	// END: UObject

public:
	FMaxTransferChanged MaxTransferChanged;
	FCoreStateChanged CoreStateChanged;


	UFUNCTION( BlueprintPure, Category="Network" )
	int32 GetBytes() const;

	UFUNCTION( BlueprintPure, Category="Network" )
	bool HasCore() const;

	void SetHasCore( bool Has );

	// Request for the next Frame
	void SetMax( int32 MaxInput, int32 MaxOutput, bool IsFluid = false );

	UFUNCTION( BlueprintPure, Category="Network" )
	bool IsCore() const;

	UFUNCTION( BlueprintPure, Category="Network" )
	int32 GetMaxInput( bool IsFluid = false ) const;

	UFUNCTION( BlueprintPure, Category="Network" )
	int32 GetMaxOutput( bool IsFluid = false ) const;

	// Request for the next Frame
	UFUNCTION( BlueprintCallable, Category="Network" )
	void SetBytes( int32 Bytes );

	UFUNCTION( BlueprintPure, Category="Network" )
	class UKPCLNetwork* GetNetwork() const;

	// Request for the next Frame
	UFUNCTION( BlueprintCallable, Category="Network" )
	void SetHandleBytesAsFluid( bool IsFluid );

	UFUNCTION( BlueprintPure, Category="Network" )
	bool IsFluidBytesHandler() const;

private:
	UPROPERTY( SaveGame, Replicated )
	int32 mNetworkBytes = 0;

	UPROPERTY( SaveGame, Replicated )
	bool bHandleBytesAsFluid = false;

	UPROPERTY( EditDefaultsOnly, SaveGame )
	int32 mMaxInputSolid = 0;

	UPROPERTY( EditDefaultsOnly, SaveGame )
	int32 mMaxOutputSolid = 0;

	UPROPERTY( EditDefaultsOnly, SaveGame )
	int32 mMaxInputFluid = 0;

	UPROPERTY( EditDefaultsOnly, SaveGame )
	int32 mMaxOutputFluid = 0;

	// is written by the Circuit!
	UPROPERTY( Replicated )
	bool mNetworkHasCore = false;
};
