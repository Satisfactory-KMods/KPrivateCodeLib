#pragma once

#include "CoreMinimal.h"
#include "FGResourceSinkSubsystem.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "Resources/FGNoneDescriptor.h"
#include "KPCLNetworkConnectionBuilding.generated.h"

USTRUCT(BlueprintType)
struct FNetworkConnectionInformations
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItemsToGrab = UFGNoneDescriptor::StaticClass();

	UPROPERTY(BlueprintReadOnly)
	bool bIsInput = false;

	UPROPERTY(BlueprintReadOnly)
	EResourceForm mForm = EResourceForm::RF_SOLID;

	bool CanPush() const
	{
		return !bIsInput && mItemsToGrab && mItemsToGrab != UFGNoneDescriptor::StaticClass();
	}

	int32 GetBufferSize() const
	{
		switch (mForm)
		{
		case EResourceForm::RF_LIQUID:
		case EResourceForm::RF_GAS: return mNetworkConnectionFluidBufferSize;
		default: return mNetworkConnectionSolidBufferSize;
		}
	}

	inline static int32 mNetworkConnectionFluidBufferSize = 1000;
	inline static int32 mNetworkConnectionSolidBufferSize = 2;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkConnectionBuilding : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkConnectionBuilding();

	// START: AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// END: AActor
	
private:
	friend class AKPCLNetworkSink;

	UPROPERTY(SaveGame)
	UFGInventoryComponent* mInventory;
};
