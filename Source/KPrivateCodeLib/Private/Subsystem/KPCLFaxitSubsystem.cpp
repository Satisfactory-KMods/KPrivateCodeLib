// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/KPCLFaxitSubsystem.h"

#include "KPCLNetworkCore.h"
#include "UnrealNetwork.h"

void AKPCLFaxitSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworks);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mOverflowUnlocked);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkMachineLevel);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkSpeedLevel);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkStackSizeLevel);
}

void AKPCLFaxitSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const int32 NumPerGroup = FMath::Max(FMath::DivideAndRoundUp(mNetworks.Num(), 8), 1);
	ParallelFor(8, [&](int32 Index)
	{
		
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

FKPCLFaxitNetwork* AKPCLFaxitSubsystem::GetNetworkRef(AActor* Actor)
{
	for (FKPCLFaxitNetwork& network : mNetworks)
	{
		if (network.mCore == Cast<AKPCLNetworkCore>(Actor) || network.mNetworkBuildings.Contains(Actor))
		{
			return &network;
		}
	}
	return nullptr;
}

FKPCLFaxitNetwork AKPCLFaxitSubsystem::CreateOrAddNetwork(FString networkName, AKPCLNetworkCore* Core)
{
	return FKPCLFaxitNetwork();
}

bool AKPCLFaxitSubsystem::HasNetwork(AActor* Actor)
{
	bool bSuccess;
	GetNetwork(Actor, bSuccess);
	return bSuccess;
}

FKPCLFaxitNetwork AKPCLFaxitSubsystem::GetNetwork(AActor* Actor, bool& bSuccess)
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

void AKPCLFaxitSubsystem::UpdateNetworkName(AActor* Actor, FString NewName)
{
	FKPCLFaxitNetwork* network = GetNetworkRef(Actor);
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

void FKPCLFaxitNetwork::RemoveActorFromNetwork(AActor* actor)
{
	mNetworkBuildings.Remove(actor);
}

void FKPCLFaxitNetwork::AddActorToNetwork(AActor* actor)
{
	mNetworkBuildings.AddUnique(actor);
}
