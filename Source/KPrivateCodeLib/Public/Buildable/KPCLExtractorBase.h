// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KPCLProducerBase.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "KPCLExtractorBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class KPRIVATECODELIB_API AKPCLExtractorBase: public AFGBuildableResourceExtractor, public IFGRecipeProducerInterface {
	GENERATED_BODY()

	public:
		AKPCLExtractorBase();

#if WITH_EDITOR
		virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
		virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

		// START: AActor
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		virtual bool ShouldSave_Implementation() const override;
		virtual void BeginPlay() override;
		// END: AActor

		// START: Mod Dependencies
		virtual void CheckingModDependencies();

		virtual void OnBuildEffectFinished() override;
		virtual void OnBuildEffectActorFinished() override;
		virtual void ReadyForVisuelUpdate();

		bool        mNeedModDependencies = false;
		TSet<FName> mModRefsToCheck;
		// END: Mod Dependencies

		// START: AudioConfig
		virtual void InitAudioConfig();

		UFUNCTION(BlueprintImplementableEvent)
		void         OnAudioConfigChanged();
		virtual void OnAudioConfigChanged_Native();

		UPROPERTY(EditDefaultsOnly, Category="KMods|Config")
		FKPCLModConfigHelper_Float  mAudioConfig;
		TArray<FKPCLAudioComponent> mAudioComponents;
		// END: AudioConfig

		// START: Interaction
		virtual void StartIsLookedAtForDismantle_Implementation(AFGCharacterPlayer* byCharacter) override;
		virtual void StartIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;
		virtual void StartIsAimedAtForColor_Implementation(AFGCharacterPlayer* byCharacter, bool isValid) override;
		virtual void StartIsLookedAtForConnection(AFGCharacterPlayer* byCharacter, UFGCircuitConnectionComponent* overlappingConnection) override;

		void UpdateInstancesForOutline() const;
		// END: Interaction

		// START: Advanced Instance Overwrite

	protected:
		UPROPERTY(EditDefaultsOnly, Category="KMods|Mesh")
		TArray<FKPCLMeshOverwriteInformation> mDefaultMeshOverwriteInformations;

		UPROPERTY(Replicated, SaveGame)
		TArray<FKPCLMeshOverwriteInformation> mMeshOverwriteInformations;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Mesh")
		TArray<int32> mCustomIndicatorHandleIndexes;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Indicator", meta=(ArraySizeEnum="ENewProductionState"))
		FLinearColor mStateColors[11];

		UPROPERTY(EditDefaultsOnly, Category="KMods|Indicator")
		int32 mDefaultIndex = 20;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Indicator")
		int32 mIntensity = 2.f;

		virtual void ReApplyColorForIndex(int32 Idx, const FFactoryCustomizationData& customizationData);
		virtual void ApplyCustomizationData_Native(const FFactoryCustomizationData& customizationData) override;
		virtual void SetCustomizationData_Native(const FFactoryCustomizationData& customizationData) override;

		virtual void InitMeshOverwriteInformation();
		void         ApplyMeshOverwriteInformation(int32 Idx);
		void         ApplyMeshInformation(FKPCLMeshOverwriteInformation Information);
		virtual bool ShouldOverwriteIndexHandle(int32 Idx, FKPCLMeshOverwriteInformation& Information);

	public:
		UFUNCTION(BlueprintCallable, Category="KMods")
		bool AIO_OverwriteInstanceData(UStaticMesh* Mesh, int32 Idx);

		UFUNCTION(BlueprintCallable, Category="KMods")
		bool AIO_OverwriteInstanceData_Transform(UStaticMesh* Mesh, FTransform NewRelativTransform, int32 Idx);

		UFUNCTION(BlueprintCallable, Category="KMods")
		bool AIO_UpdateCustomFloat(int32 FloatIndex, float Data, int32 InstanceIdx, bool MarkDirty = true);

		UFUNCTION(BlueprintCallable, Category="KMods")
		bool AIO_UpdateCustomFloatAsColor(int32 StartFloatIndex, FLinearColor Data, int32 InstanceIdx, bool MarkDirty = true);

		UFUNCTION(BlueprintCallable, Category="KMods")
		bool AIO_SetInstanceHidden(int32 InstanceIdx, bool IsHidden);

		UFUNCTION(BlueprintCallable, Category="KMods")
		bool AIO_SetInstanceWorldTransform(int32 InstanceIdx, FTransform Transform);

	protected:
		TMap<int32 , TMap<int32 , float>> mCachedCustomData;
		TMap<int32 , FTransform>          mCachedTransforms;
		// END: Advanced Instance Overwrite

		// START: AFGBuildableFactory
		virtual void Factory_Tick(float dt) override;

		virtual void Factory_TickAuthOnly(float dt) {
		};

		virtual void Factory_TickClientOnly(float dt) {
		};
		bool bCustomFactoryTickPreCustomLogic = false;

		virtual bool  CanProduce_Implementation() const override;
		virtual float GetProductionCycleTime() const override;
		virtual float GetDefaultProductionCycleTime() const override;
		virtual float GetProductionProgress() const override;

		virtual void  SetPendingPotential(float newPendingPotential) override;
		virtual float CalcProductionCycleTimeForPotential(float potential) const override;
		virtual float GetProducingPowerConsumptionBase() const override;

		virtual void Factory_CollectInput_Implementation() override;
		virtual void Factory_PullPipeInput_Implementation(float dt) override;
		virtual void Factory_PushPipeOutput_Implementation(float dt) override;

		virtual void OnReplicationDetailActorCreated() override;
		virtual void OnReplicationDetailActorRemoved() override;
		virtual void OnBuildableReplicationDetailStateChange(bool newStateIsActive) override;
		// END: AFGBuildableFactory

	protected:
		virtual void CollectBelts();
		virtual void CollectAndPushPipes(float dt, bool IsPush);

	public:
		/** ----- Blueprint Functions (setter and functions) ----- */
		/** This function reset the Production time to 0 */
		UFUNCTION(BlueprintCallable, Category = "KMods ")
		void ResetProduction();

		/** {Only works on Server Side} Set a new Production Time (Should it also reset the Production?) */
		UFUNCTION(BlueprintCallable, Category = "KMods ")
		void SetProductionTime(float NewTime, bool ShouldResetProduction = false);

		/** {Only works on Server Side} Set a new Power Option */
		UFUNCTION(BlueprintCallable, Category = "KMods ")
		void SetPowerOption(FPowerOptions NewPowerOption);

		/** Get Power Option (Only a copy!) */
		UFUNCTION(BlueprintCallable, Category = "KMods ")
		FPowerOptions  GetPowerOption() const;
		FPowerOptions& GetPowerOptionRef();

		UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods ")
		UKPCLDefaultRCO* GetDefaultKModRCO() const;

		template<class T>
		FORCEINLINE T* GetRCO() { return Cast<T>(GetDefaultKModRCO()); }

		template<class T>
		FORCEINLINE bool GetRCOChecked(T*& OutRCO) {
			OutRCO = Cast<T>(GetDefaultKModRCO());
			return IsValid(OutRCO);
		}

		FORCEINLINE virtual TSubclassOf<UFGRemoteCallObject> GetRCOClass() const {
			return UKPCLDefaultRCO::StaticClass();
		}

		/** ----- Blueprint Functions (setter and functions) END ----- */


		/** ----- Events ----- */
		/** This Event call if a Production done NATIVE use this to handle producing */
		UFUNCTION(BlueprintNativeEvent, Category = "KMods  Events")
		void onProducingFinal();

		virtual void onProducingFinal_Implementation() {
		};

		UFUNCTION(BlueprintNativeEvent, Category = "KMods  Events")
		void                     RevalidateInventoryStateForReplication();
		FORCEINLINE virtual void RevalidateInventoryStateForReplication_Implementation() {
			if(HasAuthority()) {
				SetBelts();
				ReconfigureInventory();
			}
		}

		/** ----- Events END ----- */

		/** ----- Blueprint Functions (Getter) ----- */
		/** Get Production time (based also on Power Shards!) */
		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods  Events")
		virtual float GetProductionTime() const;

		/** Get Production time (PENDING) (based also on Power Shards!) */
		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods  Events")
		virtual float GetPendingProductionTime() const;

		/** Get Production Handler (Task) */
		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
		FFullProductionHandle GetProductionHandle() const;

		/** Work on Both sides */
		UFUNCTION(BlueprintCallable, Category="KMods|Cleaner")
		virtual void             FlushFluids();
		FORCEINLINE virtual void Server_DoFlush() {
		};
		/** ----- Blueprint Functions (Getter) END ----- */


		/** ----- Cpp Functions ----- */
		UFUNCTION()
		virtual void EndProductionTime();

		virtual void BeltPipeGrab(float dt);
		virtual void HandlePower(float dt);
		virtual void HandlePowerInit();
		virtual void HandleUiTick(float dt);
		virtual void InitComponents();

		// UI: Get max Power Consume
		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
		virtual float GetMaxPowerConsume() const;

		// UI: Get current Power Consume
		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
		virtual float GetPowerConsume() const;

		// UI: Get current Power Consume
		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
		virtual bool GetHasPower() const;

		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
		virtual UFGPowerConnectionComponent* GetPowerConnection() const;

		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
		float GetCurrentPowerMultiplier() const;

		UFUNCTION(BlueprintCallable, Category = "KMods|Production")
		void SetPowerMultiplier(float NewMultiplier);
		/** ----- Cpp Functions END ----- */


		/** ----- Inventory Stuff ----- */
		virtual UClass* GetReplicationDetailActorClass() const override;

		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		UFGInventoryComponent* GetInventory() const;

		virtual void             ReconfigureInventory();
		FORCEINLINE virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const { return true; }
		FORCEINLINE virtual bool FormFilterInputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const { return true; }

		UFUNCTION()
		virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) {
		}

		UFUNCTION()
		virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved) {
		}

		friend class AKPCLReplicationActor_ExtractorBase;

		UPROPERTY(EditDefaultsOnly, Category = "KMods|Inventory")
		TArray<FKPCLInventoryStructure> mInventoryDatas;

		UPROPERTY(SaveGame, Replicated)
		TArray<FKPCLInventoryStructure> mInventoryDatasSaved;

		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		UFGInventoryComponent* GetInventoryFromIndex(int32 Idx) const;

		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		bool GetInventoryData(int32 Idx, FKPCLInventoryStructure& Out) const;

		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		bool GetStackFromInventory(int32 Idx, int32 InventoryIdx, FInventoryStack& Stack) const;

		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		bool DoInventoryDataExsists(int32 Idx) const;

		UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
		bool AreAllInventorysValid() const;

		UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
		void ResizeInventory(int32 Idx, int32 Size);

		virtual void OnRep_ReplicationDetailActor() override;

		virtual void InitInventorys();

		virtual void PreInitInventoryIdex(int32 Idx) {
		};

		virtual void PostInitInventoryIdex(int32 Idx) {
		};

		virtual void OnReplicatedInventoryIndex(int32 Idx) {
		};

		virtual void OnFlushInventoryIndex(int32 Idx) {
		};

		virtual void OnRemoveReplicatedInventoryIndex(int32 Idx) {
		};

		virtual bool GetInventorysAreValid() const;

		UFUNCTION(BlueprintCallable, Category = "KMods|Production")
		// set again the inventory on all Belts after and while init the replication actor
		virtual void SetBelts();
		/** ----- Inventory Stuff END ----- */


		/** ----- Production State Start ----- */
		/** Call on Server; is used for Apply ProductionState to replication and set the State!!! */
		virtual void HandleIndicator();

		/** Can call from all will only set on owning Player */
		UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
		void ApplyNewProductionState(ENewProductionState NewState);

		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		FORCEINLINE ENewProductionState GetCurrentProductionState() const { return mCurrentState; }

		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		FORCEINLINE ENewProductionState GetLastProductionState() const { return mLastState; }

		UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
		FORCEINLINE ENewProductionState GetCurrentCompProductionState() const {
			if(mCustomProductionStateIndicator) {
				return mCustomProductionStateIndicator->GetState();
			}
			return GetLastProductionState();
		}

		UPROPERTY(BlueprintReadOnly, Replicated, Category="KMods|Indicator")
		TEnumAsByte<ENewProductionState> mCurrentState = ENewProductionState::Max;

		UPROPERTY(BlueprintReadOnly, Category="KMods|Indicator")
		TEnumAsByte<ENewProductionState> mLastState = ENewProductionState::Max;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Indicator")
		TArray<TEnumAsByte<ENewProductionState>> mPulsStates = {ENewProductionState::Error, ENewProductionState::Idle, ENewProductionState::Paused};

		bool bOneStateWasNotApplied = false;
		/** ----- Production State End ----- */

		/** ----- Variables ----- */
		UPROPERTY(EditDefaultsOnly, SaveGame, Replicated, Category="KMods")
		FFullProductionHandle mProductionHandle;

		UPROPERTY(Replicated, EditDefaultsOnly, Category="KMods")
		FPowerOptions mPowerOptions;


		// Component Caching
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class UFGPowerConnectionComponent* mFGPowerConnection;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class UKPCLBetterIndicator* mCustomProductionStateIndicator;

		TMap<EKPCLConnectionType , TArray<UFGConnectionComponent*>> mConnectionMap;

		// Native Helper
		UFGFactoryConnectionComponent*         GetConv(int Index, ECDirection Direction = Input) const;
		TArray<UFGFactoryConnectionComponent*> GetAllConv(ECDirection Direction = Any) const;
		UFGPipeConnectionFactory*              GetPipe(int Index, ECDirection Direction = Input) const;
		TArray<UFGPipeConnectionFactory*>      GetAllPipes(ECDirection Direction = Any) const;
		bool                                   bInventoryHasInit = false;
};
