// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGResourceSinkSubsystem.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "Resources/FGNoneDescriptor.h"
#include "KPCLNetworkConnectionBuilding.generated.h"

USTRUCT(BlueprintType)
struct FNetworkConnectionInformations {
	GENERATED_BODY()

	public:
		UPROPERTY(SaveGame, BlueprintReadOnly)
		TSubclassOf<UFGItemDescriptor> mItemsToGrab = UFGNoneDescriptor::StaticClass();

		UPROPERTY(BlueprintReadOnly)
		bool bIsInput = false;

		UPROPERTY(BlueprintReadOnly)
		EResourceForm mForm = EResourceForm::RF_SOLID;

		bool CanPush() const {
			return !bIsInput && mItemsToGrab && mItemsToGrab != UFGNoneDescriptor::StaticClass();
		}

		int32 GetBufferSize() const {
			switch(mForm) {
				case EResourceForm::RF_LIQUID:
				case EResourceForm::RF_GAS: return mNetworkConnectionFluidBufferSize;
				default: return mNetworkConnectionSolidBufferSize;
			}
		}

		inline static int32 mNetworkConnectionFluidBufferSize = 1000;
		inline static int32 mNetworkConnectionSolidBufferSize = 2;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkConnectionBuilding: public AKPCLNetworkBuildingBase {
	GENERATED_BODY()

	public:
		AKPCLNetworkConnectionBuilding();

		// START: AActor
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		// END: AActor

		virtual bool FormFilterInputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const override;

		virtual void BeginPlay() override;

		virtual void Factory_Tick(float dt) override;
		virtual void CollectBelts() override;
		virtual void CollectAndPushPipes(float dt, bool IsPush) override;

		virtual void OnMaxChanged() override;
		virtual void OnSinkOverflowChanged();

		bool         GetStack(FInventoryStack& Stack) const;
		virtual bool TryToReceiveItems(FInventoryStack& Stack);
		virtual bool TryToGrabItems(FInventoryStack& Stack, float MaxSolidBytes, float MaxFluidBytes);

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		void GetConnectionInformations(FNetworkConnectionInformations& Informations) const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		int32 GetMaxItemCount() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		int32 GetMaxItemCountPure() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		int32 GetManuellItemCountMax() const;

		UFUNCTION(BlueprintCallable, Category="KMods|Network")
		void SetManuellItemMax(int32 Value = -1);

		UFUNCTION(BlueprintCallable, Category="KMods|Network")
		virtual void SetGrabItem(TSubclassOf<UFGItemDescriptor> Item);

		TSubclassOf<UFGItemDescriptor>    PeekItemClass() const;
		virtual AFGResourceSinkSubsystem* GetSinkSub() override;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		bool GetIsAllowedToSinkOverflow() const;

		UFUNCTION(BlueprintCallable, Category="KMods|Network")
		void SetIsAllowedToSinkOverflow(bool IsAllowed);

	private:
		friend class AKPCLNetworkSink;

		UPROPERTY(SaveGame)
		FSmartTimer mResetTimer = FSmartTimer(0.01666667);

		UPROPERTY(SaveGame, Replicated)
		FNetworkConnectionInformations mInformations;

		UPROPERTY(SaveGame)
		int32 mItemCount;

		UPROPERTY(SaveGame, Replicated)
		int32 mManuellItemCountMax = -1;

		UPROPERTY(SaveGame, Replicated)
		bool mSinkOverflow = false;
		bool CachedSinkOverflow = false;

		UPROPERTY(Transient)
		AFGResourceSinkSubsystem* mSinkSub;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Network")
		float mExtraPowerConsumeWithSinkOverflow = 5.0f;
};
