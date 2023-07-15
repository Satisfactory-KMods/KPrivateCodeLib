﻿// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Interfaces/KPCLNetworkDataInterface.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "KPCLNetworkPole.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkPole: public AFGBuildablePowerPole, public IKPCLNetworkDataInterface {
	GENERATED_BODY()

	public:
		//Begin IKPCLNetworkDataInterface
		virtual bool              HasCore_Implementation() const override;
		virtual AKPCLNetworkCore* GetCore_Implementation() const override;
		virtual UKPCLNetwork*     GetNetwork_Implementation() const override;
		virtual FNetworkUIData    GetUIDData_Implementation() const override;

		UPROPERTY(EditDefaultsOnly, Category="KMods|UI")
		FNetworkUIData mNetworkUIData;
		// End IKPCLNetworkDataInterface

		virtual void RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;
		virtual void UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		virtual UKPCLNetworkConnectionComponent* GetNetworkConnectionComponent() const;
};
