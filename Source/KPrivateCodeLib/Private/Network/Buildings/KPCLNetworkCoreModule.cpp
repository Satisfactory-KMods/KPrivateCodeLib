// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkCoreModule.h"

#include "Kismet/KismetMathLibrary.h"

AKPCLNetworkCoreModule::AKPCLNetworkCoreModule()
{
	mShouldUseUiFromMaster = true;
}

void AKPCLNetworkCoreModule::GetStats( EKPCLDirection& Direction, int32& FluidMax, int32& SolidMax ) const
{
	Direction = mDirection;
	FluidMax = mFluidMax * GetTier();
	SolidMax = mSolidMax * GetTier();
}

void AKPCLNetworkCoreModule::OnTierUpdated()
{
	Super::OnTierUpdated();

	if( HasAuthority() )
	{
		mPowerOptions.mPowerMultiplier = 1.f + GetTier() * 0.25f;
	}
}
