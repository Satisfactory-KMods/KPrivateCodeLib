// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkCore.h"

#include "FGResourceSinkSubsystem.h"
#include "KPrivateCodeLibModule.h"
#include "Logging.h"

#include "BFL/KBFL_Inventory.h"
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "C++/KBFLCppInventoryHelper.h"
#include "Components/KPCLNetworkPlayerComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "Net/UnrealNetwork.h"

#include "Network/KPCLNetwork.h"
#include "Network/Buildings/KPCLNetworkCoreModule.h"
#include "Registry/ModContentRegistry.h"
#include "Resources/FGAnyUndefinedDescriptor.h"
#include "Resources/FGItemDescriptor.h"
#include "Resources/FGNoneDescriptor.h"
#include "Resources/FGOverflowDescriptor.h"
#include "Resources/FGWildCardDescriptor.h"
#include "Subsystem/KPCLUnlockSubsystem.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

#undef GetForm

AKPCLNetworkCore::AKPCLNetworkCore()
{
	PrimaryActorTick.bCanEverTick = false;
	mDismantleAllChilds = true;

	mInventoryDatas.Add(FKPCLInventoryStructure("Inv_Output"));
	mInventoryDatas.Add(FKPCLInventoryStructure("Inv_Fluid"));

	mInventoryDatas[0].mDontResizeOnBeginPlay = true;
	mInventoryDatas[1].mInventorySize = 30;
	mInventoryDatas[2].mInventorySize = 2;

	mForceNetUpdateOnRegisterPlayer = 1;
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

void AKPCLNetworkCore::onProducingFinal_Implementation()
{
	Super::onProducingFinal_Implementation();
	GetFluidBufferInventory()->RemoveFromIndex(0, GetCurrentInputAmount());
}

void AKPCLNetworkCore::CollectAndPushPipes(float dt, bool IsPush)
{
	Super::CollectAndPushPipes(dt, IsPush);

	if (IsPush)
	{
		UKBFLCppInventoryHelper::PushPipe(GetFluidBufferInventory(), 1, dt, GetPipe(0, KPCLOutput));
		return;
	}

	UKBFLCppInventoryHelper::PullPipe(GetFluidBufferInventory(), 0, dt, mInputConsume.ItemClass, GetPipe(0, KPCLInput));
}

void AKPCLNetworkCore::OnTierUpdated()
{
	Super::OnTierUpdated();
	if (HasAuthority())
	{
		UpdateNetworkMax();
	}
}

// Called when the game starts or when spawned
void AKPCLNetworkCore::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (AKPCLUnlockSubsystem* Sub = AKPCLUnlockSubsystem::Get(GetWorld()))
		{
			Sub->OnNexusConstruct(this);
		}

		ConfigureCoreInventory();

		GetFluidBufferInventory()->AddArbitrarySlotSize(0, FMath::Max(mMaxConsumeAmount * 2, 50000));
		GetFluidBufferInventory()->SetAllowedItemOnIndex(0, mInputConsume.ItemClass);

		FInventoryStack Stack;
		if (GetFluidBufferInventory()->GetStackFromIndex(0, Stack))
		{
			if (Stack.Item.GetItemClass() != mInputConsume.ItemClass)
			{
				GetFluidBufferInventory()->RemoveAllFromIndex(0);
			}
		}

		if (GetNetworkInfoComponent() && GetNetworkConnectionComponent())
		{
			GetNetworkConnectionComponent()->SetPowerInfo(GetNetworkInfoComponent());
		}

		if (GetPowerConnectionExplicit() && GetPowerInfoExplicit())
		{
			GetPowerConnectionExplicit()->SetPowerInfo(GetPowerInfoExplicit());
		}

		UpdateNetworkMax();
	}
}

void AKPCLNetworkCore::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (AKPCLUnlockSubsystem* Sub = AKPCLUnlockSubsystem::Get(GetWorld()))
		{
			Sub->OnNexusDeconstruct(this);
		}
	}
}

void AKPCLNetworkCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLNetworkCore, mNetworkConnections);
	DOREPLIFETIME(AKPCLNetworkCore, mSlotMappingAll);
	DOREPLIFETIME(AKPCLNetworkCore, mTotalFluidBytes);
	DOREPLIFETIME(AKPCLNetworkCore, mTotalSolidBytes);
	DOREPLIFETIME(AKPCLNetworkCore, mUsedFluidBytes);
	DOREPLIFETIME(AKPCLNetworkCore, mUsedSolidBytes);
	DOREPLIFETIME(AKPCLNetworkCore, mItemCountMap);
}

