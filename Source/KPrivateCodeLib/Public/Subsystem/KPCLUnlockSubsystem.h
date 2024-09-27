// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "KPCLModSubsystem.h"
#include "Network/Buildings/KPCLNetworkCore.h"

#include "KPCLUnlockSubsystem.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkTierUnlocked, int32, Tier);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPointsUpdated, int64, mNewPointCount);

UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API AKPCLUnlockSubsystem: public AKPCLModSubsystem {
	GENERATED_BODY()

	AKPCLUnlockSubsystem();

	public:
		UFUNCTION(BlueprintPure, Category = "Subsystem", DisplayName = "GetKPCLUnlockSubsystem", meta = ( DefaultToSelf = "worldContext" ))
		static AKPCLUnlockSubsystem* Get(UObject* worldContext);

		/** Start Replication */
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		/** End Replication */

		virtual void BeginPlay() override;

		UFUNCTION()
		void OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic);

		virtual void Init() override;

		virtual void Tick(float DeltaSeconds) override;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		int32 GetNetworkTier() const;

		void UnlockNetworkTier(TSubclassOf<UFGSchematic> BoundedSchematic);

		// Helper for Nexus
		void OnNexusConstruct(AKPCLNetworkCore* Nexus);
		void OnNexusDeconstruct(AKPCLNetworkCore* Nexus);

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		int32 GetGlobalNexusCount() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		int32 GetMaxGlobalNexusCount() const;

		UFUNCTION(BlueprintPure, Category="KMods|Network")
		TArray<AKPCLNetworkCore*> GetAllNexusInTheWorld() const;

		void RegisterPlayerState(AFGPlayerState* State);

		UPROPERTY(BlueprintAssignable, Category="KMods|Network")
		FOnNetworkTierUnlocked OnNetworkTierUnlocked;

	private:
		UPROPERTY(EditDefaultsOnly, SaveGame, Category="KMods|EndlessShop")
		FSmartTimer mTimeToGetPassivePoints = FSmartTimer(5.f);

		UPROPERTY(EditDefaultsOnly, Category="KMods|EndlessShop")
		int64 mPassivePoints = 5;

		UPROPERTY(EditDefaultsOnly, SaveGame, Category="KMods|EndlessShop")
		TSubclassOf<UFGSchematic> mSchematicToUnlockPassivPoints;

		bool bPassiveIsUnlocked = false;

	private:
		UPROPERTY(SaveGame)
		TArray<AFGPlayerState*> mPlayerStates;

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkSystem")
		TSubclassOf<class UKPCLNetworkPlayerComponent> mStateComponentClass;

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkSystem")
		int32 mFluidItemsPerBytes;

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkSystem")
		int32 mSolidItemsPerBytes;

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkSystem")
		int32 mNetworkConnectionFluidBufferSize;

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkSystem")
		int32 mNetworkConnectionSolidBufferSize;

		UPROPERTY(EditDefaultsOnly, Category="KMods|NetworkSystem")
		int32 mNetworkGlobalNexusMaxCount = 4;

		UPROPERTY(Replicated)
		TArray<AKPCLNetworkCore*> mBuildedNexus;
		
		UPROPERTY(Replicated, SaveGame)
		TArray<TSubclassOf<UFGSchematic>> mUnlockedNetworkTiers;
};
