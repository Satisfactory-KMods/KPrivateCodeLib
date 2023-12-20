// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/KPCLNetworkCoreHologram.h"

#include "Subsystem/KPCLUnlockSubsystem.h"

void AKPCLNetworkCoreHologram::BeginPlay() {
	Super::BeginPlay();

	mUnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
}

void AKPCLNetworkCoreHologram::CheckValidPlacement() {
	Super::CheckValidPlacement();

	if(IsValid(mUnlockSubsystem)) {
		if(mUnlockSubsystem->GetGlobalNexusCount() >= mUnlockSubsystem->GetMaxGlobalNexusCount()) {
			AddConstructDisqualifier(UKPCLCDMaxCountReached::StaticClass());
		}
	} else {
		mUnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
		AddConstructDisqualifier(UKPCLCDMaxCountReached::StaticClass());
	}
}
