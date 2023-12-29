// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkBuildingBase.h"

#include "KPrivateCodeLibModule.h"
#include "BFL/KBFL_Inventory.h"
#include "C++/KBFLCppInventoryHelper.h"

#include "Net/UnrealNetwork.h"

#include "Network/KPCLNetwork.h"
#include "Network/KPCLNetworkInfoComponent.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

bool AKPCLNetworkBuildingBase::HasCore_Implementation() const
{
	return HasCore_Internal();
}

AKPCLNetworkCore* AKPCLNetworkBuildingBase::GetCore_Implementation() const
{
	return GetCore_Internal();
}

UKPCLNetwork* AKPCLNetworkBuildingBase::GetNetwork_Implementation() const
{
	return GetNetwork_Internal();
}

FNetworkUIData AKPCLNetworkBuildingBase::GetUIDData_Implementation() const
{
	return mNetworkUIData;
}

void AKPCLNetworkBuildingBase::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	DequeueItems();
	DequeueSink();

	Super::PreSaveGame_Implementation(saveVersion, gameVersion);
}

bool AKPCLNetworkBuildingBase::HasCore_Internal() const
{
	return IsValid(GetCore_Internal()) && IsProducing();
}

AKPCLNetworkCore* AKPCLNetworkBuildingBase::GetCore_Internal() const
{
	if (!HasAuthority())
	{
		return Cast<AKPCLNetworkCore>(mCore);
	}

	if (UKPCLNetwork* Network = Execute_GetNetwork(this))
	{
		if (Network->GetCore())
		{
			return Network->GetCore();
		}
		return Cast<AKPCLNetworkCore>(mCore);
	}
	return nullptr;
}

UKPCLNetwork* AKPCLNetworkBuildingBase::GetNetwork_Internal() const
{
	if (GetNetworkInfoComponent() && GetNetworkInfoComponent()->IsConnected())
	{
		return Cast<UKPCLNetwork>(GetNetworkInfoComponent()->GetPowerCircuit());
	}
	return nullptr;
}


AKPCLNetworkBuildingBase::AKPCLNetworkBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
	mPowerInfoClass = UKPCLNetworkInfoComponent::StaticClass();
}

void AKPCLNetworkBuildingBase::MultiCast_OnNetworkCoreChanged_Implementation(bool HasCore)
{
}

void AKPCLNetworkBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	TArray<UFGPowerInfoComponent*> Infos;
	GetComponents<UFGPowerInfoComponent>(Infos);
	for (UFGPowerInfoComponent* Info : Infos)
	{
		if (UFGPowerInfoComponent* AsPower = ExactCast<UFGPowerInfoComponent>(Info))
		{
			mPowerInfo = AsPower;
		}

		if (UKPCLNetworkInfoComponent* AsNetwork = ExactCast<UKPCLNetworkInfoComponent>(Info))
		{
			mNetworkInfoComponent = AsNetwork;
		}
	}
	mNetworkConnection = FindComponentByClass<UKPCLNetworkConnectionComponent>();

	if (HasAuthority())
	{
		if (GetNetworkConnectionComponent())
		{
			GetNetworkConnectionComponent()->SetPowerInfo(GetNetworkInfoComponent());
			if (bBindNetworkComponent)
			{
				GetNetworkConnectionComponent()->OnConnectionChanged.AddUObject(
					this, &AKPCLNetworkBuildingBase::OnCircuitChanged);
				GetNetworkInfoComponent()->CoreStateChanged.AddUObject(
					this, &AKPCLNetworkBuildingBase::OnHasCoreChanged);
				GetNetworkInfoComponent()->MaxTransferChanged.AddUObject(this, &AKPCLNetworkBuildingBase::OnMaxChanged);
			}
		}

		AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
		check(UnlockSubsystem);

		mUnlockedTier = UnlockSubsystem->GetNetworkTier();
		UnlockSubsystem->OnNetworkTierUnlocked.AddUniqueDynamic(this, &AKPCLNetworkBuildingBase::OnTierUnlocked);
	}

	OnNetworkCoreChanged(Execute_GetCore(this) != nullptr);
}

void AKPCLNetworkBuildingBase::Factory_Tick(float dt)
{
	DequeueItems();
	DequeueSink();

	Super::Factory_Tick(dt);

	if (HasAuthority())
	{
		if (mCore != Execute_GetCore(this))
		{
			mCore = Execute_GetCore(this);

			if (IsInGameThread())
			{
				MultiCast_OnNetworkCoreChanged(mCore != nullptr);
			}
			else
			{
				AsyncTask(ENamedThreads::GameThread, [&]()
				{
					MultiCast_OnNetworkCoreChanged(mCore != nullptr);
				});
			}
		}
	}
}

void AKPCLNetworkBuildingBase::DequeueItems()
{
	if (HasAuthority() && GetInventory())
	{
		if (!mInventoryQueue.IsEmpty())
		{
			while (!mInventoryQueue.IsEmpty())
			{
				FKPCLItemTransferQueue QueueItem;
				mInventoryQueue.Dequeue(QueueItem);
				if (QueueItem.IsValid())
				{
					if (QueueItem.mAddAmount)
					{
						UKBFLCppInventoryHelper::AddItemsInInventory(GetInventory(), QueueItem.mAmount.ItemClass,
						                                             QueueItem.mAmount.Amount);
					}
					else
					{
						FInventoryStack Stack;
						GetInventory()->GetStackFromIndex(QueueItem.mInventoryIndex, Stack);
						if (Stack.HasItems())
						{
							Stack.NumItems = FMath::Min(Stack.NumItems, QueueItem.mAmount.Amount);
							GetInventory()->RemoveFromIndex(QueueItem.mInventoryIndex, Stack.NumItems);
						}
					}
				}
			}
		}
	}
}

