#pragma once

#include "CoreMinimal.h"
#include "FGCentralStorageSettings.h"
#include "FGResourceSinkSubsystem.h"
#include "KPCLNetworkCore.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "Resources/FGNoneDescriptor.h"
#include "KPCLNetworkConnectionBuilding.generated.h"


UENUM(BlueprintType)
enum class EKPCLOverflowMode : uint8
{
	Ignore,
	Sink,
	Depot,
	DepotAndSink
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkConnectionBuilding : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkConnectionBuilding();

	// START: AActor
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// END: AActor
	
	// START: FGBuildable
	virtual bool CanProduce_Implementation() const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	// END: FGBuildable

protected:
	virtual void TickNetwork(float dt, FKPCLFaxitNetwork* Network) override;

	virtual void EndProductionTime() override;
	virtual void onProducingFinal_Implementation() override;
	
	friend class AKPCLNetworkSink;

	virtual void SetBelts() override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void Server_DoFlush() override;
	
	void UpdateInventoryState();
	void UploadToDepot();
	void Sink();
	void UpdateProductionSpeed();

	virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved, UFGInventoryComponent* sourceInventory) override;
	virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved, UFGInventoryComponent* sourceInventory) override;

	void UpdateInventoryFilter();

public:
	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool CanUploadStorage() const;
	
	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool StorageIsEmpty() const;
	
	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	EKPCLOverflowMode GetOverflowMode() const;
	
	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void SetOverflowMode(EKPCLOverflowMode NewMode);
	
	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void ClearSpeedOverride();
	
	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void SetSpeedOverride(float NewSpeed);
	
	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void SetFilterItem(TSubclassOf<UFGItemDescriptor> NewItem);
	
	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	TSubclassOf<UFGItemDescriptor> GetFilterItem() const;
	TSubclassOf<UFGItemDescriptor> GetStoredItemClass() const;

private:
	UPROPERTY( SaveGame, meta = ( FGReplicated ) )
	EKPCLOverflowMode mOverflowMode = EKPCLOverflowMode::Ignore;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mInventory;

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	TSubclassOf<UFGItemDescriptor> mFilterItem;

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	float mSpeedOverride = -1.f;

	UPROPERTY()
	TMap<TSubclassOf<UFGItemDescriptor>, FItemAmount> mCurrentStateCache;
	int32 mItemAmount = 1;
	int32 mFluidAmount = 1000;
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	bool mIsUpload = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	EResourceForm mItemForm = EResourceForm::RF_SOLID;

protected:
	UPROPERTY(Transient)
	AFGCentralStorageSubsystem* mCentralStorageSubsystem;
};
