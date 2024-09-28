// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGItemDescriptor.h"
#include "KPCLModSubsystem.h"
#include "KPCLProducerBase.h"
#include "KPCLFaxitSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FKPCLFaxitNetworkStatData
{
	GENERATED_BODY()

private:
	FKPCLFaxitNetworkStatData(): mItem(nullptr) {}

public:
	FKPCLFaxitNetworkStatData(TSubclassOf<UFGItemDescriptor> Item)
	{
		mItem = Item;
	}

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TSubclassOf<UFGItemDescriptor> mItem;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 mInput = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 mOutput = 0;
};

USTRUCT(BlueprintType)
struct FKPCLFaxitNetwork
{
	GENERATED_BODY()

	FKPCLFaxitNetwork(): mCore(nullptr)
	{
	}

	FKPCLFaxitNetwork(FString networkName, AKPCLNetworkCore* Core)
	{
		this->mNetworkName = networkName;
		this->mCore = Core;
	}

	void RemoveActorFromNetwork(AKPCLNetworkBuildingBase* actor);

	void AddActorToNetwork(AKPCLNetworkBuildingBase* actor);

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FString mNetworkName;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<AKPCLNetworkBuildingBase*> mNetworkBuildings;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	AKPCLNetworkCore* mCore;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLFaxitSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaSeconds) override;

public:
	AKPCLFaxitSubsystem();
	FKPCLFaxitNetwork* GetNetworkRef(AKPCLNetworkBuildingBase* Actor);

	UFUNCTION(BlueprintPure, Category = "Subsystem", DisplayName = "GetKPCLFaxitSubsystem",
		meta = ( DefaultToSelf = "worldContext" ))
	static AKPCLFaxitSubsystem* Get(UObject* worldContext);

	UFUNCTION(BlueprintCallable, Category="Faxit")
	FKPCLFaxitNetwork CreateOrAddNetwork(FString networkName, AKPCLNetworkCore* Core);
	FKPCLFaxitNetwork* CreateOrAddNetworkNative(FString networkName, AKPCLNetworkCore* Core);

	void DestoryNetwork(AKPCLNetworkCore* Core);

	UFUNCTION(BlueprintCallable, Category="Faxit")
	bool HasNetwork(AKPCLNetworkBuildingBase* Actor);

	UFUNCTION(BlueprintCallable, Category="Faxit")
	FKPCLFaxitNetwork GetNetwork(AKPCLNetworkBuildingBase* Actor, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="Faxit")
	void UpdateNetworkName(AKPCLNetworkCore* Core, FString NewName);

	UFUNCTION(BlueprintPure, Category="Faxit")
	int32 GetItemsPerMinute() const;

	UFUNCTION(BlueprintPure, Category="Faxit")
	int32 GetFluidPerMinute() const;

	UFUNCTION(BlueprintPure, Category="Faxit")
	int32 GetNetworkLimit() const;

	UFUNCTION(BlueprintPure, Category="Faxit")
	int32 GetNetworkCount() const;

	UFUNCTION(BlueprintPure, Category="Faxit")
	int32 GetBuildingLimit() const;

	void UnlockNetworkTier(int32 Tier, EKPCLConnectionType UnlockType);

private:
	UPROPERTY(SaveGame, Replicated)
	TArray<FKPCLFaxitNetwork> mNetworks;

public:
	UPROPERTY(SaveGame, BlueprintReadOnly, Replicated, Category="Faxit")
	bool mOverflowUnlocked = false;
	
	UPROPERTY(SaveGame, BlueprintReadOnly, Replicated, Category="Faxit")
	bool mRemoteAccessUnlocked = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Replicated, Category="Faxit")
	int32 mNetworkSolidSpeedLevel = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, Replicated, Category="Faxit")
	int32 mNetworkFluidSpeedLevel = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, Replicated, Category="Faxit")
	int32 mNetworkMachineLevel = 1;

	// Configurable values
	// Base value for Max Building Count per Network (xNetworkMachineLevel)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Faxit")
	UCurveFloat* mNetworkCountsPerTier = nullptr;

	// Base value for Max Building Count per Network (xNetworkMachineLevel)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Faxit")
	UCurveFloat* mBuildingCountsPerTier = nullptr;

	// Base value for Max Building Count per Network (xNetworkSolidSpeedLevel)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Faxit")
	UCurveFloat* mBeltSpeedPerTier = nullptr;

	// Base value for Max Building Count per Network (xNetworkFluidSpeedLevel)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Faxit")
	UCurveFloat* mPipeSpeedPerTier = nullptr;
};
