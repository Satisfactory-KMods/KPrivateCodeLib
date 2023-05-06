// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkConnectionComponent.h"

#include "Network/KPCLNetwork.h"

UKPCLNetworkConnectionComponent::UKPCLNetworkConnectionComponent()
{
	mCircuitType = UKPCLNetwork::StaticClass();
}
