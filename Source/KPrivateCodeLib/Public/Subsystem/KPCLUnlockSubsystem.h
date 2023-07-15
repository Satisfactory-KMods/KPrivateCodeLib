// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "KPCLModSubsystem.h"
#include "Description/KPCLEndlessShopItem.h"
#include "Description/Decor/KPCLDecorationActorData.h"
#include "Looting/KPCLLootChest.h"
#include "Network/Buildings/KPCLNetworkCore.h"

#include "KPCLUnlockSubsystem.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkTierUnlocked, int32, Tier);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPointsUpdated, int64, mNewPointCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndlessShoppingItemsUnlocked, const TArray< TSubclassOf<UKPCLEndlessShopItem> >, UnlockedItems);

UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API AKPCLUnlockSubsystem: public AKPCLModSubsystem {
	GENERATED_BODY()

	AKPCLUnlockSubsystem();

	protected:
		virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
		void         DequeuePoints();
		void         GenerateCategoryMap();

	public:
		void AddPoints(int64 Points);
		void AddPoints_ThreadSafe(int64 Points);

		UFUNCTION()
		void OnRep_PointsUpdated();

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

	public:
		void UnlockDecorations(TArray<TSubclassOf<UKPCLDecorationRecipe>> Decorations);

		UFUNCTION()
		void OnRep_Decoration();

		UFUNCTION(BlueprintCallable, Category="KMods|Decoration")
		void Decor_GetAllMainCats(TArray<TSubclassOf<UKPCLDecorMainCategory>>& MainCats);

		UFUNCTION(BlueprintCallable, Category="KMods|Decoration")
		void Decor_GetAllSubCatsOfMainCat(TSubclassOf<UKPCLDecorMainCategory> MainCat, TArray<TSubclassOf<UKPCLDecorSubCategory>>& SubCats);

		UFUNCTION(BlueprintCallable, Category="KMods|Decoration")
		TArray<TSubclassOf<UKPCLDecorationRecipe>> Decor_GetAllItemsOfSubCat(TSubclassOf<UKPCLDecorMainCategory> MainCat, TSubclassOf<UKPCLDecorSubCategory> SubCat);

	public:
		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		bool IsEndlessShopItemUnlocked(TSubclassOf<UKPCLEndlessShopItem> InClass) const;

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		void UnlockEndlessShopItems(AFGCharacterPlayer* Player, TArray<TSubclassOf<UKPCLEndlessShopItem>> UnlockingItems);
		bool UnlockEndlessShopItem(AFGCharacterPlayer* Player, TSubclassOf<UKPCLEndlessShopItem> UnlockingItem);
		void ApplyEndlessShopItem(TSubclassOf<UKPCLEndlessShopItem> UnlockingItem);

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		void ClearShoppingList();

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		int64 GetCurrentPoints() const;

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		bool AddToShoppingList(TSubclassOf<UKPCLEndlessShopItem> UnlockingItem);

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		void RemoveFromShoppingList(TSubclassOf<UKPCLEndlessShopItem> UnlockingItem);

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		bool CanBuyShoppingList(AFGCharacterPlayer* Player, TArray<TSubclassOf<UKPCLEndlessShopItem>> ShoppingList) const;

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		TArray<TSubclassOf<UKPCLEndlessShopItem>> GetShoppingList(bool FilterUnlockable = false) const;

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		TArray<TSubclassOf<UKPCLEndlessShopItem>> GetUnlockedEndlessShopItems() const;

		UFUNCTION(NetMulticast, Reliable)
		void MultiCast_UnlockedItems(const TArray<TSubclassOf<UKPCLEndlessShopItem>>& UnlockedItems);

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		void ES_GetAllMainCats(TArray<TSubclassOf<UKPCLEndlessShopMainCategory>>& MainCats);

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		void ES_GetAllSubCatsOfMainCat(TSubclassOf<UKPCLEndlessShopMainCategory> MainCat, TArray<TSubclassOf<UKPCLEndlessShopSubCategory>>& SubCats);

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		TArray<TSubclassOf<UKPCLEndlessShopItem>> ES_GetAllItemsOfSubCat(TSubclassOf<UKPCLEndlessShopMainCategory> MainCat, TSubclassOf<UKPCLEndlessShopSubCategory> SubCat);

		// Events
		UPROPERTY(BlueprintAssignable)
		FOnEndlessShoppingItemsUnlocked OnEndlessShoppingItemsUnlocked;

		UPROPERTY(BlueprintAssignable, Category="KMods|Network")
		FOnPointsUpdated OnPointsUpdated;

		UFUNCTION(BlueprintImplementableEvent, DisplayName="OnEndlessShoppingItemsUnlocked")
		void BP_OnEndlessShoppingItemsUnlocked(const TArray<TSubclassOf<UKPCLEndlessShopItem>>& UnlockedItems);

	private:
		UPROPERTY(EditDefaultsOnly, SaveGame, ReplicatedUsing=OnRep_PointsUpdated, Category="KMods|EndlessShop")
		int64 mCurrentPoints = 1000;

		UPROPERTY(EditDefaultsOnly, SaveGame, Category="KMods|EndlessShop")
		FSmartTimer mTimeToGetPassivePoints = FSmartTimer(5.f);

		UPROPERTY(EditDefaultsOnly, Category="KMods|EndlessShop")
		int64 mPassivePoints = 5;

		UPROPERTY(EditDefaultsOnly, SaveGame, Category="KMods|EndlessShop")
		TSubclassOf<UFGSchematic> mSchematicToUnlockPassivPoints;

		bool bPassiveIsUnlocked = false;

	private:
		TMap<TSubclassOf<UKPCLEndlessShopMainCategory> , TMap<TSubclassOf<UKPCLEndlessShopSubCategory> , TArray<TSubclassOf<UKPCLEndlessShopItem>>>> mCategoryMap;
		TMap<TSubclassOf<UKPCLDecorMainCategory> , TMap<TSubclassOf<UKPCLDecorSubCategory> , TArray<TSubclassOf<UKPCLDecorationRecipe>>>>            mDecorationCategoryMap;

		TQueue<int64 , EQueueMode::Mpsc> mPointQueue;

		UPROPERTY(Replicated, SaveGame)
		TArray<TSubclassOf<UFGSchematic>> mUnlockedNetworkTiers;

		UPROPERTY(SaveGame)
		TArray<TSubclassOf<UKPCLEndlessShopItem>> mShoppingList;

		UPROPERTY(Replicated, SaveGame)
		TArray<TSubclassOf<UKPCLEndlessShopItem>> mUnlockedShopItems;

		UPROPERTY(ReplicatedUsing=OnRep_Decoration, SaveGame)
		TArray<TSubclassOf<UKPCLDecorationRecipe>> mUnlockedDecorations;

		UPROPERTY()
		TArray<TSubclassOf<UKPCLEndlessShopItem>> mAllEndlessShopItem;

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
};
