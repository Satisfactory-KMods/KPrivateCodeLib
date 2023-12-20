// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkStorage.h"

AKPCLNetworkStorage::AKPCLNetworkStorage() {
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AKPCLNetworkStorage::BeginPlay() {
	Super::BeginPlay();

	OnTierUpdated();
}