void AKPCLNetworkCore::GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund,
                                                         bool noBuildCostEnabled) const
{
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

void AKPCLNetworkCore::OnPlayerItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved)
{
}

void AKPCLNetworkCore::OnPlayerItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved)
{
}

void AKPCLNetworkCore::GetCoreData(FCoreDataSortOptionStruc SortOption, TArray<FCoreInventoryData>& Data)
{
	Data.Empty();
	TArray<FCoreInventoryData> CoreData;
	switch (SortOption.mShow)
	{
	case ECoreDataShowOption::fluid:
		for (FCoreInventoryData SlotMapping : mSlotMappingAll)
		{
			if (SlotMapping.mItemForm == EResourceForm::RF_GAS || SlotMapping.mItemForm == EResourceForm::RF_LIQUID)
			{
				CoreData.Add(SlotMapping);
			}
		}
		break;
	case ECoreDataShowOption::solid:
		for (FCoreInventoryData SlotMapping : mSlotMappingAll)
		{
			if (SlotMapping.mItemForm == EResourceForm::RF_SOLID)
			{
				CoreData.Add(SlotMapping);
			}
		}
		break;
	default:
		CoreData = mSlotMappingAll;
	}


	AFGResourceSinkSubsystem* SinkSubsystem = AFGResourceSinkSubsystem::Get(GetWorld());
	auto RM = AFGRecipeManager::Get(GetWorld());
	for (FCoreInventoryData InventoryData : CoreData)
	{
		if (ensure(InventoryData.mItem))
		{
			if (SortOption.mSinkableFilter)
			{
				if (SinkSubsystem->GetResourceSinkPointsForItem(InventoryData.mItem) <= 0)
				{
					continue;
				}
			}

			if (!SortOption.mShowEmptySlots && ensure(GetInventory()))
			{
				if (GetInventory()->IsIndexEmpty(InventoryData.mInventoryIndex))
				{
					continue;
				}
			}

			bool HasRecipe = true;
			if (SortOption.mFilteredUnlocked && GetInventory()->IsIndexEmpty(InventoryData.mInventoryIndex))
			{
				TArray<TSubclassOf<UFGRecipe>> RecipeArray = RM->FindRecipesByIngredient(InventoryData.mItem);
				if (RecipeArray.Num() <= 0)
				{
					RecipeArray = RM->FindRecipesByProduct(InventoryData.mItem);
				}
				HasRecipe = RecipeArray.Num() > 0;
			}

			if (SortOption.mFilteredUnlocked && HasRecipe && SortOption.mNameFilter.IsEmpty())
			{
				Data.Add(InventoryData);
			}
			else if ((SortOption.mFilteredUnlocked && HasRecipe && !SortOption.mNameFilter.IsEmpty()) || !SortOption.
				mFilteredUnlocked)
			{
				if ((InventoryData.mItem->GetName().Contains(*SortOption.mNameFilter) ||
					UFGItemDescriptor::GetAbbreviatedDisplayName(InventoryData.mItem).ToString().
					Contains(*SortOption.mNameFilter) || UFGItemDescriptor::GetItemName(InventoryData.mItem).ToString().
					Contains(*SortOption.mNameFilter)) && HasRecipe)
				{
					Data.Add(InventoryData);
				}
			}
		}
	}

	CoreData.Empty();

	Data.Sort([&](const FCoreInventoryData& A, const FCoreInventoryData& B)
	{
		switch (SortOption.mSorting)
		{
		case ECoreDataSortOption::alphab:
			return UFGItemDescriptor::GetItemName(A.mItem).ToString() < UFGItemDescriptor::GetItemName(B.mItem).
				ToString();

		case ECoreDataSortOption::revalphab:
			return UFGItemDescriptor::GetItemName(A.mItem).ToString() > UFGItemDescriptor::GetItemName(B.mItem).
				ToString();

		case ECoreDataSortOption::form:
			return UFGItemDescriptor::GetForm(A.mItem) < UFGItemDescriptor::GetForm(B.mItem);

		case ECoreDataSortOption::revform:
			return UFGItemDescriptor::GetForm(A.mItem) > UFGItemDescriptor::GetForm(B.mItem);

		case ECoreDataSortOption::formalphab:
			return UFGItemDescriptor::GetItemName(A.mItem).ToString() < UFGItemDescriptor::GetItemName(B.mItem).
				ToString() && UFGItemDescriptor::GetForm(A.mItem) < UFGItemDescriptor::GetForm(B.mItem);

		case ECoreDataSortOption::revformalphab:
			return UFGItemDescriptor::GetItemName(A.mItem).ToString() > UFGItemDescriptor::GetItemName(B.mItem).
				ToString() && UFGItemDescriptor::GetForm(A.mItem) > UFGItemDescriptor::GetForm(B.mItem);

		case ECoreDataSortOption::revindex:
			return A.mInventoryIndex > B.mInventoryIndex;

		default:
			return A.mInventoryIndex < B.mInventoryIndex;
		}
	});
}

