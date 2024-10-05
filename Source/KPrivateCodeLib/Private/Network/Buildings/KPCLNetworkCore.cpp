// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkCore.h"

#include "KPCLNetworkConnectionComponent.h"
#include "KPCLNetworkDrive.h"
#include "KPCLNetworkInfoComponent.h"
#include "KPrivateCodeLibModule.h"

#include "BFL/KBFL_Inventory.h"
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"

#include "Net/UnrealNetwork.h"

#include "Network/KPCLNetwork.h"
#include "Registry/ModContentRegistry.h"
#include "Resources/FGItemDescriptor.h"
#include "Subsystem/KPCLUnlockSubsystem.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

#undef GetForm

class UKPCLNetworkDrive;

AKPCLNetworkCore::AKPCLNetworkCore()
{
	PrimaryActorTick.bCanEverTick = false;
	mDismantleAllChilds = true;

	mInputInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
	mOutputInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::OutputName);

	mInputInventory->SetDefaultSize(4);
	mOutputInventory->SetDefaultSize(5);

	mStateGatherTimer.mIsActive = true;
}

void AKPCLNetworkCore::TryConnectNetworks(AFGBuildable* OtherBuildable) const
{
	UE_LOG(LogKPCL, Log, TEXT("TryConnectNetwork Slave -> Core"))
	if (const AKPCLNetworkBuildingBase* Base = Cast<AKPCLNetworkBuildingBase>(OtherBuildable))
	{
		UKPCLNetworkConnectionComponent* MainCon = GetNetworkConnectionComponent();
		UKPCLNetworkConnectionComponent* SlaveCon = Base->GetNetworkConnectionComponent();
		if (ensure(MainCon && SlaveCon))
		{
			if (!MainCon->HasHiddenConnection(SlaveCon))
			{
				UE_LOG(LogKPCL, Log, TEXT("TryConnectNetwork SUCCESS!"))
				MainCon->AddHiddenConnection(SlaveCon);
			}
			else UE_LOG(LogKPCL, Log, TEXT("TryConnectNetwork allready exsists!"))
		}
	}
}

FKPCLFaxitNetwork AKPCLNetworkCore::GetNetworkData_Implementation() const
{
	if(mNetworkRef)
	{
		return *mNetworkRef;
	}

	return FKPCLFaxitNetwork();
}

bool AKPCLNetworkCore::HasCoreInNetwork_Implementation() const
{
	return IsValid(Execute_GetNetwork(this));
}

// Called when the game starts or when spawned
void AKPCLNetworkCore::BeginPlay()
{
	Super::BeginPlay();
	
	if(HasAuthority()) {
		mNetworkRef = mFaxitSubsystem->CreateOrAddNetworkNative(GetName(), this);
		UpdateStorageState();


		if(mNetworkRef)
		{
			for (AKPCLNetworkBuildingBase* Building : mNetworkRef->mNetworkBuildings)
			{
				Building->SetNetworkCore(this);
			}
		}
	}
}

void AKPCLNetworkCore::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (AKPCLFaxitSubsystem* Sub = AKPCLFaxitSubsystem::Get(GetWorld()))
		{
			Sub->DestoryNetwork(this);
		}
		mFaxitSubsystem->DestoryNetwork(this);
	}
}

void AKPCLNetworkCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLNetworkCore, mNetworkConnections);
}

void AKPCLNetworkCore::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);
	
	FG_DOREPCONDITIONAL(ThisClass, mStateBundels);
	FG_DOREPCONDITIONAL(ThisClass, mStorage);
	FG_DOREPCONDITIONAL(ThisClass, mNetworkPower);
	FG_DOREPCONDITIONAL(ThisClass, mMaxNetworkPower);
	FG_DOREPCONDITIONAL(ThisClass, mStackSizeMultiplier);
	FG_DOREPCONDITIONAL(ThisClass, mDrivePower);
}

void AKPCLNetworkCore::GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund,
                                                         bool noBuildCostEnabled) const
{
	if(noBuildCostEnabled)
	{
		return;
	}
	
	if (TSubclassOf<UFGRecipe> Recipe = GetBuiltWithRecipe())
	{
		for (FItemAmount ItemAmount : UFGRecipe::GetIngredients(Recipe))
		{
			if (ItemAmount.Amount > 0 && ItemAmount.ItemClass)
			{
				out_refund.Add(FInventoryStack(ItemAmount.Amount, ItemAmount.ItemClass));
			}
		}

		TArray<UFGPowerConnectionComponent*> PowerCons;
		GetComponents<UFGPowerConnectionComponent>(PowerCons);
		for (UFGPowerConnectionComponent* PowerCon : PowerCons)
		{
			if (ensure(PowerCon))
			{
				TArray<AFGBuildableWire*> Wires;
				PowerCon->GetWires(Wires);
				for (AFGBuildableWire* Wire : Wires)
				{
					if (ensure(Cast<UObject>(Wire)))
					{
						Execute_GetDismantleRefund(Cast<UObject>(Wire), out_refund, false);
					}
				}
			}
		}
	}
}

