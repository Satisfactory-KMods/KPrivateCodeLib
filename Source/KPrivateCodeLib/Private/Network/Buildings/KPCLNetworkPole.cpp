// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkPole.h"

#include "Network/KPCLNetwork.h"
#include "Network/Buildings/KPCLNetworkCore.h"

bool AKPCLNetworkPole::HasCore_Implementation() const {
	return Execute_GetCore(this) != nullptr;
}

AKPCLNetworkCore* AKPCLNetworkPole::GetCore_Implementation() const {
	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if(Network) {
		return Cast<AKPCLNetworkCore>(Network->GetCore());
	}
	return nullptr;
}

UKPCLNetwork*                           AKPCLNetworkPole::GetNetwork_Implementation() const {
	if(UKPCLNetworkConnectionComponent* con = GetNetworkConnectionComponent()) {
		return Cast<UKPCLNetwork>(con->GetPowerCircuit());
	}
	return nullptr;
}

FNetworkUIData AKPCLNetworkPole::GetUIDData_Implementation() const {
	return mNetworkUIData;
}

void AKPCLNetworkPole::RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) {
	Super::RegisterInteractingPlayer_Implementation(player);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if(HasAuthority() && Network) {
		Network->RegisterInteractingPlayer(player);

		if(Network->NetworkHasCore()) {
			Execute_RegisterInteractingPlayer(Network->GetCore(), player);
		}
	}
}

void AKPCLNetworkPole::UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) {
	Super::UnregisterInteractingPlayer_Implementation(player);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if(HasAuthority() && Network) {
		Network->RegisterInteractingPlayer(player);

		if(Network->NetworkHasCore()) {
			Execute_UnregisterInteractingPlayer(Network->GetCore(), player);
		}
	}
}

UKPCLNetworkConnectionComponent* AKPCLNetworkPole::GetNetworkConnectionComponent() const {
	return FindComponentByClass<UKPCLNetworkConnectionComponent>();
}
