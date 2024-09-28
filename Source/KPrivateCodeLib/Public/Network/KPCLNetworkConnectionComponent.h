// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGItemDescriptor.h"
#include "FGPowerConnectionComponent.h"
#include "ItemAmount.h"
#include "UObject/Object.h"
#include "KPCLNetworkConnectionComponent.generated.h"

/**
 * only for indicator
 */
UCLASS(ClassGroup = ( Custom ), meta = ( BlueprintSpawnableComponent ))
class KPRIVATECODELIB_API UKPCLNetworkConnectionComponent : public UFGPowerConnectionComponent
{
	GENERATED_BODY()

public:
	UKPCLNetworkConnectionComponent();

	TMap<TSubclassOf<UFGItemDescriptor>, FItemAmount> mCurrentStateCache;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	bool mIsUpload = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	EResourceForm mItemForm = EResourceForm::RF_SOLID;

	int32 mItemAmount = 1;
	int32 mFluidAmount = 1000;
};