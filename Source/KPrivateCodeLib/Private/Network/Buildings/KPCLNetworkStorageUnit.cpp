// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkStorageUnit.h"

#include "Description/KPCLNetworkDrive.h"


// Sets default values
AKPCLNetworkStorageUnit::AKPCLNetworkStorageUnit() {
	mInventoryDatas[0].mInventorySize = 8;
}

void AKPCLNetworkStorageUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLNetworkStorageUnit, mDriveCount);
}

void AKPCLNetworkStorageUnit::OnDrivesUpdated_Implementation() {
	if(HasAuthority() && GetInventory()) {
		int32 TotalBytes = 0;
		int32 NewCount = 0;
		float NewPowerConsume = 0;
		for(int32 Idx = 0; Idx < GetInventory()->GetSizeLinear(); ++Idx) {
			FInventoryStack Stack;
			if(GetInventory()->GetStackFromIndex(Idx, Stack)) {
				if(Stack.HasItems()) {
					if(const TSubclassOf<UKPCLNetworkDrive> Drive = TSubclassOf<UKPCLNetworkDrive>(Stack.Item.GetItemClass())) {
						NewCount++;
						TotalBytes += UKPCLNetworkDrive::GetFicsitBytes(Drive);
						NewPowerConsume += UKPCLNetworkDrive::GetPowerConsume(Drive);
					}
				}
			}
		}
		mDriveCount = NewCount;

		if(IsValid(GetNetworkInfoComponent())) {
			GetNetworkInfoComponent()->SetBytes(TotalBytes);
		}

		mPowerOptions.mOtherPowerConsume = NewPowerConsume;

		OnRep_DriveCount();
	} else if(!HasAuthority()) {
		OnRep_DriveCount();
	}
}

bool AKPCLNetworkStorageUnit::FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const {
	if(IsValid(object)) {
		if(const TSubclassOf<UKPCLNetworkDrive> Drive{object}) {
			//return UKPCLNetworkDrive::GetIsFluidDrive( Drive ) == mIsFluidStorage;
			return true;
		}
	}
	return false;
}

void AKPCLNetworkStorageUnit::OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) {
	Super::OnInputItemAdded(itemClass, numRemoved);

	OnDrivesUpdated();
}

void AKPCLNetworkStorageUnit::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) {
	Super::OnInputItemRemoved(itemClass, numRemoved);

	OnDrivesUpdated();
}

void AKPCLNetworkStorageUnit::OnMasterBuildingReceived_Implementation(AActor* Actor) {
	Super::OnMasterBuildingReceived_Implementation(Actor);

	if(HasAuthority()) {
		InitNetwork();
		OnDrivesUpdated();
	}
}

void AKPCLNetworkStorageUnit::OnRep_DriveCount() {
	if(mCountToMeshMap.Contains(mDriveCount)) {
		if(IsValid(mCountToMeshMap[mDriveCount]) && mMeshOverwriteInformations.IsValidIndex(0)) {
			mMeshOverwriteInformations[0].mOverwriteMesh = mCountToMeshMap[mDriveCount];
			ApplyMeshInformation(mMeshOverwriteInformations[0]);
		}
	}
}

// Called when the game starts or when spawned
void AKPCLNetworkStorageUnit::BeginPlay() {
	if(!mMeshOverwriteInformations.IsValidIndex(0) && DoesContainLightweightInstances_Native()) {
		FKPCLMeshOverwriteInformation Information;
		Information.mOverwriteMesh = mInstanceDataCDO->GetInstanceData()[0].StaticMesh;
		Information.mOverwriteHandleIndex = 0;
		Information.mUseCustomTransform = false;
		mMeshOverwriteInformations.Add(Information);
	}

	Super::BeginPlay();

	OnRep_DriveCount();

	if(HasAuthority() && GetInventory()) {
		for(int32 Idx = 0; Idx < GetInventory()->GetSizeLinear(); ++Idx) {
			GetInventory()->AddArbitrarySlotSize(Idx, 1);
		}
	}
}

void AKPCLNetworkStorageUnit::InitNetwork() {
	if(HasAuthority() && GetNetworkInfoComponent()) {
		OnDrivesUpdated();
		GetNetworkInfoComponent()->SetHandleBytesAsFluid(mIsFluidStorage);
	} else if(HasAuthority()) {
		GetWorldTimerManager().SetTimerForNextTick(this, &AKPCLNetworkStorageUnit::InitNetwork);
	}
}