void AKPCLNetworkCore::GetTotalBytes(float& Fluid, float& Solid) const
{
	Fluid = FMath::Max(0.0f, mTotalFluidBytes);
	Solid = FMath::Max(0.0f, mTotalSolidBytes);
}

void AKPCLNetworkCore::GetUsedBytes(float& Fluid, float& Solid) const
{
	Fluid = FMath::Max(0.0f, mUsedFluidBytes);
	Solid = FMath::Max(0.0f, mUsedSolidBytes);
}

void AKPCLNetworkCore::GetFreeBytes(float& Fluid, float& Solid) const
{
	Fluid = FMath::Max(mTotalFluidBytes - mUsedFluidBytes, 0.0f);
	Solid = FMath::Max(mTotalSolidBytes - mUsedSolidBytes, 0.0f);
}

void AKPCLNetworkCore::GetFreeBytesPrt(float& Fluid, float& Solid) const
{
	float FreeFluid;
	float FreeSolid;
	float MaxFluid;
	float MaxSolid;

	GetUsedBytes(FreeFluid, FreeSolid);
	GetTotalBytes(MaxFluid, MaxSolid);

	Fluid = UKismetMathLibrary::SafeDivide(FreeFluid, MaxFluid);
	Solid = UKismetMathLibrary::SafeDivide(FreeSolid, MaxSolid);
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

int32 AKPCLNetworkCore::GetAllowedIndex(TSubclassOf<UFGItemDescriptor> Item) const
{
	if (GetInventory())
	{
		for (int32 i = 0; i < GetInventory()->GetSizeLinear(); ++i)
		{
			if (GetInventory()->GetAllowedItemOnIndex(i) == Item)
			{
				return i;
			}
		}
	}

	return INDEX_NONE;
}

float AKPCLNetworkCore::GetBytesForItemClass(TSubclassOf<UFGItemDescriptor> itemClass, float Num)
{
	if (ensureAlways(itemClass) && Num > 0)
	{
		const EResourceForm Form = UFGItemDescriptor::GetForm(itemClass);

		float ByteSize;
		if (Form == EResourceForm::RF_GAS || Form == EResourceForm::RF_LIQUID)
		{
			ByteSize = Num / mFluidItemsPerBytes;
		}
		else
		{
			ByteSize = Num / mSolidItemsPerBytes;
		}

		if (ByteSize > 0)
		{
			return ByteSize;
		}
	}
	return -1.f;
}

int32 AKPCLNetworkCore::GetMaxItemsByBytes(TSubclassOf<UFGItemDescriptor> itemClass, float FluidBytes, float SolidBytes)
{
	if (ensureAlways(itemClass) && (FluidBytes > 0.f || SolidBytes > 0.f))
	{
		const EResourceForm Form = UFGItemDescriptor::GetForm(itemClass);

		int32 MaxItems;
		if (Form == EResourceForm::RF_GAS || Form == EResourceForm::RF_LIQUID)
		{
			MaxItems = FMath::Floor(FluidBytes * mFluidItemsPerBytes);
		}
		else
		{
			MaxItems = FMath::Floor(SolidBytes * mSolidItemsPerBytes);
		}

		if (MaxItems > 0)
		{
			return MaxItems;
		}
	}
	return 0;
}

float AKPCLNetworkCore::GetBytesForItemAmount(FItemAmount Amount, bool& IsFluid)
{
	if (ensureAlways(Amount.ItemClass) && Amount.Amount > 0)
	{
		const EResourceForm Form = UFGItemDescriptor::GetForm(Amount.ItemClass);

		if (Form == EResourceForm::RF_GAS || Form == EResourceForm::RF_LIQUID)
		{
			IsFluid = true;
			return 1 / mFluidItemsPerBytes * Amount.Amount;
		}

		IsFluid = false;
		return 1 / mSolidItemsPerBytes * Amount.Amount;
	}
	return 0.f;
}

void AKPCLNetworkCore::Core_SetMaxItemCount(TSubclassOf<UFGItemDescriptor> Item, int32 Max)
{
	if (!IsValid(Item))
	{
		return;
	}

	if (HasAuthority())
	{
		if (Max > 0)
		{
			UE_LOG(LogKLIB, Warning, TEXT("Removed Item Limit: %d"), mItemCountMap.Remove(FKPCLNetworkMaxData(Item, 0)))
			UE_LOG(LogKLIB, Warning, TEXT("Add Item Limit: %d"), mItemCountMap.Add(FKPCLNetworkMaxData(Item, Max)))
		}
		else
		{
			UE_LOG(LogKLIB, Warning, TEXT("Removed Item Limit: %d"), mItemCountMap.Remove(FKPCLNetworkMaxData(Item, 0)))
		}
	}
	else
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld()))
		{
			RCO->Server_Core_SetMaxItemCount(this, Item, Max);
		}
	}
}

