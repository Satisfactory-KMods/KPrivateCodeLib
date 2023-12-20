// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGCategory.h"
#include "KPCLRegistryObject.h"
#include "UObject/Object.h"
#include "KPCLEndlessShopItem.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLEndlessShopMainCategory: public UFGCategory {
	GENERATED_BODY()
};

UCLASS()
class KPRIVATECODELIB_API UKPCLEndlessShopSubCategory: public UFGCategory {
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FKPCLEndlessShopItemInformation {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mIsUnlockable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mIsUnlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UFGSchematic>> mAllDependencies = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UFGSchematic>> mMissingDependencies = {};
};

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLEndlessShopItem: public UKPCLRegistryObject {
	GENERATED_BODY()

	public:
		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static TArray<TSubclassOf<UFGRecipe>> GetRecipesToUnlock(TSubclassOf<UKPCLEndlessShopItem> InClass);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static TArray<TSubclassOf<UFGSchematic>> GetSchematicsToUnlock(TSubclassOf<UKPCLEndlessShopItem> InClass);

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		static void GetCosts(TSubclassOf<UKPCLEndlessShopItem> InClass, TArray<FItemAmount>& Items, int64& Points);

		UFUNCTION(BlueprintCallable, Category="KMods|EndlessShop")
		static void GetListCosts(TArray<TSubclassOf<UKPCLEndlessShopItem>> UnlockingItems, TArray<FItemAmount>& Items, int64& Points);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static FText GetName(TSubclassOf<UKPCLEndlessShopItem> InClass);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static FText GetDescription(TSubclassOf<UKPCLEndlessShopItem> InClass);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static FSlateBrush GetSlate(TSubclassOf<UKPCLEndlessShopItem> InClass);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static TArray<TSubclassOf<UKPCLEndlessShopItem>> GetSeeAlso(TSubclassOf<UKPCLEndlessShopItem> InClass);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop", Meta = ( WorldContext = "WorldContext" ))
		static FKPCLEndlessShopItemInformation GetInformation(TSubclassOf<UKPCLEndlessShopItem> InClass, UObject* WorldContext);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static TSubclassOf<UKPCLEndlessShopMainCategory> GetMainCategory(TSubclassOf<UKPCLEndlessShopItem> InClass);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static TSubclassOf<UKPCLEndlessShopSubCategory> GetSubCategory(TSubclassOf<UKPCLEndlessShopItem> InClass);

		UFUNCTION(BlueprintPure, Category="KMods|EndlessShop")
		static bool IsShopItemsIsHidden(TSubclassOf<UKPCLEndlessShopItem> InClass);

	protected:
		virtual TArray<TSubclassOf<UFGRecipe>>    GetRecipesToRegister() override;
		virtual TArray<TSubclassOf<UFGSchematic>> GetSchematicsToRegister() override;

	private:
		UPROPERTY(EditDefaultsOnly, Category = "UI")
		FText mDisplayName;

		UPROPERTY(EditDefaultsOnly, Category = "UI")
		FText mDescription;

		UPROPERTY(EditDefaultsOnly, Category = "UI")
		TSubclassOf<UKPCLEndlessShopMainCategory> mMainCategory;

		UPROPERTY(EditDefaultsOnly, Category = "UI")
		TSubclassOf<UKPCLEndlessShopSubCategory> mSubCategory;

		UPROPERTY(EditDefaultsOnly, Category = "UI")
		FSlateBrush mIconSlate;

		UPROPERTY(EditDefaultsOnly, Category = "UI")
		TArray<TSubclassOf<UKPCLEndlessShopItem>> mSeeAlso;

		UPROPERTY(EditDefaultsOnly, Category="Unlock")
		TArray<TSubclassOf<UFGRecipe>> mRecipesToUnlock;

		UPROPERTY(EditDefaultsOnly, Category="Unlock")
		TArray<TSubclassOf<UFGSchematic>> mSchematicsToUnlock;

		UPROPERTY(EditDefaultsOnly, Category="Unlock")
		int32 mPointCost;

		UPROPERTY(EditDefaultsOnly, Category="Unlock")
		TArray<FItemAmount> mItemCosts;

		UPROPERTY(EditDefaultsOnly, Category="Dependencies")
		TArray<TSubclassOf<UFGSchematic>> mDependencieSchematics;
};
