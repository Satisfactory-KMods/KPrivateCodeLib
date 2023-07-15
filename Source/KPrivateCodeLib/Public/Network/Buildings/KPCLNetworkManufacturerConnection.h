// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGResourceSinkSubsystem.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "KPCLNetworkManufacturerConnection.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkManufacturerConnection: public AKPCLNetworkBuildingBase {
	GENERATED_BODY()

	public:
		AKPCLNetworkManufacturerConnection();

		// START: AActor
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		// END: AActor

		virtual void BeginPlay() override;

		virtual void Factory_Tick(float dt) override;
		bool         GetNeededResourceClassesThisFrame(TArray<FItemAmount>& Items) const;
		bool         PushToNetwork(TArray<FItemAmount>& ToPush, float MaxSolidBytes, float MaxFluidBytes);
		bool         GetFromNetwork(const TArray<FItemAmount>& ToReceive);

		UFUNCTION(BlueprintImplementableEvent)
		void BP_OnRecipeChanged(TSubclassOf<UFGRecipe> NewRecipe);

		UFUNCTION(BlueprintImplementableEvent)
		void BP_OnNewManufacturerSet(AFGBuildableManufacturer* NewManufacturer);
		void SetManufacturer(AFGBuildableManufacturer* NewManufacturer);

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		bool GetIsAllowedToSinkOverflow() const;

		UFUNCTION(BlueprintCallable, Category="KMods|Network")
		void SetIsAllowedToSinkOverflow(bool IsAllowed);

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		bool IsSafeToModifyInventorys() const;

	private:
		virtual AFGResourceSinkSubsystem* GetSinkSub() override;
		void                              SetNewRecipe(TSubclassOf<UFGRecipe> NewRecipe);

		UFUNCTION()
		void OnRep_Manufacturer();

		UFUNCTION()
		void OnRep_Recipe();

		UPROPERTY(SaveGame, Replicated)
		bool mSinkOverflow = false;

		UPROPERTY(SaveGame)
		int32 mStartIndexOutput = -1;

		UPROPERTY(SaveGame, Replicated, ReplicatedUsing=OnRep_Recipe)
		TSubclassOf<UFGRecipe> mLastRecipe = nullptr;

		UPROPERTY(SaveGame, Replicated, ReplicatedUsing=OnRep_Manufacturer)
		AFGBuildableManufacturer* mManufacturer;

		UPROPERTY(Transient)
		AFGResourceSinkSubsystem* mSinkSub;

		UPROPERTY(SaveGame)
		FSmartTimer mCycleTimer;
};
