// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/KPCLFaxitSubsystem.h"

#include "BFL/KBFL_Util.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkCore.h"

void AKPCLFaxitSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworks);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mOverflowUnlocked);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkMachineLevel);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkFluidSpeedLevel);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkSolidSpeedLevel);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mOverflowUnlocked);
}

void AKPCLFaxitSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const int32 NumPerGroup = FMath::Max(FMath::DivideAndRoundUp(mNetworks.Num(), 8), 1);
	ParallelFor(8, [&](int32 Index) {
		for (int32 Member = Index * NumPerGroup; Member < FMath::Min(
			     (Index + 1) * NumPerGroup, mNetworks.Num()); Member++)
		{
			FKPCLFaxitNetwork* Network = &mNetworks[Member];
			if (ensure(Network) && ensure(Network->mCore))
			{
				Network->mCore->TickNetwork(DeltaSeconds, Network);
			}
		}
	});
}

AKPCLFaxitSubsystem::AKPCLFaxitSubsystem()
{
	mShouldSave = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_LastDemotable;
	PrimaryActorTick.EndTickGroup = TG_LastDemotable;
}

FKPCLFaxitNetwork* AKPCLFaxitSubsystem::GetNetworkRef(AKPCLNetworkBuildingBase* Actor)
{
	return mNetworks.FindByPredicate([Actor](const FKPCLFaxitNetwork& item) {
		return item.mCore == Actor || item.mNetworkBuildings.Contains(Actor);
	});
}

AKPCLFaxitSubsystem* AKPCLFaxitSubsystem::Get(UObject* worldContext)
{
	return Cast<AKPCLFaxitSubsystem>(UKBFL_Util::GetSubsystemFromChild(worldContext, StaticClass()));
}

FKPCLFaxitNetwork AKPCLFaxitSubsystem::CreateOrAddNetwork(FString networkName, AKPCLNetworkCore* Core)
{
	return *CreateOrAddNetworkNative(networkName, Core);
}

FKPCLFaxitNetwork* AKPCLFaxitSubsystem::CreateOrAddNetworkNative(FString networkName, AKPCLNetworkCore* Core)
{
	FKPCLFaxitNetwork* found = mNetworks.FindByPredicate([Core](const FKPCLFaxitNetwork& item) {
		return item.mCore == Core;
	});

	if (!found)
	{
		mNetworks.Add(FKPCLFaxitNetwork(networkName, Core));
		return mNetworks.FindByPredicate([Core](const FKPCLFaxitNetwork& item) {
			return item.mCore == Core;
		});
	}

	return found;
}

void AKPCLFaxitSubsystem::DestoryNetwork(AKPCLNetworkCore* Core)
{
	FKPCLFaxitNetwork* found = CreateOrAddNetworkNative(Core->GetName(), Core);
	if (found)
	{
		for (AKPCLNetworkBuildingBase*
		     NetworkBuilding : found->mNetworkBuildings)
		{
			if (IsValid(NetworkBuilding))
			{
				NetworkBuilding->OnNetworkDestoryed_Internal();
			}
		}
	}

	mNetworks.RemoveAll([Core](const FKPCLFaxitNetwork& item) {
		return item.mCore == Core;
	});
}

void AKPCLFaxitSubsystem::DestroyNetworkBuilding(AKPCLNetworkBuildingBase* Building)
{
	FKPCLFaxitNetwork* FaxitNetwork = GetNetworkRef(Building);
	if (FaxitNetwork)
	{
		FaxitNetwork->RemoveActorFromNetwork(Building);
	}
}

void AKPCLFaxitSubsystem::AddBuildingToCore(AKPCLNetworkBuildingBase* Building, AKPCLNetworkCore* Core)
{
	FKPCLFaxitNetwork* FaxitNetwork = GetNetworkRef(Core);
	if (FaxitNetwork)
	{
		DestroyNetworkBuilding(Building);
		FaxitNetwork->AddActorToNetwork(Building);
		Building->SetNetworkCore(FaxitNetwork->mCore);
	}
}

