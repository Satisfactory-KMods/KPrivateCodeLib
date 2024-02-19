// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KPCLNetworkBuildingAttachment.h"
#include "KPCLNetworkConnectionBuilding.h"
#include "KPCLNetworkCore.generated.h"

USTRUCT(BlueprintType)
struct FKPCLNetworkMaxData {
	GENERATED_BODY()

	FKPCLNetworkMaxData() {
		mItemClass = nullptr;
		mMaxItemCount = -1;
	}

	FKPCLNetworkMaxData(TSubclassOf<UFGItemDescriptor> Class, int32 Count) {
		mItemClass = Class;
		mMaxItemCount = Count;
	}

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	TSubclassOf<UFGItemDescriptor> mItemClass;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int32 mMaxItemCount;

	friend bool operator==(const FKPCLNetworkMaxData& A, const FKPCLNetworkMaxData& B) {
		return A.mItemClass == B.mItemClass;
	}

	friend bool operator!=(const FKPCLNetworkMaxData& A, const FKPCLNetworkMaxData& B) {
		return A.mItemClass != B.mItemClass;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCoreItemStateStateChanged);


UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCore: public AKPCLNetworkBuildingBase {
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
		virtual void GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund, bool noBuildCostEnabled) const override;
		virtual void Factory_Tick(float dt) override;
		virtual bool CanProduce_Implementation() const override;
		// END: AFGFactoryBuilding

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
		virtual bool  RemoveFromCore(const TArray<FItemAmount>& Amounts);
		virtual int32 GetIndexFromItem(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	protected:
		virtual void ReconfigureInventory() override;

		UFUNCTION()
		virtual bool FilterPlayerInventory(TSubclassOf<UObject> object, int32 idx) const;

		UFUNCTION()
		virtual bool FormFilterPlayerInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const;

		UFUNCTION()
		virtual void OnPlayerItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved);

		UFUNCTION()
		virtual void OnPlayerItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved);

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

		virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) override;
		virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) override;

		void CacheBytes();

	private:
		friend class UKPCLNetwork;
		friend class AKPCLUnlockSubsystem;
		friend class UKPCLNetworkPlayerComponent;

		inline static int32 mFluidItemsPerBytes = 10000;
		inline static int32 mSolidItemsPerBytes = 100;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Cooling")
		FItemAmount mInputConsume = {UFGNoneDescriptor::StaticClass(), 10};

		UPROPERTY(EditDefaultsOnly, Category="KMods|Cooling")
		int32 mMaxProduceAmount = 5000;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Cooling")
		int32 mMaxConsumeAmount = 5000;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Cooling")
		int32 mByteCalcDiv = 2;

		UPROPERTY(Replicated)
		TArray<AKPCLNetworkConnectionBuilding*> mNetworkConnections;

		UPROPERTY(Replicated)
		TArray<AKPCLNetworkBuildingAttachment*> mNetworkManuConnections;

		// Queue for calling the Player Logic!
		TQueue<UKPCLNetworkPlayerComponent* , EQueueMode::Mpsc> mNetworkPlayerComponentsThisFrame;

		UPROPERTY(Replicated)
		TArray<FCoreInventoryData> mSlotMappingAll;

		UPROPERTY(Replicated, SaveGame)
		TArray<FKPCLNetworkMaxData> mItemCountMap;

		UPROPERTY(Replicated)
		float mUsedSolidBytes = 0;

		UPROPERTY(Replicated)
		float mUsedFluidBytes = 0;

		UPROPERTY(Replicated)
		float mTotalSolidBytes = 0;

		UPROPERTY(Replicated)
		float mTotalFluidBytes = 0;

		// --------------------------
		// Editor Settings
		// --------------------------
		FCriticalSection mMutexLock;
};
