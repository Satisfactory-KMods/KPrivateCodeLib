// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGResourceSinkSubsystem.h"
#include "KPCLNetworkConnectionBuilding.h"
#include "KPCLNetworkSink.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkSink : public AKPCLNetworkConnectionBuilding
{
	GENERATED_BODY()

public:
	// START: AActor
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;
	virtual void BeginPlay() override;
	// END: AActor

	virtual void Factory_Tick( float dt ) override;

	UFUNCTION( BlueprintPure, Category="KMods|Network" )
	bool IsItemSinkable() const;

	virtual void SetGrabItem( TSubclassOf< UFGItemDescriptor > Item ) override;

private:
	UPROPERTY( Replicated )
	bool bHasSinkableItem;

	UPROPERTY( Transient )
	AFGResourceSinkSubsystem* mSinkSubsystem;
};