void AKPCLNetworkBuildingBase::DequeueSink()
{
	if (HasAuthority() && GetInventory())
	{
		AFGResourceSinkSubsystem* Sub = GetSinkSub();
		if (IsValid(Sub))
		{
			while (!mSinkQueue.IsEmpty())
			{
				FKPCLSinkQueue Sink;
				if (mSinkQueue.Dequeue(Sink))
				{
					if (IsValid(Sink.mAmount.ItemClass) && Sink.mAmount.Amount > 0 && Sink.mIndex != INDEX_NONE)
					{
						if (UFGItemDescriptor::GetForm(Sink.mAmount.ItemClass) == EResourceForm::RF_SOLID)
						{
							if (GetInventory()->IsValidIndex(Sink.mIndex))
							{
								for (int32 C = 0; C < Sink.mAmount.Amount; ++C)
								{
									if (!Sub->AddPoints_ThreadSafe(Sink.mAmount.ItemClass))
									{
										break;
									}
								}
								GetInventory()->RemoveFromIndex(Sink.mIndex, Sink.mAmount.Amount);
							}
						}
					}
				}
			}
		}
	}
}

bool AKPCLNetworkBuildingBase::Factory_IsProducing() const
{
	if (IsValid(mCore) && mCore != this && !IsCore())
	{
		return mCore->IsProducing();
	}
	return Super::Factory_IsProducing();
}

void AKPCLNetworkBuildingBase::RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player)
{
	Super::RegisterInteractingPlayer_Implementation(player);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (HasAuthority() && Network)
	{
		Network->RegisterInteractingPlayer(player);

		if (!IsCore())
		{
			if (Network->NetworkHasCore())
			{
				Execute_RegisterInteractingPlayer(Network->GetCore(), player);
			}
		}
	}
}

void AKPCLNetworkBuildingBase::UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player)
{
	Super::UnregisterInteractingPlayer_Implementation(player);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (HasAuthority() && Network)
	{
		Network->UnregisterInteractingPlayer(player);

		if (!IsCore())
		{
			if (Network->NetworkHasCore())
			{
				Execute_UnregisterInteractingPlayer(Network->GetCore(), player);
			}
		}
	}
}

void AKPCLNetworkBuildingBase::OnReplicationDetailActorCreated()
{
	Super::OnReplicationDetailActorCreated();
}

void AKPCLNetworkBuildingBase::OnReplicationDetailActorRemoved()
{
	Super::OnReplicationDetailActorRemoved();
}

bool AKPCLNetworkBuildingBase::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect())
	{
		return false;
	}

	if (mCore)
	{
		return mCore->IsProducing();
	}
	return false;
}

void AKPCLNetworkBuildingBase::OnCircuitChanged(UFGCircuitConnectionComponent* Component)
{
}

void AKPCLNetworkBuildingBase::OnTierUnlocked(int32 Tier)
{
	if (mUnlockedTier != FMath::Clamp(Tier, 0, mMaxTier))
	{
		mUnlockedTier = FMath::Clamp(Tier, 0, mMaxTier);
		OnTierUpdated();
	}
}


int32 AKPCLNetworkBuildingBase::GetTier() const
{
	return FMath::Clamp(mUnlockedTier, 0, mMaxTier);
}

bool AKPCLNetworkBuildingBase::IsCore() const
{
	return false;
}

UKPCLNetworkInfoComponent* AKPCLNetworkBuildingBase::GetNetworkInfoComponent() const
{
	if (mNetworkInfoComponent)
	{
		return mNetworkInfoComponent;
	}

	return FindComponentByClass<UKPCLNetworkInfoComponent>();
}

UKPCLNetworkConnectionComponent* AKPCLNetworkBuildingBase::GetNetworkConnectionComponent() const
{
	if (mNetworkConnection)
	{
		return mNetworkConnection;
	}

	return FindComponentByClass<UKPCLNetworkConnectionComponent>();
}

UFGPowerInfoComponent* AKPCLNetworkBuildingBase::GetPowerInfoExplicit() const
{
	if (ExactCast<UFGPowerInfoComponent>(GetPowerInfo()))
	{
		return GetPowerInfo();
	}

	TArray<UFGPowerInfoComponent*> Infos;
	GetComponents<UFGPowerInfoComponent>(Infos);
	for (UFGPowerInfoComponent* Info : Infos)
	{
		if (UFGPowerInfoComponent* AsExact = ExactCast<UFGPowerInfoComponent>(Info))
		{
			return AsExact;
		}
	}

	return nullptr;
}

UFGPowerConnectionComponent* AKPCLNetworkBuildingBase::GetPowerConnectionExplicit() const
{
	if (ExactCast<UFGPowerConnectionComponent>(GetPowerConnection()))
	{
		return GetPowerConnection();
	}

	TArray<UFGPowerConnectionComponent*> Infos;
	GetComponents<UFGPowerConnectionComponent>(Infos);
	for (UFGPowerConnectionComponent* Info : Infos)
	{
		if (UFGPowerConnectionComponent* AsExact = ExactCast<UFGPowerConnectionComponent>(Info))
		{
			return AsExact;
		}
	}

	return nullptr;
}

void AKPCLNetworkBuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLNetworkBuildingBase, mCore);
	DOREPLIFETIME(AKPCLNetworkBuildingBase, mUnlockedTier);
}

AFGResourceSinkSubsystem* AKPCLNetworkBuildingBase::GetSinkSub()
{
	return nullptr;
}
