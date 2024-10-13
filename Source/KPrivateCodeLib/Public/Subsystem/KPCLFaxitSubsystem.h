// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KPCLModSubsystem.h"
#include "Resources/FGItemDescriptor.h"
#include "Unlocks/KPCLUnlockNetworkTier.h"
#include "KPCLFaxitSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FKPCLFaxitNetworkStatData
{
	GENERATED_BODY()

	FKPCLFaxitNetworkStatData()
		: mItem(nullptr) {}

	FKPCLFaxitNetworkStatData(TSubclassOf<UFGItemDescriptor> Item)
	{
		mItem = Item;
	}

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItem;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 mUpload = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 mDownload = 0;

	// Compare item descriptors
	bool operator==(const FKPCLFaxitNetworkStatData& Other) const
	{
		return mItem == Other.mItem;
	}

	void Merge(FKPCLFaxitNetworkStatData& Other, bool ResetOther = false);
	void Merge(FKPCLFaxitNetworkStatData* Other, bool ResetOther = false);
};

USTRUCT(BlueprintType)
struct FKPCLFaxitNetworkStatDataBundle
{
	GENERATED_BODY()

	FKPCLFaxitNetworkStatDataBundle() {}

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int64 mTimestamp = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FKPCLFaxitNetworkStatData> mStats;
};

USTRUCT(BlueprintType)
struct FKPCLFaxitNetwork
{
	GENERATED_BODY()

	FKPCLFaxitNetwork()
		: mCore(nullptr) {}

	FKPCLFaxitNetwork(FString networkName, AKPCLNetworkCore* Core)
	{
		this->mNetworkName = networkName;
		this->mCore = Core;
		this->IsValid = Core != nullptr;
	}

	void RemoveActorFromNetwork(AKPCLNetworkBuildingBase* actor);

	void AddActorToNetwork(AKPCLNetworkBuildingBase* actor);

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FString mNetworkName;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	AKPCLNetworkCore* mCore;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<AKPCLNetworkBuildingBase*> mNetworkBuildings;

	UPROPERTY(BlueprintReadOnly)
	bool IsValid = false;
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
	FKPCLFaxitNetwork  CreateOrAddNetwork(FString networkName, AKPCLNetworkCore* Core);
	FKPCLFaxitNetwork* CreateOrAddNetworkNative(FString networkName, AKPCLNetworkCore* Core);

	void DestoryNetwork(AKPCLNetworkCore* Core);

	void DestroyNetworkBuilding(AKPCLNetworkBuildingBase* Building);

	UFUNCTION(BlueprintCallable, Category="Faxit")
	void AddBuildingToCore(AKPCLNetworkBuildingBase* Building, AKPCLNetworkCore* Core);

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

	void UnlockNetworkTier(int32 Tier, EKPCLUnlockTier UnlockType);

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