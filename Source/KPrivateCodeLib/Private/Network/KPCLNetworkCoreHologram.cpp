// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/KPCLNetworkCoreHologram.h"

#include "Subsystem/KPCLUnlockSubsystem.h"

void AKPCLNetworkCoreHologram::BeginPlay()
{
	Super::BeginPlay();

	mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
}

void AKPCLNetworkCoreHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	if (IsValid(mFaxitSubsystem))
	{
		if (mFaxitSubsystem->GetNetworkCount() >= mFaxitSubsystem->GetNetworkLimit())
		{
			AddConstructDisqualifier(UKPCLCDMaxCountReached::StaticClass());
		}
	}
	else
	{
		mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
		AddConstructDisqualifier(UKPCLCDMaxCountReached::StaticClass());
	}
}
