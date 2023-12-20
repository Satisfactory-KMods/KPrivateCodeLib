// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGInventoryComponent.h"
#include "KPCLProducerBase.h"
#include "Buildables/FGBuildableFactory.h"
#include "Replication/FGReplicationDetailInventoryComponent.h"

#include "KPCLProducerBase_2Slots.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class KPRIVATECODELIB_API AKPCLProducerBase_2Slots: public AKPCLProducerBase {
	GENERATED_BODY()

	public:
		AKPCLProducerBase_2Slots();

		/** ----- Inventory Stuff ----- */
		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		UFGInventoryComponent* GetOutputInventory() const;

		FORCEINLINE virtual bool FilterOutputInventory(TSubclassOf<UObject> object, int32 idx) const { return true; }
		FORCEINLINE virtual bool FormFilterOutputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const { return true; }

		UFUNCTION()
		virtual void OnOutputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) {
		}

		UFUNCTION()
		virtual void OnOutputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) {
		}

		friend class AKPCLReplicationActor_ProducerBase;

		virtual void ReconfigureInventory() override;

		// set again the inventory on all Belts after and while init the replication actor
		virtual void SetBelts() override;
		/** ----- Inventory Stuff END ----- */
};
