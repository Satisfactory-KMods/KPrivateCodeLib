// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"

#include "KPrivateCodeLibModule.h"
#include "BFL/KBFL_Inventory.h"
#include "C++/KBFLCppInventoryHelper.h"
#include "Network/Buildings/KPCLNetworkCore.h"


AKPCLNetworkConnectionBuilding::AKPCLNetworkConnectionBuilding() {
	PrimaryActorTick.bCanEverTick = false;
	bBindNetworkComponent = true;
}

void AKPCLNetworkConnectionBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLNetworkConnectionBuilding, mInformations);
	DOREPLIFETIME(AKPCLNetworkConnectionBuilding, mManuellItemCountMax);
	DOREPLIFETIME(AKPCLNetworkConnectionBuilding, mSinkOverflow);
}

bool AKPCLNetworkConnectionBuilding::FormFilterInputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const {
	if(idx == 0) {
		TSubclassOf<UFGItemDescriptor> Item = TSubclassOf<UFGItemDescriptor>(object);
		if(Item) {
			EResourceForm Form = UFGItemDescriptor::GetForm(Item);
			if(Form == mInformations.mForm) {
				return true;
			}
			if(Form == EResourceForm::RF_GAS && mInformations.mForm == EResourceForm::RF_LIQUID) {
				return true;
			}
		}
	}

	return Super::FormFilterInputInventory(object, idx);
}

void AKPCLNetworkConnectionBuilding::BeginPlay() {
	Super::BeginPlay();

	if(HasAuthority()) {
		mInformations.bIsInput = GetConv(0, Input) || GetPipe(0, Input);
		mInformations.mForm = GetPipe(0, Any) != nullptr ? EResourceForm::RF_LIQUID : EResourceForm::RF_SOLID;

		if(GetMaxItemCount() > 0) {
			mResetTimer.mTime = (60.f * static_cast<float>(mInformations.GetBufferSize())) / static_cast<float>(GetMaxItemCount());
		}


		if(GetInventory()) {
			GetInventory()->AddArbitrarySlotSize(0, mInformations.GetBufferSize() * 2);
			FInventoryStack Stack;
			if(GetInventory()->GetStackFromIndex(0, Stack)) {
				if(Stack.HasItems() && !FormFilterInputInventory(Stack.Item.GetItemClass(), 0)) {
					GetInventory()->Empty();
				}
			}
		}
	}
}

void AKPCLNetworkConnectionBuilding::Factory_Tick(float dt) {
	if(HasAuthority()) {
		if(GetIsAllowedToSinkOverflow() != CachedSinkOverflow) {
			OnSinkOverflowChanged();
		}

		if(GetMaxItemCount() > 0) {
			if(mResetTimer.Tick(dt)) {
				mItemCount = mInformations.GetBufferSize();
				mResetTimer.mTime = (60.f * static_cast<float>(mInformations.GetBufferSize())) / static_cast<float>(GetMaxItemCount());
			}
		}
	}

	Super::Factory_Tick(dt);
}

void AKPCLNetworkConnectionBuilding::CollectBelts() {
	Super::CollectBelts();

	if(mInformations.mForm == EResourceForm::RF_SOLID && mInformations.bIsInput) {
		if(mInformations.mItemsToGrab == UFGNoneDescriptor::StaticClass() || !mInformations.mItemsToGrab) {
			UKBFLCppInventoryHelper::PullBeltChildClass(GetInventory(), 0, 0.f, UFGItemDescriptor::StaticClass(), GetConv(0, Input));
		} else {
			UKBFLCppInventoryHelper::PullBelt(GetInventory(), 0, 0.f, mInformations.mItemsToGrab, GetConv(0, Input));
		}
	}
}

void AKPCLNetworkConnectionBuilding::CollectAndPushPipes(float dt, bool IsPush) {
	Super::CollectAndPushPipes(dt, IsPush);

	if(mInformations.bIsInput && !IsPush) {
		if(mInformations.mForm == EResourceForm::RF_LIQUID) {
			if(mInformations.mItemsToGrab == UFGNoneDescriptor::StaticClass() || !mInformations.mItemsToGrab) {
				UKBFLCppInventoryHelper::PullAllFromPipe(GetInventory(), 0, dt, GetPipe(0, Input));
			} else {
				UKBFLCppInventoryHelper::PullPipe(GetInventory(), 0, dt, mInformations.mItemsToGrab, GetPipe(0, Input));
			}
		}
	} else if(!mInformations.bIsInput && IsPush) {
		if(mInformations.mForm == EResourceForm::RF_LIQUID) {
			UKBFLCppInventoryHelper::PushPipe(GetInventory(), 0, dt, GetPipe(0, Output));
		} else if(mInformations.mForm == EResourceForm::RF_SOLID) {
			if(UFGFactoryConnectionComponent* Conv = GetConv(0, Output)) {
				Conv->SetInventory(GetInventory());
				Conv->SetInventoryAccessIndex(0);
			}
		}
	}
}