bool AKPCLFaxitSubsystem::HasNetwork(AKPCLNetworkBuildingBase* Actor)
{
	bool bSuccess;
	GetNetwork(Actor, bSuccess);
	return bSuccess;
}

FKPCLFaxitNetwork AKPCLFaxitSubsystem::GetNetwork(AKPCLNetworkBuildingBase* Actor, bool& bSuccess)
{
	bSuccess = false;
	FKPCLFaxitNetwork* network = GetNetworkRef(Actor);
	if (network)
	{
		bSuccess = true;
		return *network;
	}
	return FKPCLFaxitNetwork();
}

void AKPCLFaxitSubsystem::UpdateNetworkName(AKPCLNetworkCore* Core, FString NewName)
{
	FKPCLFaxitNetwork* network = GetNetworkRef(Core);
	if (network)
	{
		network->mNetworkName = NewName;
	}
}

int32 AKPCLFaxitSubsystem::GetItemsPerMinute() const
{
	return mBeltSpeedPerTier->GetFloatValue(mNetworkSolidSpeedLevel);
}

int32 AKPCLFaxitSubsystem::GetFluidPerMinute() const
{
	return mPipeSpeedPerTier->GetFloatValue(mNetworkFluidSpeedLevel);
}

int32 AKPCLFaxitSubsystem::GetNetworkLimit() const
{
	return mNetworkCountsPerTier->GetFloatValue(mNetworkMachineLevel);
}

int32 AKPCLFaxitSubsystem::GetNetworkCount() const
{
	return mNetworks.Num();
}

int32 AKPCLFaxitSubsystem::GetBuildingLimit() const
{
	return mBuildingCountsPerTier->GetFloatValue(mNetworkMachineLevel);
}

void AKPCLFaxitSubsystem::UnlockNetworkTier(int32 Tier, EKPCLUnlockTier UnlockType)
{
	switch (UnlockType)
	{
		case EKPCLUnlockTier::Overflow:
			mOverflowUnlocked = true;
			break;
		case EKPCLUnlockTier::RemoteAccess:
			mRemoteAccessUnlocked = true;
			break;
		case EKPCLUnlockTier::NetworkSolidSpeedLevel:
			mNetworkSolidSpeedLevel += Tier;
			break;
		case EKPCLUnlockTier::NetworkFluidSpeedLevel:
			mNetworkFluidSpeedLevel += Tier;
			break;
		case EKPCLUnlockTier::NetworkMachineLevel:
			mNetworkMachineLevel += Tier;
			break;
	}

	const int32 NumPerGroup = FMath::Max(FMath::DivideAndRoundUp(mNetworks.Num(), 8), 1);
	ParallelFor(8, [&](int32 Index) {
		for (int32 Member = Index * NumPerGroup; Member < FMath::Min(
			     (Index + 1) * NumPerGroup, mNetworks.Num()); Member++)
		{
			FKPCLFaxitNetwork* Network = &mNetworks[Member];
			if (ensure(Network) && ensure(Network->mCore))
			{
				Network->mCore->OnTiersUpdated();
				for (AKPCLNetworkBuildingBase* NetworkBuilding : Network->mNetworkBuildings)
				{
					NetworkBuilding->OnTiersUpdated();
				}
			}
		}
	});
}

void FKPCLFaxitNetworkStatData::Merge(FKPCLFaxitNetworkStatData& Other, bool ResetOther)
{
	if (mItem != Other.mItem)
	{
		return;
	}

	mDownload += Other.mDownload;
	mUpload += Other.mUpload;

	if (!ResetOther)
	{
		return;
	}
	Other.mDownload = 0;
	Other.mUpload = 0;
}

void FKPCLFaxitNetworkStatData::Merge(FKPCLFaxitNetworkStatData* Other, bool ResetOther)
{
	if (!Other)
	{
		return;
	}
	Merge(*Other, ResetOther);
}

void FKPCLFaxitNetwork::RemoveActorFromNetwork(AKPCLNetworkBuildingBase* actor)
{
	mNetworkBuildings.Remove(actor);
}

void FKPCLFaxitNetwork::AddActorToNetwork(AKPCLNetworkBuildingBase* actor)
{
	mNetworkBuildings.AddUnique(actor);
}