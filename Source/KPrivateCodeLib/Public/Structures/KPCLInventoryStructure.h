#pragma once
#include "FGInventoryComponent.h"
#include "Buildables/FGBuildableFactory.h"
#include "KPCLInventoryStructure.generated.h"
/**
 * Helper Struc to handle Inventory easier for custom buildings.
 * Also redurce the object count by rip out the ReplicationDetailsComponent!
 */

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLInventoryStructure {
	public:
		GENERATED_BODY()

		FKPCLInventoryStructure();
		FKPCLInventoryStructure(FName ComponentName);
		FKPCLInventoryStructure(UFGInventoryComponent* InventoryComponent);

		bool operator==(const FKPCLInventoryStructure& Other) const {
			return mComponentName == Other.mComponentName;
		}

		//  -------------------  Getter  -------------------  //

		// Get the active inventory ( if Detailed valid then the Detailed otherwise the Main ( Server Only ) )
		UFGInventoryComponent* GetInventory() const;

		// Get the Detailed Inventory ( Server + Client if replicated (DetailedActor) active )
		UFGInventoryComponent* GetInventory_Detailed() const;

		// Get the Main Inventory ( Server Only )
		UFGInventoryComponent* GetInventory_Main() const;

		// Get Replication Detailed Actor
		AFGReplicationDetailActor* GetReplicationDetailedActor() const;

		// Is this Structure Valid?
		bool IsValid() const;

		// Get the Main Inventory ( Server Only )
		bool IsReplicated() const;

		//  -------------------  Setter  -------------------  //

		// Set and Flush the Replication Inventory
		void SetReplicated(UFGInventoryComponent* ReplicationComponent);

		// Set Size and resize the inventory
		void SetInventorySize(int32 Size);

		//  -------------------  Functions  -------------------  //

		// Init the Inventory Struct and handle the cration / reading of the inventory
		void InitInventory(AActor* Owner, FName ComponentName = FName());

		// Configure / Resize the Main Inventory
		void ConfigureInventory();

		// Clear and Flush the Replication Inventory
		void ClearReplication(bool PreventFlush = false);

		// Flush Main Inventory < Replication Inventory
		void FlushToOwner();

		// Flush Main Inventory > Replication Inventory
		void FlushToReplication();

		// Load from default Struct
		void LoadDefaultData(const FKPCLInventoryStructure& InventoryData);

		// Remove and clear the mInventoryComponent
		void RemoveInventory();

	private:
		// Has the owner of the Main inventory Auth?
		bool HasAuthority() const;

		// Get the owner of the Main Inventory
		AActor* GetStrucOwner() const;

		/**
		* Try to find a component with given ComponentName 
		* @param OutComponent Component that found if you put a valid in it will return true without reading Owner
		* @param Owner owner where we want to Search for
		* @param ComponentName if FName() it will return the first component of class otherwise find the component with the same name
		* @return true of OutComponent valid or false if Owner is invalid
		*/
		template<class T>
		static bool TryFindComponentOnOwner(T*& OutComponent, AActor* Owner, FName ComponentName = FName());

		// The Inventory (only Server can access to it)
		UPROPERTY(SaveGame, NotReplicated)
		UFGInventoryComponent* mInventoryComponent = nullptr;

		// Temp Inventory form the DetailedActor that will called / used if its valid. Will set and clear by the DetailedActor
		UPROPERTY(Transient)
		UFGInventoryComponent* mInventoryComponent_DetailedActor = nullptr;

		UPROPERTY(Transient)
		AFGReplicationDetailActor* mCachedReplicationDetailActor = nullptr;

	public:
		// Name of the Inventory Component (if default changed it will delete the old one)
		UPROPERTY(EditDefaultsOnly, SaveGame, NotReplicated)
		FName mComponentName = FName();

		// Cached Inventory size > SetInventorySize( int32 Size ); > Used by resizing on BeginPlay
		UPROPERTY(EditDefaultsOnly, SaveGame, NotReplicated)
		int32 mInventorySize = 1;

		// Should this inventory resize on begin play?
		UPROPERTY(EditDefaultsOnly, SaveGame, NotReplicated)
		bool mDontResizeOnBeginPlay = false;

		// Should we force to overwrite the saved mInventorySize to Default?
		UPROPERTY(EditDefaultsOnly, SaveGame, NotReplicated)
		bool mOverwriteSavedSize = true;
};

template<class T>
bool FKPCLInventoryStructure::TryFindComponentOnOwner(T*& OutComponent, AActor* Owner, FName ComponentName) {
	// Todo: do we need that can be find the wrong inventory for the Inventory Handler because wrong component name!
	//if( OutComponent != nullptr ) return true;

	if(!Owner) {
		return false;
	}

	if(ComponentName == FName()) {
		OutComponent = Owner->FindComponentByClass<T>();
		return OutComponent != nullptr;
	}

	TArray<T*> AllComponents;
	Owner->GetComponents(AllComponents);
	for(T* Component: AllComponents) {
		if(Component->GetFName() == ComponentName) {
			OutComponent = Component;
			break;
		}
	}

	return OutComponent != nullptr;
}


UCLASS()
class KPRIVATECODELIB_API UKPCLInventoryLibrary: public UBlueprintFunctionLibrary {
	GENERATED_BODY()

	public:
		/**
		*@return the active component from the FKPCLInventoryStructure ( not the main component )
		*/
		UFUNCTION(BlueprintPure, Category="KMods|Inventory")
		static UFGInventoryComponent* GetInventory(FKPCLInventoryStructure InventoryData);

		/**
		*@return the Main component from the FKPCLInventoryStructure
		*/
		UFUNCTION(BlueprintPure, Category="KMods|Inventory")
		static UFGInventoryComponent* GetInventory_Main(FKPCLInventoryStructure InventoryData);

		/**
		*@return the Detailed component from the FKPCLInventoryStructure
		*/
		UFUNCTION(BlueprintPure, Category="KMods|Inventory")
		static UFGInventoryComponent* GetInventory_Detailed(FKPCLInventoryStructure InventoryData);

		/**
		*@return the ReplicationDetailActor from the FKPCLInventoryStructure
		*/
		UFUNCTION(BlueprintPure, Category="KMods|Inventory")
		static AFGReplicationDetailActor* GetReplicationDetailActor(FKPCLInventoryStructure InventoryData);

		/**
		*@return return true if GetInventory_Detailed() is true and has a replicated inventory
		*/
		UFUNCTION(BlueprintPure, Category="KMods|Inventory")
		static bool IsReplicated(FKPCLInventoryStructure InventoryData);

		/**
		*@return true if the Inventory and owner (on server) valid
		*/
		UFUNCTION(BlueprintPure, Category="KMods|Inventory")
		static bool IsValid(FKPCLInventoryStructure InventoryData);
};