void AKPCLNetworkConnectionBuilding::OnMaxChanged() {
	Super::OnMaxChanged();

	if(GetMaxItemCount() > 0) {
		mResetTimer.mTime = (60.f * static_cast<float>(mInformations.GetBufferSize())) / static_cast<float>(GetMaxItemCount());
	}
}

void AKPCLNetworkConnectionBuilding::OnSinkOverflowChanged() {
	if(HasAuthority()) {
		mPowerOptions.mOtherPowerConsume = GetIsAllowedToSinkOverflow() ? mExtraPowerConsumeWithSinkOverflow : 0.f;
	}
}

bool AKPCLNetworkConnectionBuilding::GetStack(FInventoryStack& Stack) const {
	if(GetInventory()) {
		return GetInventory()->GetStackFromIndex(0, Stack);
	}
	return false;
}

bool AKPCLNetworkConnectionBuilding::TryToReceiveItems(FInventoryStack& Stack) {
	if(mItemCount > 0) {
		if(GetInventory() && Stack.HasItems() && GetMaxItemCount() > 0) {
			FInventoryStack BuildingStack;
			GetInventory()->GetStackFromIndex(0, BuildingStack);
			Stack.NumItems = FMath::Min(Stack.NumItems, mItemCount);

			if(Stack.HasItems()) {
				if(UKBFLCppInventoryHelper::CanStoreItemStack(GetInventory(), Stack)) {
					FKPCLItemTransferQueue Queue;
					Queue.mAddAmount = true;
					Queue.mAmount = FItemAmount(Stack.Item.GetItemClass(), Stack.NumItems);
					mInventoryQueue.Enqueue(Queue);

					mItemCount -= Stack.NumItems;
					return true;
				}
			}
		}
	}
	return false;
}

bool AKPCLNetworkConnectionBuilding::TryToGrabItems(FInventoryStack& Stack, float MaxSolidBytes, float MaxFluidBytes) {
	if(mItemCount > 0) {
		AFGResourceSinkSubsystem* SinkSubsystem = GetSinkSub();
		if(GetInventory() && (MaxSolidBytes > 0.f || MaxFluidBytes > 0.f) && GetMaxItemCount() > 0) {
			GetInventory()->GetStackFromIndex(0, Stack);
			if(Stack.HasItems()) {
				const int32 NumThisFrame = FMath::Min(Stack.NumItems, mItemCount);
				int32       PossibleToSinkThisFrame = NumThisFrame;

				Stack.NumItems = NumThisFrame;

				const int32 MaxAmount = AKPCLNetworkCore::GetMaxItemsByBytes(Stack.Item.GetItemClass(), MaxFluidBytes, MaxSolidBytes);
				Stack.NumItems = FMath::Min(MaxAmount, Stack.NumItems);

				PossibleToSinkThisFrame -= Stack.NumItems;

				if(Stack.HasItems()) {
					FKPCLItemTransferQueue Queue;
					Queue.mAddAmount = false;
					Queue.mAmount = FItemAmount(Stack.Item.GetItemClass(), Stack.NumItems);
					mInventoryQueue.Enqueue(Queue);

					mItemCount -= Queue.mAmount.Amount;
				}

				if(PossibleToSinkThisFrame > 0 && GetIsAllowedToSinkOverflow() && IsValid(SinkSubsystem) && SinkSubsystem->GetResourceSinkPointsForItem(Stack.Item.GetItemClass()) > 0) {
					FKPCLSinkQueue SinkQueue;
					SinkQueue.mIndex = 0;
					SinkQueue.mAmount = FItemAmount(Stack.Item.GetItemClass(), PossibleToSinkThisFrame);
					mSinkQueue.Enqueue(SinkQueue);

					mItemCount -= SinkQueue.mAmount.Amount;
				}
				return true;
			}
		} else if(GetInventory() && GetMaxItemCount() > 0 && GetIsAllowedToSinkOverflow() && IsValid(SinkSubsystem) && SinkSubsystem->GetResourceSinkPointsForItem(Stack.Item.GetItemClass()) > 0) {
			GetInventory()->GetStackFromIndex(0, Stack);
			const int32 PossibleToSinkThisFrame = FMath::Min(Stack.NumItems, mItemCount);
			if(PossibleToSinkThisFrame > 0) {
				FKPCLSinkQueue SinkQueue;
				SinkQueue.mIndex = 0;
				SinkQueue.mAmount = FItemAmount(Stack.Item.GetItemClass(), PossibleToSinkThisFrame);
				mSinkQueue.Enqueue(SinkQueue);

				mItemCount -= SinkQueue.mAmount.Amount;
			}
		}
	}
	return false;
}

