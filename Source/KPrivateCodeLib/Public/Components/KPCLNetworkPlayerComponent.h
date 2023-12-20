// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGPlayerState.h"
#include "Interfaces/KPCLNetworkDataInterface.h"
#include "Structures/KPCLFunctionalStructure.h"
#include "KPCLNetworkPlayerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDistanceUpdated, int32, Distance);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRulesUpdated);

USTRUCT(BlueprintType)
struct FKPCLPlayerInventoryRules {
	GENERATED_BODY()

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FItemAmount mItemAmount;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	bool mShouldPullFromNetwork;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mUIOnly_Index = -1;

	bool IsValid() const {
		return mItemAmount.Amount >= 0 && mItemAmount.ItemClass != nullptr;
	}
};

UENUM(BlueprintType)
enum EKPCLPlayerRuleFilter {
	All,
	Push,
	Pull
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class KPRIVATECODELIB_API UKPCLNetworkPlayerComponent: public UActorComponent, public IFGSaveInterface {
	GENERATED_BODY()

	protected:
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		virtual void BeginPlay() override;
		virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;

		virtual bool ShouldSave_Implementation() const override;

	private:
		void DoDistanceCheck();

		void ReceiveTickFromNetwork(AKPCLNetworkCore* Core);
		void ReValidSchematics();

		UFUNCTION()
		void OnSchematicUnlocked(TSubclassOf<UFGSchematic> Schematic);

	public:
		void CustomTick(float dt);

		UFUNCTION(BlueprintCallable, Category="KMods|Network", meta = ( WorldContext = "WorldContextObject" ))
		static UKPCLNetworkPlayerComponent* GetOrCreateNetworkComponentToPlayerState(UObject* WorldContextObject, AFGPlayerState* State, TSubclassOf<UKPCLNetworkPlayerComponent> ComponentClass = nullptr);

		// Helper Functions Start
		UFUNCTION(BlueprintPure, Category="KMods|Network")
		FVector GetPlayerCharacterLocation() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		AFGPlayerState* GetPlayerState() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		AFGCharacterPlayer* GetPlayerCharacter() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		bool IsPlayerDead() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		APawn* GetPlayerPawn() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		bool IsStateActive() const;

		/**
		* @return false if the OutInventory is Invalid or the State is not active!
		*/
		UFUNCTION(BlueprintPure, Category="Network")
		bool GetPlayerCharacterInventory(UFGInventoryComponent*& OutInventory);
		// Helper Functions End

		/**
		* @return true if we are available to access a network
		*/
		UFUNCTION(BlueprintPure, Category="Network")
		bool DistanceAccessUnlocked() const;

		/**
		* @return true if we are available to auto pull items from the nearest network
		*/
		UFUNCTION(BlueprintPure, Category="Network")
		bool PullLogicUnlocked() const;

		/**
		* @return true if we are available to auto push items from the nearest network
		*/
		UFUNCTION(BlueprintPure, Category="Network")
		bool PushLogicUnlocked() const;

		/**
		* @return true if we are available access a network
		*/
		UFUNCTION(BlueprintPure, Category="Network")
		bool CanAccessTo() const;

		/**
		* @return true if we are available to auto push items from the nearest network
		*/
		UFUNCTION(BlueprintPure, Category="Network")
		bool SinkOverFlowUnlocked() const;

		UFUNCTION(BlueprintPure, Category="Network")
		int32 GetDistanceStrength() const;

		UFUNCTION(BlueprintPure, Category="Network")
		AFGBuildable* GetNextBuilding() const;

		template<class T>
		T* GetNextBuilding_Native() const;

		UFUNCTION(BlueprintCallable, Category="Network")
		TArray<FKPCLPlayerInventoryRules> GetRules(EKPCLPlayerRuleFilter Filter = All) const;

		UFUNCTION(BlueprintCallable, Category="Network")
		void AddRule(FKPCLPlayerInventoryRules Rule);

		UFUNCTION(BlueprintCallable, Category="Network")
		void RemoveRule(int32 RuleIndex);

		UFUNCTION(BlueprintCallable, Category="Network")
		void EditRule(int32 RuleIndex, FKPCLPlayerInventoryRules Rule);

		UPROPERTY(BlueprintAssignable, Category="Network")
		FOnDistanceUpdated mOnDistanceUpdated;

		UPROPERTY(BlueprintAssignable, Category="Network")
		FOnRulesUpdated mOnRulesUpdated;

	protected:
		UFUNCTION()
		void OnRep_DistanceUpdated();

		UFUNCTION()
		void OnRep_RulesUpdated();

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkDistanceManager")
		float mMaxDistance = 10000.f;

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkDistanceManager")
		FSmartTimer mDistanceCheckTimer = FSmartTimer(.25f);

		UPROPERTY(EditDefaultsOnly, Category="KMods|SphereCheck")
		TArray<TEnumAsByte<EObjectTypeQuery>> mObjectTypes = TArray<TEnumAsByte<EObjectTypeQuery>>{ObjectTypeQuery1, ObjectTypeQuery2};

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkPullAndPushManager")
		FSmartTimer mTimerForPushAndPullLogic = FSmartTimer(.25f);

		UPROPERTY(EditDefaultsOnly, Category="KMods|Permissions")
		TSubclassOf<UFGSchematic> mSchematicForOverflowSink;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Permissions")
		TSubclassOf<UFGSchematic> mSchematicsToUnlockDistanceAccess;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Permissions")
		TSubclassOf<UFGSchematic> mSchematicsToUnlockAutoPull;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Permissions")
		TSubclassOf<UFGSchematic> mSchematicsToUnlockAutoPush;

	private:
		// we want that the Core is friend because we want that he QUEUE items to pull or push;
		friend AKPCLNetworkCore;

		UPROPERTY(Replicated)
		UFGInventoryComponent* mCachedPlayerCharacterInventory;

		UPROPERTY(SaveGame, Replicated)
		TArray<TSubclassOf<UFGSchematic>> mUnlockedPermissions;
		TArray<TSubclassOf<UFGSchematic>> mUnlockableSchematics;

		// Begin Replications
		UPROPERTY(Replicated, ReplicatedUsing=OnRep_DistanceUpdated)
		int32 mDistanceStrength = 0;

		UPROPERTY(Replicated)
		AKPCLNetworkCore* mNextCore;

		UPROPERTY(Replicated)
		AFGBuildable* mNextBuilding;

		UPROPERTY(Replicated, ReplicatedUsing=OnRep_RulesUpdated, SaveGame)
		TArray<FKPCLPlayerInventoryRules> mInventoryRules;

		UPROPERTY(Replicated, SaveGame)
		bool mIsAllowedToSetOverflow = false;
};

template<class T>
T* UKPCLNetworkPlayerComponent::GetNextBuilding_Native() const {
	return Cast<T>(GetNextBuilding());
}