bool AKPCLNetworkCore::IsCore() const
{
	return true;
}

void AKPCLNetworkCore::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	TArray<UFGPowerInfoComponent*> PowerInfoComponents;
	GetComponents<UFGPowerInfoComponent>(PowerInfoComponents);
	for (UFGPowerInfoComponent* PowerInfoComponent : PowerInfoComponents)
	{
		if (UKPCLNetworkInfoComponent* NetworkInfoComponent = Cast<UKPCLNetworkInfoComponent>(PowerInfoComponent))
		{
			mNetworkInfoComponent = NetworkInfoComponent;
		}
		else
		{
			mPowerInfo = PowerInfoComponent;
		}
	}

	TArray<UFGPowerConnectionComponent*> PowerConnectionComponents;
	GetComponents<UFGPowerConnectionComponent>(PowerConnectionComponents);
	for (UFGPowerConnectionComponent* PowerConnectionComponent : PowerConnectionComponents)
	{
		if (UKPCLNetworkConnectionComponent* NetworkConnectionComponent = Cast<UKPCLNetworkConnectionComponent>(
			PowerConnectionComponent))
		{
			mNetworkConnection = NetworkConnectionComponent;
		}
	}
}

void AKPCLNetworkCore::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority())
	{
		if(mPlayerInventoryHandle.TickHandle(dt, CanConsumeFromPlayerInventory()))
		{
			OnConsumeFromPlayerInventory();
		}

		if(mItemFlushTimer.Tick(dt))
		{
			FlushOverflow();
		}
	}
}

bool AKPCLNetworkCore::CanProduce_Implementation() const
{
	return HasPower();
}

bool AKPCLNetworkCore::CanConsumeFromPlayerInventory() const
{
	FInventoryStack Stack;
	int32 Index;
	return IsProducing() && GetStackThatCanConsumeFromPlayerInventory(Stack, Index);
}

void AKPCLNetworkCore::OnConsumeFromPlayerInventory()
{
	FInventoryStack Stack;
	int32 Index;
	if(GetStackThatCanConsumeFromPlayerInventory(Stack, Index))
	{
		GetPlayerBufferInventory()->RemoveFromIndex(Index, 1);
	}
}

bool AKPCLNetworkCore::GetStackThatCanConsumeFromPlayerInventory(FInventoryStack& Stack, int32& Index) const
{
	return false;
}

FItemAmount AKPCLNetworkCore::GetItemOrCreateAmount(TSubclassOf<UFGItemDescriptor> Item)
{
	return *GetItemAmountRef(Item);
}

FItemAmount* AKPCLNetworkCore::GetItemAmountRef(TSubclassOf<UFGItemDescriptor> Item)
{
	FItemAmount* foundItemAmount = mStorage.FindByPredicate( [Item](const FItemAmount& itemAmount)
		{
			return itemAmount.ItemClass == Item;
		} );

	if(!foundItemAmount)
	{
		mStorage.Add(FItemAmount(Item, 0));
		return mStorage.FindByPredicate( [Item](const FItemAmount& itemAmount)
		{
			return itemAmount.ItemClass == Item;
		} );
	}
	
	return foundItemAmount;
}

int32 AKPCLNetworkCore::GetMaxItemAmount(TSubclassOf<UFGItemDescriptor> Item) const
{
	return UFGItemDescriptor::GetStackSize(Item) * mStackSizeMultiplier;
}

TArray<FItemAmount> AKPCLNetworkCore::GetItemAmounts() const
{
	return mStorage;
}

void AKPCLNetworkCore::GrabFromNetwork(AFGCharacterPlayer* Player, FItemAmount Amount)
{
	if(!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if(IsValid(RCO))
		{
			RCO->Server_Faxit_GrabFromNetwork(this, Player, Amount);
		}
		return;
	}
	
	if(IsValid(Player) && IsValid(Player->GetInventory()))
	{
		TryToGrabItem(Player->GetInventory(), Amount.ItemClass, Amount.Amount);
	}
}

