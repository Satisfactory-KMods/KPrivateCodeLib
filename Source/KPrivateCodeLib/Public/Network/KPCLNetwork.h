// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGPowerCircuit.h"
#include "KPCLNetworkBuildingBase.h"
#include "Buildings/KPCLNetworkBuildingAttachment.h"
#include "Buildings/KPCLNetworkConnectionBuilding.h"
#include "Buildings/KPCLNetworkCoreModule.h"
#include "KPCLNetwork.generated.h"

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLNetwork: public UFGPowerCircuit {
	GENERATED_BODY()

	// START: UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// END: UObject

	virtual void TickCircuit(float dt) override;
	void         UpdateBytesInNetwork();
	virtual void OnCircuitChanged() override;

	public:
		void GetProccessorCapacity(int32& TotalInFluid, int32& TotalInSolid, int32& TotalOutFluid, int32& TotalOutSolid) const;

		UFUNCTION(BlueprintPure, Category = "Circuits|Network")
		bool NetworkHasCore() const;

		UFUNCTION(BlueprintPure, Category = "Circuits|Network")
		bool CoreStateIsOk() const;

		UFUNCTION(BlueprintPure, Category = "Circuits|Network")
		class AKPCLNetworkCore* GetCore() const;

		UFUNCTION(BlueprintPure, Category = "Circuits|Network")
		int32 GetBytes(bool AsFluid) const;

		/**
		 * Get the Capacity of the processor in the network
		 * @return count of running processors (max 8)
		 */
		UFUNCTION(BlueprintPure, Category = "Circuits|Network")
		int32 GetProcessorCapacity() const;

		UFUNCTION(BlueprintCallable)
		void GetAllTeleporter(TArray<AKPCLNetworkTeleporter*>& OtherTeleporter);

		bool IsNetworkDirty() const;

		TArray<AKPCLNetworkConnectionBuilding*>     GetNetworkConnectionBuildings();
		TArray<AKPCLNetworkBuildingAttachment*>		GetNetworkAttachments();
		TArray<AKPCLNetworkCoreModule*>				GetNetworkProcessors();

		UFUNCTION(BlueprintPure, Category="Network")
		int32 GetMaxInput(bool IsFluid = false) const;

		UFUNCTION(BlueprintPure, Category="Network")
		int32 GetMaxOutput(bool IsFluid = false) const;

	private:
		void ReBuildGroups();

		UPROPERTY(Replicated, SaveGame)
		AKPCLNetworkCore* mCoreBuilding = nullptr;

		UPROPERTY(Replicated, SaveGame)
		TArray<AKPCLNetworkTeleporter*> mConnectedTeleporterInNetwork;

		UPROPERTY(Replicated, SaveGame)
		TArray<AKPCLNetworkConnectionBuilding*> mConnectionBuildings;

		UPROPERTY(Replicated, SaveGame)
		TArray<AKPCLNetworkBuildingAttachment*> mConnectionAttachments;

		UPROPERTY(Replicated, SaveGame)
		TArray<AKPCLNetworkCoreModule*> mNetworkProcessors;

		UPROPERTY(Replicated, SaveGame)
		int32 mNetworkBytesForSolids = 0;

		UPROPERTY(Replicated, SaveGame)
		int32 mNetworkBytesForFluids = 0;

		UPROPERTY(Replicated, SaveGame)
		int32 mNetworkMaxInputSolid = 0;

		UPROPERTY(Replicated, SaveGame)
		int32 mNetworkMaxOutputSolid = 0;

		UPROPERTY(Replicated, SaveGame)
		int32 mNetworkMaxInputFluid = 0;

		UPROPERTY(Replicated, SaveGame)
		int32 mNetworkMaxOutputFluid = 0;

		TArray<TArray<UKPCLNetworkInfoComponent*>> mNetworkGroups;
		bool                                       bNetworkGroupsAreDirty = true;
		bool                                       bNetworkBuildingsAreDirty = true;
};
