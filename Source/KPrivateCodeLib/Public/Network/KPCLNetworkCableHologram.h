// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGConstructDisqualifier.h"
#include "Buildings/KPCLNetworkCore.h"
#include "Hologram/FGWireHologram.h"
#include "KPCLNetworkCableHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCableHologram : public AFGWireHologram
{
	GENERATED_BODY()

public:
	AKPCLNetworkCableHologram();

	virtual void BeginPlay() override;
	virtual void CheckValidPlacement() override;
	virtual void SetHologramLocationAndRotation( const FHitResult& hitResult ) override;
	virtual bool TrySnapToActor( const FHitResult& hitResult ) override;
	virtual void AdjustWire( const FHitResult& hitResult );
	virtual bool TryUpgrade( const FHitResult& hitResult ) override;

private:
	UPROPERTY()
	UStaticMeshComponent* mConnectionMesh;
};

UCLASS()
class KPRIVATECODELIB_API UKPCLCDUniqueCore : public UFGConstructDisqualifier
{
	GENERATED_BODY()

	UKPCLCDUniqueCore()
	{
		mDisqfualifyingText = NSLOCTEXT( "KPrivateCodeLib", "ConstructDisqualifier_UniqueCore", "Both Circuits has a Nexus! Only one per network is allowed!" );
	}
};
