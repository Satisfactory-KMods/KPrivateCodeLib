// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KPCLNetworkBuildingAttachment.h"
#include "KPCLNetworkConnectionBuilding.h"
#include "KPCLNetworkCore.generated.h"

USTRUCT(BlueprintType)
struct FKPCLNetworkMaxData
{
	GENERATED_BODY()

	FKPCLNetworkMaxData()
	{
		mItemClass = nullptr;
		mMaxItemCount = -1;
	}

	FKPCLNetworkMaxData(TSubclassOf<UFGItemDescriptor> Class, int32 Count)
	{
		mItemClass = Class;
		mMaxItemCount = Count;
	}

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	TSubclassOf<UFGItemDescriptor> mItemClass;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int32 mMaxItemCount;

	friend bool operator==(const FKPCLNetworkMaxData& A, const FKPCLNetworkMaxData& B)
	{
		return A.mItemClass == B.mItemClass;
	}

	friend bool operator!=(const FKPCLNetworkMaxData& A, const FKPCLNetworkMaxData& B)
	{
		return A.mItemClass != B.mItemClass;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCoreItemStateStateChanged);


UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCore : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkCore();

protected:
	// START: AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END: AActor

	// START: Modular Building
	void TryConnectNetworks(AFGBuildable* OtherBuildable) const;

	virtual void onProducingFinal_Implementation() override;

	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	// END: Modular Building

	// START: KPCL
	virtual void OnTierUpdated() override;
	virtual bool IsCore() const override;

	/** Overwrite the Power handle to translate network to power */
	virtual void HandlePower(float dt) override;
	// END: KPCL

	// START: AFGFactoryBuilding
	virtual void
	GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund, bool noBuildCostEnabled) const override;
	virtual void Factory_Tick(float dt) override;
	virtual void Factory_TickAuthOnly(float dt) override;
	virtual bool CanProduce_Implementation() const override;
	// END: AFGFactoryBuilding

	// START: Player HANDLE
	bool CanConsumeFromPlayerInventory() const;
	void OnConsumeFromPlayerInventory();
	bool GetStackThatCanConsumeFromPlayerInventory(FInventoryStack& Stack, int32& Index) const;

	UPROPERTY(EditDefaultsOnly, SaveGame, Replicated, Category="KMods")
	FFullProductionHandle mPlayerInventoryHandle;
	// END: Player HANDLE

	virtual void TickPlayerNetworkInventory(float dt);
	virtual void TickPlayerBufferInventory(float dt);
	virtual void ReGroupSlaves();

	// Start Player Inventory
public:
	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	UFGInventoryComponent* GetPlayerBufferInventory() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	UFGInventoryComponent* GetFluidBufferInventory() const;

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	virtual bool CanRemoveFromCore(const TArray<FItemAmount>& Amounts) const;

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	virtual int32 GetAmountFromItemClass(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	// Host Only
	UFUNCTION(BlueprintPure, Category="KMods|Network")
	virtual bool RemoveFromCore(const TArray<FItemAmount>& Amounts);
	virtual int32 GetIndexFromItem(TSubclassOf<UFGItemDescriptor> ItemClass) const;

protected:
	UFUNCTION()
	virtual bool FilterPlayerInventory(TSubclassOf<UObject> object, int32 idx) const;

	virtual bool FilterOutputInventory(TSubclassOf<UObject> object, int32 idx) const override
	{
		return FilterPlayerInventory(object, idx);
	};

	UFUNCTION()
	virtual bool FormFilterPlayerInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const;

	virtual bool FormFilterOutputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const override
	{
		return FormFilterPlayerInventory(object, idx);
	};

	UFUNCTION()
	virtual void OnPlayerItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved);

	virtual void OnOutputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                                 UFGInventoryComponent* sourceInventory) override
	{
		return OnPlayerItemRemoved(itemClass, numRemoved);
	}

	UFUNCTION()
	virtual void OnPlayerItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved);

	virtual void OnOutputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                               UFGInventoryComponent* sourceInventory) override
	{
		return OnPlayerItemAdded(itemClass, numRemoved);
	}

