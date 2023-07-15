// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkSink.h"


void AKPCLNetworkSink::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLNetworkSink, bHasSinkableItem);
}

void AKPCLNetworkSink::BeginPlay() {
	Super::BeginPlay();

	if(HasAuthority()) {
		mSinkSubsystem = AFGResourceSinkSubsystem::Get(GetWorld());
		check(mSinkSubsystem);

		bHasSinkableItem = mSinkSubsystem->GetResourceSinkPointsForItem(mInformations.mItemsToGrab) > 0;
		mInformations.bIsInput = false;
		mInformations.mForm = EResourceForm::RF_SOLID;
	}
}

void AKPCLNetworkSink::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	if(HasAuthority() && IsItemSinkable()) {
		if(GetInventory() && !GetInventory()->IsEmpty()) {
			FInventoryStack Stack;
			GetInventory()->GetStackFromIndex(0, Stack);
			mSinkSubsystem->AddPoints_ThreadSafe(Stack.Item.GetItemClass());
			GetInventory()->RemoveFromIndex(0, 1);
		}
	}
}

bool AKPCLNetworkSink::IsItemSinkable() const {
	return bHasSinkableItem;
}

void AKPCLNetworkSink::SetGrabItem(TSubclassOf<UFGItemDescriptor> Item) {
	if(HasAuthority()) {
		mInformations.mItemsToGrab = Item;
		bHasSinkableItem = mSinkSubsystem->GetResourceSinkPointsForItem(Item) > 0;
		if(!bHasSinkableItem) {
			return;
		}
	}

	Super::SetGrabItem(Item);
}
