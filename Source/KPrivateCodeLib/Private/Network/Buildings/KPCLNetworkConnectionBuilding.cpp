// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"

#include "FGCentralStorageSubsystem.h"
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
	mStateGatherTimer.mIsActive = true;
}

void AKPCLNetworkConnectionBuilding::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		UpdateInventoryState();
		mCentralStorageSubsystem = AFGCentralStorageSubsystem::Get(GetWorld());
	}
}

void AKPCLNetworkConnectionBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(AKPCLNetworkConnectionBuilding, mInformations);
}

bool AKPCLNetworkConnectionBuilding::CanProduce_Implementation() const
{
	if(!Super::CanProduce_Implementation()) return false;

	return CanUploadStorage();
}

void AKPCLNetworkConnectionBuilding::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mOverflowMode);
	FG_DOREPCONDITIONAL(ThisClass, mSpeedOverride);
}

void AKPCLNetworkConnectionBuilding::TickNetwork(float dt, FKPCLFaxitNetwork* Network)
{
	Super::TickNetwork(dt, Network);

	if(!IsProducing()) return;

	AKPCLNetworkCore* Core = Network->mCore;
	if(!Core) return;
}

void AKPCLNetworkConnectionBuilding::EndProductionTime()
{
	Super::EndProductionTime();

	UpdateProductionSpeed();
}

void AKPCLNetworkConnectionBuilding::onProducingFinal_Implementation()
{
	Super::onProducingFinal_Implementation();

	if(!GetInventory() || !mNetworkCore) return;

	if(mIsUpload)
	{
		FInventoryStack Stack;
		if(GetInventory()->GetStackFromIndex(1, Stack))
		{
			FKPCLFaxitNetworkStatData* Stat = mNetworkCore->GetState(Stack.Item.GetItemClass());
			mNetworkCore->TryToStoreItem(GetInventory(), Stack.Item.GetItemClass(), 1);
			Stat->mUpload += 1;
		}
		
		UploadToDepot();
		Sink();
	} else
	{
		mNetworkCore->TryToGrabItem(GetInventory(), GetFilterItem(), 1);
	}
}

void AKPCLNetworkConnectionBuilding::SetBelts()
{
	Super::SetBelts();

	UFGFactoryConnectionComponent* Component = mIsUpload ? GetConv(0, KPCLInput) : GetConv(0, KPCLOutput);
	if(Component)
	{
		Component->SetInventory(GetInventory());
		Component->SetInventoryAccessIndex(mOverflowMode == EKPCLOverflowMode::Ignore ? 0 : -1);
	}
}

void AKPCLNetworkConnectionBuilding::CollectAndPushPipes(float dt, bool IsPush)
{
	Super::CollectAndPushPipes(dt, IsPush);
	
	if(mItemForm != EResourceForm::RF_GAS) return;
	if(mItemForm != EResourceForm::RF_LIQUID) return;

	UFGPipeConnectionFactory* Component = mIsUpload ? GetPipe(0, KPCLInput) : GetPipe(0, KPCLOutput);
	if (mIsUpload && Component)
	{
		UKBFLCppInventoryHelper::PushPipe(GetInventory(), 0, dt, Component);
	} else if(!mIsUpload && Component)
	{
		UKBFLCppInventoryHelper::PullAllFromPipe(GetInventory(), 0, dt, Component);
	}
}

void AKPCLNetworkConnectionBuilding::Server_DoFlush()
{
	Super::Server_DoFlush();
	
	if(mItemForm != EResourceForm::RF_GAS) return;
	if(mItemForm != EResourceForm::RF_LIQUID) return;

	if(GetInventory())
	{
		GetInventory()->Empty();
	}
}

void AKPCLNetworkConnectionBuilding::UpdateInventoryState()
{
	if(!GetInventory()) return;

	if(mItemForm != EResourceForm::RF_SOLID)
	{
		GetInventory()->Resize(1);
		return;
	}
	
	GetInventory()->Resize(mOverflowMode == EKPCLOverflowMode::Ignore ? 1 : 2);
	SetBelts();
}

void AKPCLNetworkConnectionBuilding::UploadToDepot()
{
	if(!mCentralStorageSubsystem) return;
	if(mOverflowMode != EKPCLOverflowMode::Depot) return;
	if(mOverflowMode != EKPCLOverflowMode::DepotAndSink) return;
	
	FInventoryStack Stack;
	if(GetInventory()->GetStackFromIndex(2, Stack))
	{
		mCentralStorageSubsystem->UploadItemFromInventoryToCentralStorage(
			GetInventory(), 2, Stack.Item.GetItemClass()
		);
	}
}