bool AKPCLNetworkCore::GetStackFromNetwork(TSubclassOf<UFGItemDescriptor> Item, FInventoryStack& Stack, int32& Index)
{
	if (GetInventory())
	{
		if (FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(Item))
		{
			Index = CoreData->mInventoryIndex;
			return GetInventory()->GetStackFromIndex(Index, Stack);
		}
	}

	Stack = FInventoryStack();
	Index = -1;
	return false;
}

FKPCLNetworkMaxData AKPCLNetworkCore::Core_GetMaxItemCount(TSubclassOf<UFGItemDescriptor> Item)
{
	if (const FKPCLNetworkMaxData* Data = mItemCountMap.FindByKey(FKPCLNetworkMaxData(Item, 0)))
	{
		return *Data;
	}
	return FKPCLNetworkMaxData();
}

int32 AKPCLNetworkCore::GetCurrentInputAmount() const
{
	const float TotalBytes = (mTotalSolidBytes + mTotalFluidBytes) / mByteCalcDiv;
	return FMath::Max(FMath::Min(mMaxProduceAmount, FMath::TruncToInt(mInputConsume.Amount * TotalBytes)),
	                  mInputConsume.Amount);
}

void AKPCLNetworkCore::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority())
	{
		if (IsProducing())
		{
			CheckNetwork();
			TickPlayerBufferInventory(dt);

			if (mNetworkConnections.Num() > 0)
			{
				const int32 NumPerGroup = FMath::Max(FMath::DivideAndRoundUp(mNetworkConnections.Num(), 8), 1);
				ParallelFor(8, [&](int32 Index)
				{
					TArray<AKPCLNetworkConnectionBuilding*> Buildings;
					for (int32 Member = Index * NumPerGroup; Member < FMath::Min(
						     (Index + 1) * NumPerGroup, mNetworkConnections.Num()); Member++)
					{
						if (ensure(mNetworkConnections[Member]))
						{
							PullBuilding(mNetworkConnections[Member], dt);
						}
					}
				});
			}

			if (mNetworkManuConnections.Num() > 0)
			{
				const int32 NumPerGroup = FMath::Max(FMath::DivideAndRoundUp(mNetworkManuConnections.Num(), 8), 1);
				ParallelFor(8, [&](int32 Index)
				{
					TArray<AKPCLNetworkConnectionBuilding*> Buildings;
					for (int32 Member = Index * NumPerGroup; Member < FMath::Min(
						     (Index + 1) * NumPerGroup, mNetworkManuConnections.Num()); Member++)
					{
						if (ensure(mNetworkManuConnections[Member]))
						{
							HandleManuConnections(mNetworkManuConnections[Member], dt);
						}
					}
				});
			}

			TickPlayerNetworkInventory(dt);
		}
	}
}

