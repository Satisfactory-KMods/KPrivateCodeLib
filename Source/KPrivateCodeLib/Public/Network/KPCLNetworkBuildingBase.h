// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KPCLNetworkConnectionComponent.h"
#include "KPCLNetworkInfoComponent.h"
#include "Buildable/KPCLProducerBase.h"
#include "Buildable/Modular/KPCLModularBuildingBase.h"
#include "KPCLNetworkBuildingBase.generated.h"

USTRUCT(BlueprintType)
struct FKPCLItemTransferQueue {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mAddAmount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemAmount mAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mInventoryIndex = 0;

	bool IsValid() const {
		return mAmount.Amount > 0 && mInventoryIndex > INDEX_NONE && mAmount.ItemClass != nullptr;
	}
};

USTRUCT(BlueprintType)
struct FKPCLSinkQueue {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemAmount mAmount;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkBuildingBase: public AKPCLModularBuildingBase, public IKPCLNetworkDataInterface {
	GENERATED_BODY()

	public:
		//Begin IKPCLNetworkDataInterface
		virtual bool              HasCore_Implementation() const override;
		virtual AKPCLNetworkCore* GetCore_Implementation() const override;
		virtual UKPCLNetwork*     GetNetwork_Implementation() const override;
		virtual FNetworkUIData    GetUIDData_Implementation() const override;
		virtual void              PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;

		virtual bool              HasCore_Internal() const;
		virtual AKPCLNetworkCore* GetCore_Internal() const;
		virtual UKPCLNetwork*     GetNetwork_Internal() const;

		UPROPERTY(EditDefaultsOnly, Category="KMods|UI")
		FNetworkUIData mNetworkUIData;
		// End IKPCLNetworkDataInterface

		AKPCLNetworkBuildingBase();

		virtual void BeginPlay() override;
		virtual void Factory_Tick(float dt) override;
		virtual void DequeueItems();
		virtual void DequeueSink();
		virtual bool Factory_IsProducing() const override;

		virtual void RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;
		virtual void UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;

		virtual void OnReplicationDetailActorCreated() override;
		virtual void OnReplicationDetailActorRemoved() override;

		virtual bool CanProduce_Implementation() const override;

		UFUNCTION()
		virtual void OnCircuitChanged(UFGCircuitConnectionComponent* Component);

		UFUNCTION()
		virtual void OnMaxChanged() {
		};

		UFUNCTION()
		virtual void OnHasCoreChanged(bool HasCoreNewState) {
		};

		UFUNCTION()
		void OnTierUnlocked(int32 Tier);

		virtual void OnTierUpdated() {
		};

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		int32 GetTier() const;

		UFUNCTION(NetMulticast, Reliable)
		void MultiCast_OnNetworkCoreChanged(bool HasCore);

		UFUNCTION(BlueprintImplementableEvent, Category="KMods|NetworkEvents")
		void OnNetworkCoreChanged(bool HasCore);

		// Called every frame
		UFUNCTION(BlueprintPure, Category="KMods|Network")
		virtual bool IsCore() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		virtual UKPCLNetworkInfoComponent* GetNetworkInfoComponent() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		virtual UKPCLNetworkConnectionComponent* GetNetworkConnectionComponent() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		virtual UFGPowerInfoComponent* GetPowerInfoExplicit() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		virtual UFGPowerConnectionComponent* GetPowerConnectionExplicit() const;

		// START: AActor
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		// END: AActor


	protected:
		UPROPERTY(SaveGame, Replicated)
		AKPCLNetworkBuildingBase* mCore = nullptr;

		UPROPERTY(SaveGame, Replicated)
		int32 mUnlockedTier = 0;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Network")
		int32 mMaxTier = 4;

		virtual class AFGResourceSinkSubsystem* GetSinkSub();
		bool                                    bBindNetworkComponent = false;

		TQueue<FKPCLItemTransferQueue , EQueueMode::Mpsc> mInventoryQueue;
		TQueue<FKPCLSinkQueue , EQueueMode::Mpsc>         mSinkQueue;

		UPROPERTY()
		class UKPCLNetworkConnectionComponent* mNetworkConnection;

		UPROPERTY()
		class UKPCLNetworkInfoComponent* mNetworkInfoComponent;
};
