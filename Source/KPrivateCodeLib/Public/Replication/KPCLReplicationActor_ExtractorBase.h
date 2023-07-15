#pragma once

#include "Replication/FGRepDetailActor_Extractor.h"
#include "Replication/FGReplicationDetailActor_BuildableFactory.h"

#include "KPCLReplicationActor_ExtractorBase.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLReplicationActor_ExtractorBase: public AFGRepDetailActor_Extractor {
	GENERATED_BODY()

	public:
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		virtual void InitReplicationDetailActor(AFGBuildable* owningActor) override;
		virtual void FlushReplicationActorStateToOwner() override;
		virtual void RemoveDetailActorFromOwner() override;
		virtual bool HasCompletedInitialReplication() const override;
		virtual void OnDismantleRefund() override;

		FORCEINLINE void SetOwningBuildable(AFGBuildable* Buildable) {
			mOwningBuildable = Buildable;
		};

		friend class AKPCLExtractorBase;

	private:
		UPROPERTY(Replicated)
		TArray<UFGInventoryComponent*> mInventorys;
};
