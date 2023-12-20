// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGConstructDisqualifier.h"
#include "Buildings/KPCLNetworkCore.h"
#include "Hologram/FGFactoryHologram.h"
#include "KPCLNetworkCoreHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCoreHologram: public AFGFactoryHologram {
	GENERATED_BODY()

	public:
		virtual void BeginPlay() override;

		virtual void CheckValidPlacement() override;

	private:
		UPROPERTY(Transient)
		AKPCLUnlockSubsystem* mUnlockSubsystem;
};

UCLASS()
class KPRIVATECODELIB_API UKPCLCDMaxCountReached: public UFGConstructDisqualifier {
	GENERATED_BODY()

	friend AKPCLNetworkCoreHologram;

	UKPCLCDMaxCountReached() {
		mDisqfualifyingText = NSLOCTEXT("KPrivateCodeLib", "ConstructDisqualifier_MaxCountReached", "Reached global max count for this building (5)");
	}
};
