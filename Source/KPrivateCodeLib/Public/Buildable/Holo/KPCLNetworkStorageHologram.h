// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Hologram/FGFactoryHologram.h"
#include "KPCLNetworkStorageHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkStorageHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AKPCLNetworkStorageHologram();
	virtual void Scroll( int32 delta ) override;

	virtual bool IsValidHitResult( const FHitResult& hitResult ) const override;
	virtual void SetHologramLocationAndRotation( const FHitResult& hitResult ) override;

	bool mRotate = false;
};
