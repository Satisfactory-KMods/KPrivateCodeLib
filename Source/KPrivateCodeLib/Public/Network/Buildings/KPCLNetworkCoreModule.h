// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "KPCLNetworkConnectionBuilding.h"
#include "KPCLNetworkCoreModule.generated.h"

UENUM( BlueprintType )
enum class EKPCLDirection : uint8
{
	Output,
	Input,
	Storage
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCoreModule : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkCoreModule();

	void GetStats( EKPCLDirection& Direction, int32& FluidMax, int32& SolidMax ) const;
	virtual void OnTierUpdated() override;

private:
	UPROPERTY( EditDefaultsOnly, Category="KMods|Network" )
	EKPCLDirection mDirection = EKPCLDirection::Input;

	UPROPERTY( EditDefaultsOnly, Category="KMods|Network" )
	int32 mFluidMax = 30000;

	UPROPERTY( EditDefaultsOnly, Category="KMods|Network" )
	int32 mSolidMax = 15;
};
