// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkBuildingBase.h"

#include "FGResourceSinkSubsystem.h"
#include "KPCLNetworkConnectionComponent.h"
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

FKPCLFaxitNetwork AKPCLNetworkBuildingBase::GetNetworkData_Implementation() const
{
	if(UKPCLNetwork* Network = Execute_GetNetwork(this))
	{
		if(!Network->GetCore()->mNetworkRef || !Network->CoreStateIsOk()) return FKPCLFaxitNetwork();
		return *Network->GetCore()->mNetworkRef;
	}
	
	return FKPCLFaxitNetwork();
}

bool AKPCLNetworkBuildingBase::HasCoreInNetwork_Implementation() const
{
	if(UKPCLNetwork* Network = Execute_GetNetwork(this))
	{
		return IsValid(Network->GetCore()) && Network->CoreStateIsOk();
	}
	
	return false;
}

void AKPCLNetworkBuildingBase::SetNetworkCore(AKPCLNetworkCore* Core)
{
	mNetworkCore = Core;
}

bool AKPCLNetworkBuildingBase::HasCore_Internal() const
{
	return GetCore_Internal() && IsProducing();
}

AKPCLNetworkCore* AKPCLNetworkBuildingBase::GetCore_Internal() const
{
	if (!HasAuthority())
	{
		return Cast<AKPCLNetworkCore>(mNetworkCore);
	}

	
	if (mNetworkConnection && mNetworkConnection->IsNetworkOk())
	{
		return mNetworkConnection->GetCore();
	}
	
	return mNetworkCore;
}

UKPCLNetwork* AKPCLNetworkBuildingBase::GetNetwork_Internal() const
{
	if (GetNetworkInfoComponent() && GetNetworkInfoComponent()->IsConnected())
	{
		return Cast<UKPCLNetwork>(GetNetworkInfoComponent()->GetPowerCircuit());
	}
	return nullptr;
}

void AKPCLNetworkBuildingBase::OnNetworkDestoryed_Internal()
{
	mNetworkCore = nullptr;
}

void AKPCLNetworkBuildingBase::OnNetworkAdded_Internal(AKPCLNetworkCore* Core)
{
}


AKPCLNetworkBuildingBase::AKPCLNetworkBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
	mPowerInfo = CreateDefaultSubobject<UKPCLNetworkInfoComponent>(TEXT("NetworkConnection"));
}

void AKPCLNetworkBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());

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
			}
		}

		AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
		check(UnlockSubsystem);
	}

	OnNetworkCoreChanged(Execute_GetCore(this));
}

void AKPCLNetworkBuildingBase::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority())
	{
		if(mStateGatherTimer.Tick(dt))
		{
			GatherStates();
		}
		
		if(!IsCore())
		{
			bHasCableBoost = Execute_HasCoreInNetwork(this);
			if (mNetworkCore != Execute_GetCore(this))
			{
				mNetworkCore = Execute_GetCore(this);
				mFaxitSubsystem->AddBuildingToCore(this, mNetworkCore);

				if (IsInGameThread())
				{
					MultiCast_OnNetworkCoreChanged(mNetworkCore);
				}
				else
				{
					AsyncTask(ENamedThreads::GameThread, [&]()
					{
						MultiCast_OnNetworkCoreChanged(mNetworkCore );
					});
				}
			}
		}
	}
}

bool AKPCLNetworkBuildingBase::Factory_IsProducing() const
{
	if (IsValid(mNetworkCore) && mNetworkCore != this && !IsCore())
	{
		return mNetworkCore->IsProducing();
	}
	return Super::Factory_IsProducing();
}

void AKPCLNetworkBuildingBase::TickNetwork(float dt, FKPCLFaxitNetwork* Network)
{
}

void AKPCLNetworkBuildingBase::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);
	FG_DOREPCONDITIONAL(ThisClass, bHasCableBoost);
	FG_DOREPCONDITIONAL(ThisClass, mStates);
}

int32 AKPCLNetworkBuildingBase::SinkItems(FItemAmount Items)
{
	int32 Removed = 0;
	AFGResourceSinkSubsystem* Sub = GetSinkSub();
	if (IsValid(Sub))
	{
		for (int32 C = 0; C < Items.Amount; ++C)
		{
			if (!Sub->AddPoints_ThreadSafe(Items.ItemClass))
			{
				break;
			}
			Removed++;
		}
	}
	return Removed;
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

bool AKPCLNetworkBuildingBase::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect())
	{
		return false;
	}

	if (mNetworkCore)
	{
		return mNetworkCore->IsProducing();
	}
	return false;
}

void AKPCLNetworkBuildingBase::OnCircuitChanged(UFGCircuitConnectionComponent* Component)
{
}

void AKPCLNetworkBuildingBase::MultiCast_OnNetworkCoreChanged_Implementation(AKPCLNetworkCore* Core)
{
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

	DOREPLIFETIME(AKPCLNetworkBuildingBase, mNetworkCore);
}

TArray<FKPCLFaxitNetworkStatData> AKPCLNetworkBuildingBase::GetStates() const
{
	return  mStates;
}

void AKPCLNetworkBuildingBase::GatherStates()
{
	TArray<FKPCLFaxitNetworkStatData> States;
	for (FKPCLFaxitNetworkStatData Data : mCurrentStates)
	{
		States.Add(Data);
	}
	mStates = States;
	mCurrentStates.Empty();
}

FKPCLFaxitNetworkStatData* AKPCLNetworkBuildingBase::GetState(TSubclassOf<UFGItemDescriptor> Item)
{
	FKPCLFaxitNetworkStatData* Found = mCurrentStates.FindByPredicate([Item](const FKPCLFaxitNetworkStatData& Data)
	{
		return Data.mItem == Item;
	});

	if(!Found)
	{
		mCurrentStates.Add(FKPCLFaxitNetworkStatData(Item));
		return GetState(Item);
	}
	return Found;
}

AFGResourceSinkSubsystem* AKPCLNetworkBuildingBase::GetSinkSub()
{
	return AFGResourceSinkSubsystem::Get(GetWorld());
}
