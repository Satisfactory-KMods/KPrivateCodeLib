// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkCableHologram.h"

#include "Hologram/FGPowerPoleHologram.h"
#include "Kismet/KismetMathLibrary.h"
#include "Network/KPCLNetwork.h"
#include "Network/KPCLNetworkCable.h"
#include "Network/KPCLNetworkConnectionComponent.h"

AKPCLNetworkCableHologram::AKPCLNetworkCableHologram() {
	PrimaryActorTick.bCanEverTick = false;
}

void AKPCLNetworkCableHologram::BeginPlay() {
	Super::BeginPlay();

	mConnectionMesh = Cast<UStaticMeshComponent>(GetComponentsByTag(UStaticMeshComponent::StaticClass(), FName("ConInd"))[0]);
}

void AKPCLNetworkCableHologram::CheckValidPlacement() {
	Super::CheckValidPlacement();

	if(HasAuthority()) {
		const UKPCLNetworkConnectionComponent* NetworkConnection1 = Cast<UKPCLNetworkConnectionComponent>(mConnections[0]);
		const UKPCLNetworkConnectionComponent* NetworkConnection2 = Cast<UKPCLNetworkConnectionComponent>(mConnections[1]);
		if(IsValid(NetworkConnection1) && IsValid(NetworkConnection2)) {
			const UKPCLNetwork* Network1 = Cast<UKPCLNetwork>(NetworkConnection1->GetPowerCircuit());
			const UKPCLNetwork* Network2 = Cast<UKPCLNetwork>(NetworkConnection2->GetPowerCircuit());
			if(IsValid(Network1) && IsValid(Network2)) {
				if(Network1->GetCircuitID() != Network2->GetCircuitID()) {
					if(Network1->NetworkHasCore() && Network2->NetworkHasCore() && Network1->GetCore() != Network2->GetCore()) {
						AddConstructDisqualifier(UKPCLCDUniqueCore::StaticClass());
					}
				}
			} else if(IsValid(Network1)) {
				if(Network1->NetworkHasCore() && Cast<AKPCLNetworkCore>(NetworkConnection2->GetOwner())) {
					AddConstructDisqualifier(UKPCLCDUniqueCore::StaticClass());
				}
			} else if(IsValid(Network2)) {
				if(Network2->NetworkHasCore() && Cast<AKPCLNetworkCore>(NetworkConnection1->GetOwner())) {
					AddConstructDisqualifier(UKPCLCDUniqueCore::StaticClass());
				}
			}
		}
	}
}

bool AKPCLNetworkCableHologram::TryUpgrade(const FHitResult& hitResult) {
	bool Super = Super::TryUpgrade(hitResult);

	if(hitResult.IsValidBlockingHit() && Super) {
		AKPCLNetworkCable* OtherCable = Cast<AKPCLNetworkCable>(hitResult.GetActor());
		if(IsValid(OtherCable)) {
			SetConnection(0, OtherCable->GetConnection(0));
			SetConnection(1, OtherCable->GetConnection(1));
		}
	}

	return Super;
}
