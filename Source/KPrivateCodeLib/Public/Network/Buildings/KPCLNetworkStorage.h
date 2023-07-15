// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "KPCLNetworkStorage.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkStorage: public AKPCLNetworkBuildingBase {
	GENERATED_BODY()

	public:
		AKPCLNetworkStorage();

		UPROPERTY(EditDefaultsOnly, Category="KMods|Hologram")
		float mBuildingHeight = 800.f;

	protected:
		virtual void BeginPlay() override;
};
