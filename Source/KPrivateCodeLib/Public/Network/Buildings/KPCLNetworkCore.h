#pragma once

#include "CoreMinimal.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "Resources/FGItemDescriptor.h"
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
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END: AActor

	// START: Modular Building
	void TryConnectNetworks(AFGBuildable* OtherBuildable) const;
	// END: Modular Building

	// START: KPCL
	virtual FKPCLFaxitNetwork GetNetworkData_Implementation() const override;
	virtual bool              HasCoreInNetwork_Implementation() const override;
	virtual bool              IsCore() const override;

	/** Overwrite the Power handle to translate network to power */
	virtual void HandlePower(float dt) override;
	// END: KPCL

	// START: AFGFactoryBuilding
	virtual void
	GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund, bool noBuildCostEnabled) const override;
	virtual void Factory_Tick(float dt) override;
	virtual bool CanProduce_Implementation() const override;
	// END: AFGFactoryBuilding

	// START: Player HANDLE
	bool CanConsumeFromPlayerInventory() const;
	void OnConsumeFromPlayerInventory();
	bool GetStackThatCanConsumeFromPlayerInventory(FInventoryStack& Stack, int32& Index) const;

	UPROPERTY(EditDefaultsOnly, SaveGame, Replicated, Category="KMods")
	FFullProductionHandle mPlayerInventoryHandle;
	// END: Player HANDLE

public:
	// START: Item Handle
	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	FItemAmount  GetItemOrCreateAmount(TSubclassOf<UFGItemDescriptor> Item);
	FItemAmount* GetItemAmountRef(TSubclassOf<UFGItemDescriptor> Item);
	int32        GetMaxItemAmount(TSubclassOf<UFGItemDescriptor> Item) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	TArray<FItemAmount> GetItemAmounts() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	void GetItemAmountsFiltered(EResourceForm Form, TArray<FItemAmount>& Out) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	void GrabFromNetwork(AFGCharacterPlayer* Player, FItemAmount Amount);

	/**
	 * return true if the storage is full and can't store no more
	 */
	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	int32 IsStorageFull(TSubclassOf<UFGItemDescriptor> Item);
	int32 IsStorageFull(FItemAmount* ItemAmount);

	/**
	 * return true if the storage is full and can't store no more
	 */
	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	int32 IsStorageEmpty(TSubclassOf<UFGItemDescriptor> Item);
	int32 IsStorageEmpty(FItemAmount* ItemAmount);

	/**
	 * Returns the amount that was stored in the network
	 */
	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	int32 TryToStoreItem(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> Item, int32 Amount = 0);

	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	int32 TryToStoreItemAmount(TSubclassOf<UFGItemDescriptor> Item, int32 Amount);

	/**
	 * Returns the amount that was grabed from the network
	 */
	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	int32 TryToGrabItem(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> Item, int32 Amount = 0);

	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	int32 TryToGrabItemAmount(TSubclassOf<UFGItemDescriptor> Item, int32 Amount);
	// END: Item Handle

	// Start Player Inventory
	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	UFGInventoryComponent* GetPlayerBufferInventory() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	TArray<FKPCLFaxitNetworkStatDataBundle> GetStateBundles() const;

	virtual void GatherStates() override;

	virtual void TickNetwork(float dt, FKPCLFaxitNetwork* Network) override;

protected:
	virtual bool FormFilterOutputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const override;
	virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;

	virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved, UFGInventoryComponent* sourceInventory) override;
	virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved, UFGInventoryComponent* sourceInventory) override;

	void UpdateStorageState();
	void CheckStorageState();

	void FlushOverflow();
	void NotifyStorageChange();

private:
	friend class UKPCLNetwork;
	friend class AKPCLUnlockSubsystem;
	friend class UKPCLNetworkPlayerComponent;

	UPROPERTY(Replicated)
	TArray<class AKPCLNetworkConnectionBuilding*> mNetworkConnections;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mInputInventory = nullptr;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mOutputInventory = nullptr;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category="KMods|Faxit")
	FSmartTimer mItemFlushTimer = FSmartTimer(300.f, false);

public:
	FKPCLFaxitNetwork* mNetworkRef = nullptr;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ))
	TArray<FKPCLFaxitNetworkStatDataBundle> mStateBundels;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ))
	TArray<FItemAmount> mStorage;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ))
	float mNetworkPower = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ))
	float mMaxNetworkPower = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ))
	int32 mStackSizeMultiplier = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ))
	float mDrivePower = 0.f;

	UPROPERTY(BlueprintAssignable, Category="KMods|Faxit")
	FOnCoreItemStateStateChanged OnStorageChanged;
};