bool AKPCLNetworkCore::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect())
	{
		return false;
	}

	UKPCLNetwork* Network = nullptr;
	if (GetNetworkInfoComponent() && GetNetworkInfoComponent()->IsConnected())
	{
		Network = Cast<UKPCLNetwork>(GetNetworkInfoComponent()->GetPowerCircuit());
	}

	if (!GetPowerInfo() || !Network)
	{
		return false;
	}

	if (GetPowerInfo()->HasPower() && GetFluidBufferInventory())
	{
		return GetFluidBufferInventory()->HasItems(mInputConsume.ItemClass, GetCurrentInputAmount());
	}

	return false;
}

void AKPCLNetworkCore::TickPlayerNetworkInventory(float dt)
{
	while (!mNetworkPlayerComponentsThisFrame.IsEmpty())
	{
		UKPCLNetworkPlayerComponent* Comp;
		mNetworkPlayerComponentsThisFrame.Dequeue(Comp);

		if (IsValid(Comp))
		{
			Comp->ReceiveTickFromNetwork(this);
		}
	}
}

void AKPCLNetworkCore::TickPlayerBufferInventory(float dt)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!GetPlayerBufferInventory() || !GetInventory())
	{
		return;
	}

	if (GetPlayerBufferInventory()->IsEmpty())
	{
		return;
	}

	float Solid;
	float Fluid;
	GetFreeBytes(Fluid, Solid);

	if (Solid > 0.0f || Fluid > 0.0f)
	{
		const int32 BufferIndex = GetPlayerBufferInventory()->GetFirstIndexWithItem();
		FInventoryStack Stack;
		if (GetPlayerBufferInventory()->GetStackFromIndex(BufferIndex, Stack))
		{
			if (FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(Stack.Item.GetItemClass()))
			{
				int32 ItemIdx = CoreData->mInventoryIndex;

				const int32 MaxAmount = GetMaxItemsByBytes(Stack.Item.GetItemClass(), Fluid, Solid);
				Stack.NumItems = FMath::Min(MaxAmount, Stack.NumItems);

				if (Stack.HasItems())
				{
					GetPlayerBufferInventory()->RemoveFromIndex(BufferIndex,
					                                            GetInventory()->AddStackToIndex(ItemIdx, Stack, true));
				}
			}
		}
	}
}

void AKPCLNetworkCore::ReGroupSlaves()
{
	// Todo: Testing?
}

UFGInventoryComponent* AKPCLNetworkCore::GetPlayerBufferInventory() const
{
	return GetInventoryFromIndex(1);
}

UFGInventoryComponent* AKPCLNetworkCore::GetFluidBufferInventory() const
{
	return GetInventoryFromIndex(2);
}

