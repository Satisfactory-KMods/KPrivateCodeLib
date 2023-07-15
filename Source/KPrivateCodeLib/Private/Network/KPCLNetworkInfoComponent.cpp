// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkInfoComponent.h"

#include "Network/KPCLNetwork.h"

void UKPCLNetworkInfoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKPCLNetworkInfoComponent, mNetworkBytes);
	DOREPLIFETIME(UKPCLNetworkInfoComponent, mNetworkHasCore);
	DOREPLIFETIME(UKPCLNetworkInfoComponent, bHandleBytesAsFluid);
}

int32 UKPCLNetworkInfoComponent::GetBytes() const {
	return mNetworkBytes;
}

bool UKPCLNetworkInfoComponent::HasCore() const {
	if(!IsConnected()) {
		return false;
	}
	return mNetworkHasCore;
}

bool                             UKPCLNetworkInfoComponent::IsCore() const {
	if(AKPCLNetworkBuildingBase* Base = Cast<AKPCLNetworkBuildingBase>(GetOwner())) {
		return Base->IsCore();
	}
	return false;
}

void UKPCLNetworkInfoComponent::SetHasCore(bool Has) {
	if(mNetworkHasCore != Has) {
		mNetworkHasCore = Has;
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateLambda([=]() {
			if(CoreStateChanged.IsBound()) {
				CoreStateChanged.Broadcast(mNetworkHasCore);
			}
		}), TStatId(), nullptr, ENamedThreads::GameThread);
	}
}

void UKPCLNetworkInfoComponent::SetMax(int32 MaxInput, int32 MaxOutput, bool IsFluid) {
	if(IsFluid) {
		if(MaxInput != mMaxInputFluid || MaxOutput != mMaxOutputFluid) {
			mMaxInputFluid = MaxInput;
			mMaxOutputFluid = MaxOutput;
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateLambda([=]() {
				if(MaxTransferChanged.IsBound()) {
					MaxTransferChanged.Broadcast();
				}
			}), TStatId(), nullptr, ENamedThreads::GameThread);
		}
	} else {
		if(MaxInput != mMaxInputSolid || MaxOutput != mMaxOutputSolid) {
			mMaxInputSolid = MaxInput;
			mMaxOutputSolid = MaxOutput;
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateLambda([=]() {
				if(MaxTransferChanged.IsBound()) {
					MaxTransferChanged.Broadcast();
				}
			}), TStatId(), nullptr, ENamedThreads::GameThread);
		}
	}
}

int32 UKPCLNetworkInfoComponent::GetMaxInput(bool IsFluid) const {
	if(!IsConnected() || !HasCore()) {
		return 0;
	}

	if(!GetOwner()->HasAuthority()) {
		if(GetNetwork()) {
			return GetNetwork()->GetMaxInput(IsFluid);
		}
		return 0;
	}

	return IsFluid ? mMaxInputFluid : mMaxInputSolid;
}

int32 UKPCLNetworkInfoComponent::GetMaxOutput(bool IsFluid) const {
	if(!IsConnected() || !HasCore()) {
		return 0;
	}

	if(!GetOwner()->HasAuthority()) {
		if(GetNetwork()) {
			return GetNetwork()->GetMaxOutput(IsFluid);
		}
		return 0;
	}

	return IsFluid ? mMaxOutputFluid : mMaxOutputSolid;
}

void UKPCLNetworkInfoComponent::SetBytes(int32 Bytes) {
	mNetworkBytes = FMath::Clamp<int32>(Bytes, 0, INT32_MAX);
}

UKPCLNetwork*        UKPCLNetworkInfoComponent::GetNetwork() const {
	if(UKPCLNetwork* Network = Cast<UKPCLNetwork>(GetPowerCircuit())) {
		return Network;
	}
	return nullptr;
}

void UKPCLNetworkInfoComponent::SetHandleBytesAsFluid(bool IsFluid) {
	bHandleBytesAsFluid = IsFluid;
}

bool UKPCLNetworkInfoComponent::IsFluidBytesHandler() const {
	return bHandleBytesAsFluid;
}