int32 AKPCLNetworkCore::IsStorageFull(TSubclassOf<UFGItemDescriptor> Item)
{
	FItemAmount* ItemAmount = GetItemAmountRef(Item);
	int32 MaxAmount = GetMaxItemAmount(Item);

	return ItemAmount->Amount >= MaxAmount;
}

int32 AKPCLNetworkCore::IsStorageFull(FItemAmount* ItemAmount)
{
	int32 MaxAmount = GetMaxItemAmount(ItemAmount->ItemClass);

	return ItemAmount->Amount >= MaxAmount;
}

int32 AKPCLNetworkCore::IsStorageEmpty(TSubclassOf<UFGItemDescriptor> Item)
{
	FItemAmount* ItemAmount = GetItemAmountRef(Item);
	return ItemAmount->Amount <= 0;
}

int32 AKPCLNetworkCore::IsStorageEmpty(FItemAmount* ItemAmount)
{
	return ItemAmount->Amount <= 0;
}

int32 AKPCLNetworkCore::TryToStoreItem(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
{
	FInventoryStack Stack;
	int32 InventoryAmount = Inventory->GetNumItems(Item);

	int32 AmountToStore = FMath::Min(Amount, InventoryAmount);
	if(AmountToStore <= 0) return 0;

	int32 StoredAmount = TryToStoreItemAmount(Item, AmountToStore);
	if(StoredAmount <= 0) return 0;

	Inventory->Remove(Item, StoredAmount);
	return StoredAmount;
}

int32 AKPCLNetworkCore::TryToStoreItemAmount(TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
{
	FItemAmount* ItemAmount = GetItemAmountRef(Item);
	int32 MaxAmount = GetMaxItemAmount(Item);

	if(IsStorageFull(ItemAmount)) return 0;

	int32 StoredAmount = FMath::Min(Amount, MaxAmount - ItemAmount->Amount);

	ItemAmount->Amount += StoredAmount;

	NotifyStorageChange();
	return StoredAmount;
}

int32 AKPCLNetworkCore::TryToGrabItem(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
{
	FItemAmount* ItemAmount = GetItemAmountRef(Item);
	if(ItemAmount->Amount <= 0) return 0;

	int32 MaxGrabAmount = FMath::Min(Amount, ItemAmount->Amount);

	FInventoryStack Stack = FInventoryStack(MaxGrabAmount, Item);
	int AddedAmount = Inventory->AddStack(Stack, true);
	ItemAmount->Amount -= MaxGrabAmount;

	NotifyStorageChange();
	return AddedAmount;
}

int32 AKPCLNetworkCore::TryToGrabItemAmount(TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
{
	FItemAmount* ItemAmount = GetItemAmountRef(Item);
	if(ItemAmount->Amount <= 0) return 0;

	int32 MaxGrabAmount = FMath::Min(Amount, ItemAmount->Amount);
	ItemAmount->Amount -= MaxGrabAmount;

	NotifyStorageChange();
	return Amount;
}

UFGInventoryComponent* AKPCLNetworkCore::GetPlayerBufferInventory() const
{
	return GetOutputInventory();
}

TArray<FKPCLFaxitNetworkStatDataBundle> AKPCLNetworkCore::GetStateBundles() const
{
	return mStateBundels;
}

void AKPCLNetworkCore::GatherStates()
{
	Super::GatherStates();

	if(mNetworkRef)
	{
		FKPCLFaxitNetworkStatDataBundle Bundle;
		for (AKPCLNetworkBuildingBase* NetworkBuilding : mNetworkRef->mNetworkBuildings)
		{
			for (FKPCLFaxitNetworkStatData State : NetworkBuilding->GetStates())
			{
				FKPCLFaxitNetworkStatData* FoundState = Bundle.mStats.FindByPredicate( [State](const FKPCLFaxitNetworkStatData& item)
					{
						return item.mItem == State.mItem;
					} );

				if(!FoundState)
				{
					Bundle.mStats.Add(State);
				} else
				{
					FoundState->Merge(State, false);
				}
			}
		}

		Bundle.mTimestamp = FDateTime::Now().ToUnixTimestamp();
		mStateBundels.Add(Bundle);
		mStateBundels.Sort([](const FKPCLFaxitNetworkStatDataBundle& a, const FKPCLFaxitNetworkStatDataBundle& b)
					{
						return a.mTimestamp > b.mTimestamp;
					});
		
		while (mStateBundels.Num() > 60)
		{
			mStateBundels.RemoveAt(1);
		}
	}
}

void AKPCLNetworkCore::TickNetwork(float dt, FKPCLFaxitNetwork* Network)
{
	if(IsProducing() && Network)
	{
		for (AKPCLNetworkBuildingBase* Building : Network->mNetworkBuildings)
		{
			Building->TickNetwork(dt, Network);		
		}
	}
}

bool AKPCLNetworkCore::FormFilterOutputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const
{
	if (IsValid(object))
	{
		return UFGItemDescriptor::GetForm(object) == EResourceForm::RF_SOLID;
	}
	return false;
}

bool AKPCLNetworkCore::FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const
{
	if (IsValid(object))
	{
		if (const TSubclassOf<UKPCLNetworkDrive> Drive{object})
		{
			return true;
		}
	}
	return false;
}

void AKPCLNetworkCore::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                          UFGInventoryComponent* sourceInventory)
{
	Super::OnInputItemRemoved(itemClass, numRemoved, sourceInventory);
	UpdateStorageState();
}

void AKPCLNetworkCore::OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                        UFGInventoryComponent* sourceInventory)
{
	Super::OnInputItemAdded(itemClass, numRemoved, sourceInventory);
	UpdateStorageState();
}

void AKPCLNetworkCore::UpdateStorageState()
{
	if(!IsValid(GetInventory()))
	{
		return;
	}

	for(int32 i = 0; i < GetInventory()->GetSizeLinear(); i++)
	{
		GetInventory()->AddArbitrarySlotSize(i,1);
	}
	
	TArray<FInventoryStack> Stacks;
	GetInventory()->GetInventoryStacks(Stacks, false);

	float NewDrivePower = 0.f;
	int32 NewMultiplier = 1;
	for (FInventoryStack Stack : Stacks)
	{
		if (const TSubclassOf<UKPCLNetworkDrive> Drive{Stack.Item.GetItemClass()})
		{
			NewMultiplier += UKPCLNetworkDrive::GetMultiplier(Drive);
			NewDrivePower += UKPCLNetworkDrive::GetPowerConsume(Drive);
		}
	}

	mStackSizeMultiplier = FMath::Max(mStackSizeMultiplier, 1);
	mDrivePower = NewDrivePower;
	CheckStorageState();
}

void AKPCLNetworkCore::CheckStorageState()
{
	bool NewFlushState = false;
	for (FItemAmount Storage : mStorage)
	{
		if(Storage.Amount <= 0) continue;
		int32 MaxAmount = GetMaxItemAmount(Storage.ItemClass);
		if(Storage.Amount > MaxAmount)
		{
			NewFlushState = true;
			break;
		}
	}

	if(NewFlushState != mItemFlushTimer.mIsActive)
	{
		mItemFlushTimer.mIsActive = NewFlushState;
		mItemFlushTimer.Reset();
	}
}

void AKPCLNetworkCore::FlushOverflow()
{
	for (FItemAmount& Storage : mStorage)
	{
		if(Storage.Amount <= 0) continue;
		int32 MaxAmount = GetMaxItemAmount(Storage.ItemClass);
		Storage.Amount = FMath::Min(Storage.Amount, MaxAmount);
	}
}

void AKPCLNetworkCore::NotifyStorageChange()
{
	OnStorageChanged.Broadcast();
}

void AKPCLNetworkCore::HandlePower(float dt)
{
	mPowerOptions.bHasPower = HasPower();
	mPowerOptions.StructureTick(dt, IsProducing());
	GetNetworkInfoComponent()->SetTargetConsumption(mPowerOptions.GetPowerConsume() + mDrivePower);
	GetNetworkInfoComponent()->SetMaximumTargetConsumption(mPowerOptions.GetMaxPowerConsume() + mDrivePower);

	GetNetworkInfoComponent()->SetBaseProduction(50000000.f);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (IsValid(Network))
	{
		FPowerCircuitStats Stats = FPowerCircuitStats();
		Network->GetStats(Stats);
		
		mNetworkPower = FMath::Max(Stats.PowerConsumed, 0.1f);
		mMaxNetworkPower = FMath::Max(Stats.MaximumPowerConsumption, 0.1f);
		
		GetPowerInfo()->SetTargetConsumption(IsProducing() ? mNetworkPower : 0.1f);
		GetPowerInfo()->SetMaximumTargetConsumption(IsProducing()
			                                            ? mMaxNetworkPower
			                                            : 0.1f);
	}
}