bool AKPCLNetworkCore::CanRemoveFromCore(const TArray<FItemAmount>& Amounts) const
{
	if (GetInventory())
	{
		for (FItemAmount Amount : Amounts)
		{
			if (const FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(Amount.ItemClass))
			{
				FInventoryStack Stack;
				GetInventory()->GetStackFromIndex(CoreData->mInventoryIndex, Stack);
				if (Stack.NumItems < Amount.Amount || !Stack.HasItems())
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

int32 AKPCLNetworkCore::GetAmountFromItemClass(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	if (IsValid(GetInventory()) && IsValid(ItemClass))
	{
		if (const FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(ItemClass))
		{
			FInventoryStack Stack;
			GetInventory()->GetStackFromIndex(CoreData->mInventoryIndex, Stack);
			if (Stack.HasItems())
			{
				return Stack.NumItems;
			}
		}
	}

	return 0;
}

bool AKPCLNetworkCore::RemoveFromCore(const TArray<FItemAmount>& Amounts)
{
	if (!HasAuthority())
	{
		UE_LOG(LogKPCL, Error, TEXT("Try to call AKPCLNetworkCore::RemoveFromCore witout Authority!"))
		return false;
	}

	if (CanRemoveFromCore(Amounts))
	{
		for (FItemAmount Amount : Amounts)
		{
			FKPCLItemTransferQueue Queue;

			Queue.mAmount = Amount;
			Queue.mAddAmount = false;
			Queue.mInventoryIndex = GetIndexFromItem(Amount.ItemClass);

			mInventoryQueue.Enqueue(Queue);
		}
		return true;
	}
	return false;
}

int32 AKPCLNetworkCore::GetIndexFromItem(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	if (!IsValid(ItemClass))
	{
		return INDEX_NONE;
	}

	if (const FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(ItemClass))
	{
		return CoreData->mInventoryIndex;
	}
	return INDEX_NONE;
}

void AKPCLNetworkCore::ReconfigureInventory()
{
	Super::ReconfigureInventory();

	if (GetPlayerBufferInventory())
	{
		GetPlayerBufferInventory()->OnItemAddedDelegate.AddUniqueDynamic(this, &AKPCLNetworkCore::OnPlayerItemAdded);
		GetPlayerBufferInventory()->OnItemAddedDelegate.AddUniqueDynamic(this, &AKPCLNetworkCore::OnPlayerItemAdded);

		if (!GetPlayerBufferInventory()->mItemFilter.IsBoundToObject(this))
		{
			GetPlayerBufferInventory()->mItemFilter.BindUObject(this, &AKPCLNetworkCore::FilterPlayerInventory);
		}
		if (!GetPlayerBufferInventory()->mFormFilter.IsBoundToObject(this))
		{
			GetPlayerBufferInventory()->mFormFilter.BindUObject(this, &AKPCLNetworkCore::FormFilterPlayerInventory);
		}
	}
}

bool AKPCLNetworkCore::FilterPlayerInventory(TSubclassOf<UObject> object, int32 idx) const
{
	return true;
}

bool AKPCLNetworkCore::FormFilterPlayerInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const
{
	if (IsValid(object))
	{
		return UFGItemDescriptor::GetForm(object) == EResourceForm::RF_SOLID;
	}

	return false;
}

void AKPCLNetworkCore::HandlePower(float dt)
{
	mPowerOptions.bHasPower = HasPower();
	mPowerOptions.StructureTick(dt, IsProducing());
	GetNetworkInfoComponent()->SetTargetConsumption(mPowerOptions.GetPowerConsume());
	GetNetworkInfoComponent()->SetMaximumTargetConsumption(mPowerOptions.GetMaxPowerConsume());

	GetNetworkInfoComponent()->SetBaseProduction(50000000.f);
	GetNetworkInfoComponent()->SetBytes(0);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (IsValid(Network))
	{
		FPowerCircuitStats Stats = FPowerCircuitStats();
		Network->GetStats(Stats);
		GetPowerInfo()->SetTargetConsumption(IsProducing() ? FMath::Max(Stats.PowerConsumed, 0.1f) : 0.1f);
		GetPowerInfo()->SetMaximumTargetConsumption(IsProducing()
			                                            ? FMath::Max(Stats.MaximumPowerConsumption, 0.1f)
			                                            : 0.1f);
	}
}

void AKPCLNetworkCore::ConfigureCoreInventory()
{
	auto AssetSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());
	TArray<TSubclassOf<UFGItemDescriptor>> Items;

	for (auto Item : AssetSubsystem->GetAllItems())
	{
		if (UFGItemDescriptor::GetForm(Item) != EResourceForm::RF_INVALID && UFGItemDescriptor::GetForm(Item) !=
			EResourceForm::RF_HEAT && !Item->IsChildOf(UFGBuildDescriptor::StaticClass()) && !Item->
			IsChildOf(UFGFactoryCustomizationDescriptor::StaticClass()) && !UFGItemDescriptor::GetItemName(Item).
			IsEmpty() && !Item->IsChildOf(UFGNoneDescriptor::StaticClass()) && !Item->
			IsChildOf(UFGAnyUndefinedDescriptor::StaticClass()) && !Item->
			IsChildOf(UFGOverflowDescriptor::StaticClass()) && !Item->IsChildOf(
				UFGWildCardDescriptor::StaticClass()))
		{
			Items.Add(Item);
		}
	}

	if (Items.Num() > 0)
	{
		TArray<FInventoryStack> Stacks = GetInventory()->mInventoryStacks;
		GetInventory()->Empty();
		GetInventory()->Resize(Items.Num());

		for (int i = 0; i < Items.Num(); ++i)
		{
			if (Items.IsValidIndex(i))
			{
				TSubclassOf<UFGItemDescriptor> Item = Items[i];
				if (ensure(Item))
				{
					EResourceForm Form = UFGItemDescriptor::GetForm(Item);
					FCoreInventoryData Data = FCoreInventoryData(Item, i);
					Data.mItemForm = Form == EResourceForm::RF_SOLID
						                 ? EResourceForm::RF_SOLID
						                 : EResourceForm::RF_LIQUID;

					mSlotMappingAll.Add(Data);
					UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), i, Item);
					GetInventory()->AddArbitrarySlotSize(i, INT32_MAX);
				}
			}
			else
			{
				UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), i,
				                                                      UFGNoneDescriptor::StaticClass());
			}
		}

		for (FInventoryStack Stack : Stacks)
		{
			if (Stack.HasItems())
			{
				GetInventory()->AddStack(Stack, true);
			}
		}

		CacheBytes();
	}
}

