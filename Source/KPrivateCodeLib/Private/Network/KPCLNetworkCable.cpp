// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkCable.h"

#include "Network/KPCLNetwork.h"


AKPCLNetworkCable::AKPCLNetworkCable() {
	PrimaryActorTick.bCanEverTick = false;
	mCircuitType = UKPCLNetwork::StaticClass();
}

void AKPCLNetworkCable::BeginPlay() {
	Super::BeginPlay();

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AKPCLNetworkCable::FixWireMesh, 1.f, false);
}

void AKPCLNetworkCable::FixWireMesh() {
	if(ensure(mWireMesh)) {
		mWireMesh->SetCustomPrimitiveDataFloat(0, GetLength() / 100);
		mWireMesh->SetCustomPrimitiveDataFloat(1, 0.0f);
		mWireMesh->SetCustomPrimitiveDataFloat(2, 0.0f);
		mWireMesh->SetCustomPrimitiveDataFloat(3, -48.f);
		mWireMesh->SetCustomPrimitiveDataFloat(4, 0.0f);
		mWireMesh->SetCustomPrimitiveDataFloat(5, 0.703125);
		mWireMesh->SetCustomPrimitiveDataFloat(5, 0.0f);
		mWireMesh->SetCustomPrimitiveDataFloat(6, 0.0f);
		mWireMesh->SetCustomPrimitiveDataFloat(7, 0.0f);
		mWireMesh->SetCustomPrimitiveDataFloat(8, 0.0f);
		mWireMesh->SetCustomPrimitiveDataFloat(9, 0.0f);
		mWireMesh->SetCustomPrimitiveDataFloat(10, 0.0f);
	}
}