void AKPCLNetworkConnectionBuilding::GetConnectionInformations(FNetworkConnectionInformations& Informations) const {
	Informations = mInformations;
}

int32 AKPCLNetworkConnectionBuilding::GetMaxItemCount() const {
	int32 ItemMax = GetMaxItemCountPure();

	if(mManuellItemCountMax > 0) {
		return FMath::Min(mManuellItemCountMax, ItemMax);
	}

	return ItemMax;
}

int32 AKPCLNetworkConnectionBuilding::GetMaxItemCountPure() const {
	UKPCLNetworkInfoComponent* Info = GetNetworkInfoComponent();
	if(!ensure(Info)) {
		return 0;
	}

	return mInformations.bIsInput ? Info->GetMaxInput(mInformations.mForm != EResourceForm::RF_SOLID) : Info->GetMaxOutput(mInformations.mForm != EResourceForm::RF_SOLID);
}

int32 AKPCLNetworkConnectionBuilding::GetManuellItemCountMax() const {
	return mManuellItemCountMax;
}

void AKPCLNetworkConnectionBuilding::SetManuellItemMax(int32 Value) {
	if(mManuellItemCountMax != Value) {
		if(HasAuthority()) {
			mManuellItemCountMax = FMath::Clamp<int32>(Value, -1, GetMaxItemCountPure());
			if(Value == 0) {
				mManuellItemCountMax = -1;
			}

			// reset timer for handle new time
			mResetTimer.Reset();
			mResetTimer.mTime = (60.f * static_cast<float>(mInformations.GetBufferSize())) / static_cast<float>(GetMaxItemCount());
		} else if(UKPCLDefaultRCO* RCO = GetRCO<UKPCLDefaultRCO>()) {
			RCO->Server_SetManuellItemMax(this, Value);
		}
	}
}

void AKPCLNetworkConnectionBuilding::SetGrabItem(TSubclassOf<UFGItemDescriptor> Item) {
	if(!Item) {
		Item = UFGNoneDescriptor::StaticClass();
	}

	if(mInformations.mItemsToGrab != Item) {
		if(HasAuthority()) {
			mInformations.mItemsToGrab = Item;
		} else if(UKPCLDefaultRCO* RCO = GetRCO<UKPCLDefaultRCO>()) {
			RCO->Server_SetGrabItem(this, Item);
		}
	}
}

TSubclassOf<UFGItemDescriptor> AKPCLNetworkConnectionBuilding::PeekItemClass() const {
	if(mInformations.bIsInput && GetInventory()) {
		FInventoryStack Stack;
		GetInventory()->GetStackFromIndex(0, Stack);
		return Stack.Item.GetItemClass();
	}
	return nullptr;
}

AFGResourceSinkSubsystem* AKPCLNetworkConnectionBuilding::GetSinkSub() {
	if(!IsValid(mSinkSub)) {
		mSinkSub = AFGResourceSinkSubsystem::Get(GetWorld());
	}
	return mSinkSub;
}

bool AKPCLNetworkConnectionBuilding::GetIsAllowedToSinkOverflow() const {
	return mSinkOverflow;
}

void AKPCLNetworkConnectionBuilding::SetIsAllowedToSinkOverflow(bool IsAllowed) {
	if(HasAuthority()) {
		mSinkOverflow = IsAllowed;
	} else if(UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld())) {
		RCO->Server_SetSinkOverflowItem(this, IsAllowed);
	}
}
