// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "KPCLNetworkStorageUnit.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkStorageUnit: public AKPCLNetworkBuildingBase {
	GENERATED_BODY()

	public:
		// Sets default values for this actor's properties
		AKPCLNetworkStorageUnit();
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

		UFUNCTION(BlueprintNativeEvent)
		void OnDrivesUpdated();

		UFUNCTION()
		void OnRep_DriveCount();

	protected:
		virtual void BeginPlay() override;
		void         InitNetwork();

		virtual void OnMasterBuildingReceived_Implementation(AActor* Actor) override;
		virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;
		virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) override;
		virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) override;

	private:
		UPROPERTY(EditDefaultsOnly, Category="KMods|Network")
		TMap<int32 , UStaticMesh*> mCountToMeshMap;

		UPROPERTY(Replicated, ReplicatedUsing=OnRep_DriveCount)
		int32 mDriveCount = 0;

	protected:
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Network")
		bool mIsFluidStorage = false;
};
