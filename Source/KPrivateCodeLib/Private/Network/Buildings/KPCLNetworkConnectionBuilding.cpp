// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"

#include "KPrivateCodeLibModule.h"
#include "BFL/KBFL_Inventory.h"
#include "C++/KBFLCppInventoryHelper.h"

#include "Net/UnrealNetwork.h"

#include "Network/Buildings/KPCLNetworkCore.h"


AKPCLNetworkConnectionBuilding::AKPCLNetworkConnectionBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	bBindNetworkComponent = true;

	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
}

void AKPCLNetworkConnectionBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(AKPCLNetworkConnectionBuilding, mInformations);
}