void AKPCLNetworkCore::UpdateNetworkMax()
{
	if(!HasAuthority()) return;
	if (ensure(GetNetworkInfoComponent()))
	{
		GetNetworkInfoComponent()->UpdateProcessorCapacity();
	}
}

void AKPCLNetworkCore::CheckNetwork()
{
	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (Network->IsNetworkDirty())
	{
		mNetworkConnections = Network->GetNetworkConnectionBuildings();
		mNetworkManuConnections = Network->GetNetworkAttachments();
		ReGroupSlaves();
	}

	mTotalFluidBytes = Network->GetBytes(true);
	mTotalSolidBytes = Network->GetBytes(false);

	if (GetNetworkInfoComponent())
	{
		GetNetworkInfoComponent()->SetBaseProduction(1000000000.f);
	}
}

void AKPCLNetworkCore::OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved)
{
	Super::OnInputItemAdded(itemClass, numRemoved);

	if (GetInventory())
	{
		if (GetInventory()->GetNumItems(itemClass) == numRemoved && false)
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateLambda([&]()
			{
				if (mOnCoreItemStateStateChanged.IsBound())
				{
					mOnCoreItemStateStateChanged.Broadcast();
				}
			}), TStatId(), nullptr, ENamedThreads::GameThread);
		}
	}

	if (!HasAuthority())
	{
		return;
	}

	const float ByteSize = GetBytesForItemClass(itemClass, numRemoved);
	const bool Fluid = UFGItemDescriptor::GetStackSize(itemClass) > 5000;

	if (ByteSize > 0.0f)
	{
		if (Fluid)
		{
			mUsedFluidBytes += ByteSize;
		}
		else
		{
			mUsedSolidBytes += ByteSize;
		}
	}
}

void AKPCLNetworkCore::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved)
{
	Super::OnInputItemRemoved(itemClass, numRemoved);

	if (GetInventory())
	{
		if (GetInventory()->GetNumItems(itemClass) <= 0 && false)
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateLambda([&]()
			{
				if (mOnCoreItemStateStateChanged.IsBound())
				{
					mOnCoreItemStateStateChanged.Broadcast();
				}
			}), TStatId(), nullptr, ENamedThreads::GameThread);
		}
	}

	if (!HasAuthority())
	{
		return;
	}

	const float ByteSize = GetBytesForItemClass(itemClass, numRemoved);
	const bool Fluid = UFGItemDescriptor::GetStackSize(itemClass) > 5000;

	if (ByteSize > 0.0f)
	{
		if (Fluid)
		{
			mUsedFluidBytes -= ByteSize;
		}
		else
		{
			mUsedSolidBytes -= ByteSize;
		}
	}
}

void AKPCLNetworkCore::CacheBytes()
{
	if (!HasAuthority())
	{
		return;
	}

	fgcheck(GetInventory());

	TArray<FInventoryStack> out_stacks;
	GetInventory()->GetInventoryStacks(out_stacks);
	float TotalSolidByteSize = 0.0f;
	float TotalFluidByteSize = 0.0f;
	for (FInventoryStack Out_Stack : out_stacks)
	{
		float SlotSize = GetBytesForItemClass(Out_Stack.Item.GetItemClass(), Out_Stack.NumItems);
		const bool Fluid = UFGItemDescriptor::GetStackSize(Out_Stack.Item.GetItemClass()) > 5000;

		if (SlotSize > 0.0f)
		{
			if (Fluid)
			{
				TotalFluidByteSize += SlotSize;
			}
			else
			{
				TotalSolidByteSize += SlotSize;
			}
		}
	}

	mUsedFluidBytes = TotalFluidByteSize;
	mUsedSolidBytes = TotalSolidByteSize;
}