void AKPCLNetworkConnectionBuilding::Sink()
{
	if(mOverflowMode != EKPCLOverflowMode::Sink) return;
	if(mOverflowMode != EKPCLOverflowMode::DepotAndSink) return;
	
	FInventoryStack Stack;
	if(GetInventory()->GetStackFromIndex(2, Stack))
	{
		FItemAmount Amount = FItemAmount(Stack.Item.GetItemClass(), 1);
		int32 Removed = SinkItems(Amount);
		if(Removed > 0)
		{
			GetInventory()->RemoveFromIndex(2, Removed);
		}
	}
}

void AKPCLNetworkConnectionBuilding::UpdateProductionSpeed()
{
	if(mSpeedOverride > 0.f)
	{
		mProductionHandle.SetNewTime(60 / mSpeedOverride);
	}

	if(IsValid(mFaxitSubsystem))
	{
		if(mItemForm == EResourceForm::RF_SOLID)
		{
			mProductionHandle.SetNewTime(60 / mFaxitSubsystem->GetItemsPerMinute());
		} else
		{
			mProductionHandle.SetNewTime(60 / (mFaxitSubsystem->GetFluidPerMinute() / 1000));
		}
	}
}

void AKPCLNetworkConnectionBuilding::OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                                      UFGInventoryComponent* sourceInventory)
{
	Super::OnInputItemAdded(itemClass, numRemoved, sourceInventory);
	UpdateInventoryFilter();
}

void AKPCLNetworkConnectionBuilding::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                                        UFGInventoryComponent* sourceInventory)
{
	Super::OnInputItemRemoved(itemClass, numRemoved, sourceInventory);
	UpdateInventoryFilter();
}

void AKPCLNetworkConnectionBuilding::UpdateInventoryFilter()
{
	if(!GetInventory()) return;

	TSubclassOf<UFGItemDescriptor> Filter = nullptr;
	if(GetStoredItemClass())
	{
		Filter = GetStoredItemClass();
	}
	
	if(GetFilterItem())
	{
		Filter = GetFilterItem();
	}

	for(int32 i = 0; i < GetInventory()->GetSizeLinear(); i++)
	{
		GetInventory()->SetAllowedItemOnIndex(i, Filter);
	}
}

bool AKPCLNetworkConnectionBuilding::CanUploadStorage() const
{
	if(!mNetworkCore) return false;
	TSubclassOf<UFGItemDescriptor> StoredItemClass = GetStoredItemClass();

	if(!StoredItemClass) return false;
	return !mNetworkCore->IsStorageFull(StoredItemClass);
}

bool AKPCLNetworkConnectionBuilding::StorageIsEmpty() const
{
	if(GetInventory())
	{
		return GetInventory()->IsEmpty();
	}
	return false;
}

void AKPCLNetworkConnectionBuilding::SetOverflowMode(EKPCLOverflowMode NewMode)
{
	if(!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if(IsValid(RCO))
		{
			RCO->Server_Faxit_SetOverflowType(this, NewMode);
		}
		return;
	}
	
	if(NewMode != mOverflowMode)
	{
		mOverflowMode = NewMode;
		UpdateInventoryState();
	}
}

void AKPCLNetworkConnectionBuilding::ClearSpeedOverride()
{
	if(!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if(IsValid(RCO))
		{
			RCO->Server_Faxit_ClearSpeedOverride(this);
		}
		return;
	}
	
	SetSpeedOverride(-1.f);
}

void AKPCLNetworkConnectionBuilding::SetSpeedOverride(float NewSpeed)
{
	if(!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if(IsValid(RCO))
		{
			RCO->Server_Faxit_SetSpeedOverride(this, NewSpeed);
		}
		return;
	}
	
	mSpeedOverride = NewSpeed;
	UpdateProductionSpeed();
}

EKPCLOverflowMode AKPCLNetworkConnectionBuilding::GetOverflowMode() const
{
	return mOverflowMode;
}

void AKPCLNetworkConnectionBuilding::SetFilterItem(TSubclassOf<UFGItemDescriptor> NewItem)
{
	if(!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if(IsValid(RCO))
		{
			RCO->Server_Faxit_SetFilterItem(this,NewItem);
		}
		return;
	}
	
	mFilterItem = NewItem;
	UpdateInventoryFilter();
}

TSubclassOf<UFGItemDescriptor> AKPCLNetworkConnectionBuilding::GetFilterItem() const
{
	return mFilterItem;
}

TSubclassOf<UFGItemDescriptor> AKPCLNetworkConnectionBuilding::GetStoredItemClass() const
{
	TArray<FInventoryStack> Stacks;
	GetInventory()->GetInventoryStacks(Stacks, false);
	if(Stacks.Num() > 0)
	{
		return Stacks[0].Item.GetItemClass();
	}
	return nullptr;
}