public:
	UFUNCTION(BlueprintCallable, Category="KMods|Network")
	void GetCoreData(FCoreDataSortOptionStruc SortOption, TArray<FCoreInventoryData>& Data);

	UFUNCTION(BlueprintCallable, Category="KMods|Network")
	void GetTotalBytes(float& Fluid, float& Solid) const;

	UFUNCTION(BlueprintCallable, Category="KMods|Network")
	void GetUsedBytes(float& Fluid, float& Solid) const;

	UFUNCTION(BlueprintCallable, Category="KMods|Network")
	void GetFreeBytes(float& Fluid, float& Solid) const;

	UFUNCTION(BlueprintCallable, Category="KMods|Network")
	void GetFreeBytesPrt(float& Fluid, float& Solid) const;

	/** Get the index of a Item */
	UFUNCTION(BlueprintPure, Category="KMods|Network")
	int32 GetAllowedIndex(TSubclassOf<UFGItemDescriptor> Item) const;

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	static float GetBytesForItemClass(TSubclassOf<UFGItemDescriptor> itemClass, float Num = 1.0f);

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	static int32 GetMaxItemsByBytes(TSubclassOf<UFGItemDescriptor> itemClass, float FluidBytes, float SolidBytes);

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	static float GetBytesForItemAmount(FItemAmount Amount, bool& IsFluid);

	UFUNCTION(BlueprintCallable, Category="KMods|Network")
	void Core_SetMaxItemCount(TSubclassOf<UFGItemDescriptor> Item, int32 Max);

	UFUNCTION(BlueprintCallable, Category="KMods|Network")
	bool GetStackFromNetwork(TSubclassOf<UFGItemDescriptor> Item, FInventoryStack& Stack, int32& Index);

	UFUNCTION(BlueprintCallable, Category="KMods|Network")
	FKPCLNetworkMaxData Core_GetMaxItemCount(TSubclassOf<UFGItemDescriptor> Item);

	UPROPERTY(BlueprintAssignable)
	FOnCoreItemStateStateChanged mOnCoreItemStateStateChanged;

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	int32 GetCurrentInputAmount() const;

private:
	/** Configure the Inventory to all items */
	void ConfigureCoreInventory();

	/** Configure the Inventory to all items */
	void UpdateNetworkMax();

	/** Check the network for Dirty and get alle Connections */
	void CheckNetwork();

	/** Call all relevant things on slaves */
	void PullBuilding(AKPCLNetworkConnectionBuilding* NetworkConnection, float dt);

	/** Call all relevant things on slaves */
	void HandleManuConnections(AKPCLNetworkBuildingAttachment* NetworkConnection, float dt);

	virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                              UFGInventoryComponent* sourceInventory) override;
	virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                                UFGInventoryComponent* sourceInventory) override;

	void CacheBytes();

	friend class UKPCLNetwork;
	friend class AKPCLUnlockSubsystem;
	friend class UKPCLNetworkPlayerComponent;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Cooling")
	int32 mByteCalcDiv = 2;

	UPROPERTY(Replicated)
	TArray<AKPCLNetworkConnectionBuilding*> mNetworkConnections;

	UPROPERTY(SaveGame)
	UFGInventoryComponent* mInputInventory = nullptr;

	UPROPERTY(SaveGame)
	UFGInventoryComponent* mOutputInventory = nullptr;

	UPROPERTY(SaveGame)
	UFGInventoryComponent* mBoosterInventory = nullptr;

	// --------------------------
	// Editor Settings
	// --------------------------
	FCriticalSection mMutexLock;

public:
	UPROPERTY(SaveGame, BlueprintReadWrite, meta = ( FGReplicated ))
	TArray<FKPCLFaxitNetworkStatData> mItemStats;
	
	UPROPERTY(SaveGame, BlueprintReadWrite, meta = ( FGReplicated ))
	TArray<FItemAmount> mStorage;
};
