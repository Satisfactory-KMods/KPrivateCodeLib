// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Resources/FGItemDescriptor.h"
#include "UObject/Interface.h"
#include "KPCLNetworkDataInterface.generated.h"


UENUM(BlueprintType)
enum class ECoreDataSortOption : uint8 {
	index UMETA(DisplayName = "Index"),
	revindex UMETA(DisplayName = "Rev Index"),
	alphab UMETA(DisplayName = "Alpha"),
	revalphab UMETA(DisplayName = "Rev Alpha"),
	form UMETA(DisplayName = "Form"),
	revform UMETA(DisplayName = "Rev Form"),
	formalphab UMETA(DisplayName = "Form Alpha"),
	revformalphab UMETA(DisplayName = "Rev Form Alpha"),
};

UENUM(BlueprintType)
enum class ECoreDataShowOption : uint8 {
	NONE UMETA(DisplayName = "NONE"),
	all UMETA(DisplayName = "All"),
	fluid UMETA(DisplayName = "Fluid"),
	solid UMETA(DisplayName = "Solid")};

USTRUCT(BlueprintType)
struct FCoreDataSortOptionStruc {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString mNameFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoreDataSortOption mSorting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoreDataShowOption mShow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mFilteredUnlocked = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mShowEmptySlots = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mSinkableFilter = false;
};

USTRUCT(BlueprintType)
struct FCoreInventoryData {
	GENERATED_BODY()

	FCoreInventoryData() = default;

	FCoreInventoryData(TSubclassOf<UFGItemDescriptor> Item, int32 InventoryIndex): mItemForm() {
		mItem = Item;
		mInventoryIndex = InventoryIndex;
	}

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItem;

	UPROPERTY(BlueprintReadOnly)
	EResourceForm mItemForm;

	UPROPERTY(BlueprintReadOnly)
	int32 mInventoryIndex;

	bool operator==(TSubclassOf<UFGItemDescriptor> other) const {
		return mItem == other;
	};
};

USTRUCT(BlueprintType)
struct FNetworkUIData {
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mShowInventory = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanTransferItems = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanShowNetworkStats = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mOnlyShowSinkableItems = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanSwitchForm = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanSetMaxAmount = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ECoreDataShowOption mForceForm = ECoreDataShowOption::NONE;
};

// This class does not need to be modified.
UINTERFACE()
class UKPCLNetworkDataInterface: public UInterface {
	GENERATED_BODY()
};

/**
 * 
 */
class KPRIVATECODELIB_API IKPCLNetworkDataInterface {
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
	public:
		UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
		class AKPCLNetworkCore* GetCore() const;

		UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
		bool HasCore() const;

		UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
		class UKPCLNetwork* GetNetwork() const;

		UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
		FNetworkUIData GetUIDData() const;
};
