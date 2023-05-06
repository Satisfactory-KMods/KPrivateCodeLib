#pragma once

#include "Buildables/FGBuildable.h"
#include "Replication/FGReplicationDetailActor_BuildableFactory.h"
#include "Replication/FGRepDetailActor_Extractor.h"

#include "KPCLReplicationActor_ProducerBase.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLReplicationActor_ProducerBase : public AFGReplicationDetailActor_BuildableFactory
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;
	virtual void InitReplicationDetailActor( AFGBuildable* owningActor ) override;
	virtual void FlushReplicationActorStateToOwner( ) override;
	virtual void RemoveDetailActorFromOwner( ) override;
	virtual bool HasCompletedInitialReplication( ) const override;
	virtual void OnDismantleRefund( ) override;

	FORCEINLINE void SetOwningBuildable( AFGBuildable* Buildable )
	{
		mOwningBuildable = Buildable;
	};
	
	friend class AKPCLProducerBase;

private:
	UPROPERTY( Replicated )
	TArray< UFGInventoryComponent* > mInventorys;
};