void AKPCLNetworkCore::HandleManuConnections(AKPCLNetworkBuildingAttachment* NetworkConnection, float dt)
{
	TArray<FItemAmount> Amounts;
	if (NetworkConnection->GetRequiredItems(Amounts))
	{
		for (FItemAmount Amount : Amounts)
		{
			if (const FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(Amount.ItemClass))
			{
				TArray<FItemAmount> PushedItems;
				const int32 ItemIdx = CoreData->mInventoryIndex;

				mMutexLock.Lock();
				FInventoryStack Stack;
				GetInventory()->GetStackFromIndex(ItemIdx, Stack);

				if (Stack.HasItems())
				{
					Stack.NumItems = FMath::Min(Stack.NumItems, Amount.Amount);
					GetInventory()->RemoveFromIndex(ItemIdx, Stack.NumItems);
					PushedItems.Add(FItemAmount(Stack.Item.GetItemClass(), Stack.NumItems));
				}
				mMutexLock.Unlock();

				NetworkConnection->GetFromNetwork(PushedItems);
			}
		}
	}

	float Solid;
	float Fluid;
	GetFreeBytes(Fluid, Solid);
	if (Solid > 0.0f || Fluid > 0.0f)
	{
		if (NetworkConnection->PushToNetwork(Amounts, Solid, Fluid))
		{
			for (FItemAmount Amount : Amounts)
			{
				if (const FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(Amount.ItemClass))
				{
					TArray<FItemAmount> PushedItems;
					const int32 ItemIdx = CoreData->mInventoryIndex;

					FInventoryStack Stack;
					Stack.NumItems = Amount.Amount;
					Stack.Item.SetItemClass(Amount.ItemClass);

					if (Stack.HasItems())
					{
						mMutexLock.Lock();
						GetInventory()->AddStackToIndex(ItemIdx, Stack);
						mMutexLock.Unlock();
					}
				}
			}
		}
	}
}

void AKPCLNetworkCore::PullBuilding(AKPCLNetworkConnectionBuilding* NetworkConnection, float dt)
{
	if (!ensure(GetInventory() && NetworkConnection))
	{
		return;
	}

	//NetworkConnection->CollectItems(dt);

	FNetworkConnectionInformations Infos;
	NetworkConnection->GetConnectionInformations(Infos);

	FInventoryStack Stack;
	FInventoryStack InfoStack;

	if (Infos.CanPush())
	{
		if (FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(Infos.mItemsToGrab))
		{
			mMutexLock.Lock();
			int32 ItemIdx = CoreData->mInventoryIndex;
			GetInventory()->GetStackFromIndex(ItemIdx, Stack);

			if (Stack.HasItems())
			{
				if (NetworkConnection->TryToReceiveItems(Stack))
				{
					GetInventory()->RemoveFromIndex(ItemIdx, Stack.NumItems);
				}
			}
			mMutexLock.Unlock();
		}
	}
	else if (Infos.bIsInput)
	{
		TSubclassOf<UFGItemDescriptor> Item = NetworkConnection->PeekItemClass();
		if (FKPCLNetworkMaxData* Data = mItemCountMap.FindByKey(FKPCLNetworkMaxData(Item, 0)))
		{
			if (Data->mItemClass)
			{
				FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(Data->mItemClass);
				if (ensureAlways(CoreData))
				{
					mMutexLock.Lock();
					GetInventory()->GetStackFromIndex(CoreData->mInventoryIndex, Stack);
					mMutexLock.Unlock();
					if (Stack.NumItems >= Data->mMaxItemCount)
					{
						NetworkConnection->TryToGrabItems(Stack, .0f, .0f);
						return;
					}
				}
			}
		}

		Stack = FInventoryStack();
		float Solid;
		float Fluid;
		GetFreeBytes(Fluid, Solid);
		if (Solid > 0.0f || Fluid > 0.0f)
		{
			FInventoryStack CheckStack;

			if (NetworkConnection->GetStack(CheckStack))
			{
				if (!CheckStack.HasItems())
				{
					return;
				}

				if (FCoreInventoryData* CoreData = mSlotMappingAll.FindByKey(CheckStack.Item.GetItemClass()))
				{
					int32 ItemIdx = CoreData->mInventoryIndex;

					if (NetworkConnection->TryToGrabItems(Stack, Solid, Fluid))
					{
						if (Stack.HasItems())
						{
							mMutexLock.Lock();
							GetInventory()->AddStackToIndex(ItemIdx, Stack);
							mMutexLock.Unlock();
						}
					}
				}
			}
		}
	}
}
