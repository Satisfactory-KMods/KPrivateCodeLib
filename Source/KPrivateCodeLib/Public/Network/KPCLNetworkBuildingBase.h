// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Buildable/KPCLProducerBase.h"
#include "Buildable/Modular/KPCLModularBuildingBase.h"
#include "KPCLNetworkBuildingBase.generated.h"

USTRUCT(BlueprintType)
struct FKPCLItemTransferQueue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mAddAmount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemAmount mAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mInventoryIndex = 0;

	bool IsValid() const
	{
		return mAmount.Amount > 0 && mInventoryIndex > INDEX_NONE && mAmount.ItemClass != nullptr;
	}
};

USTRUCT(BlueprintType)
struct FKPCLSinkQueue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemAmount mAmount;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkBuildingBase : public AKPCLModularBuildingBase, public IKPCLNetworkDataInterface
{
	GENERATED_BODY()

public:
	AKPCLNetworkBuildingBase();

	//Begin IKPCLNetworkDataInterface
	virtual bool HasCore_Implementation() const override;
	virtual AKPCLNetworkCore* GetCore_Implementation() const override;
	virtual UKPCLNetwork* GetNetwork_Implementation() const override;
	virtual FNetworkUIData GetUIDData_Implementation() const override;
	virtual FKPCLFaxitNetwork GetNetworkData_Implementation() const override;
	virtual bool HasCoreInNetwork_Implementation() const override;
	void SetNetworkCore(AKPCLNetworkCore* Core);

	virtual bool HasCore_Internal() const;
	virtual AKPCLNetworkCore* GetCore_Internal() const;
	virtual UKPCLNetwork* GetNetwork_Internal() const;

	virtual void OnNetworkDestoryed_Internal();
	virtual void OnNetworkAdded_Internal(AKPCLNetworkCore* Core);

	UPROPERTY(EditDefaultsOnly, Category="KMods|UI")
	FNetworkUIData mNetworkUIData;
	// End IKPCLNetworkDataInterface

	virtual void BeginPlay() override;
	virtual void Factory_Tick(float dt) override;
	virtual bool Factory_IsProducing() const override;
	virtual void TickNetwork(float dt, FKPCLFaxitNetwork* Network);
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;

	int32 SinkItems(FItemAmount Items);

	virtual void RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;
	virtual void UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;

	virtual bool CanProduce_Implementation() const override;

	UFUNCTION()
	virtual void OnCircuitChanged(UFGCircuitConnectionComponent* Component);

	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnNetworkCoreChanged(AKPCLNetworkCore* Core);

	UFUNCTION(BlueprintImplementableEvent, Category="KMods|NetworkEvents")
	void OnNetworkCoreChanged(AKPCLNetworkCore* Core);

	// Called every frame
	UFUNCTION(BlueprintPure, Category="KMods|Network")
	virtual bool IsCore() const;

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	virtual class UKPCLNetworkInfoComponent* GetNetworkInfoComponent() const;

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	virtual class UKPCLNetworkConnectionComponent* GetNetworkConnectionComponent() const;

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	virtual UFGPowerInfoComponent* GetPowerInfoExplicit() const;

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	virtual UFGPowerConnectionComponent* GetPowerConnectionExplicit() const;

	// START: AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// END: AActor

	UFUNCTION(BlueprintPure, Category="KMods|Network")
	TArray<FKPCLFaxitNetworkStatData> GetStates() const;
	
	virtual void GatherStates();
	virtual void OnTiersUpdated() {}
	FKPCLFaxitNetworkStatData* GetState(TSubclassOf<UFGItemDescriptor> Item);

protected:
	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	bool bHasCableBoost = false;
	
	UPROPERTY(SaveGame, Replicated)
	AKPCLNetworkCore* mNetworkCore = nullptr;

	virtual class AFGResourceSinkSubsystem* GetSinkSub();
	bool bBindNetworkComponent = false;

	UPROPERTY()
	class UKPCLNetworkConnectionComponent* mNetworkConnection;

	UPROPERTY()
	class UKPCLNetworkInfoComponent* mNetworkInfoComponent;
	
	UPROPERTY(Transient)
	class AKPCLFaxitSubsystem* mFaxitSubsystem = nullptr;
	
	UPROPERTY( SaveGame )
	TArray<FKPCLFaxitNetworkStatData> mCurrentStates;
	
	UPROPERTY( SaveGame, meta = ( FGReplicated ) )
	TArray<FKPCLFaxitNetworkStatData> mStates;
	
	UPROPERTY(EditDefaultsOnly, Category="KMods|Faxit")
	FSmartTimer mStateGatherTimer = FSmartTimer(60.f, false);
};
