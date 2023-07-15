#include "Replication/KPCLReplicationActor_ProducerBase.h"

#include "FGInventoryLibrary.h"
#include "KPrivateCodeLibModule.h"
#include "Buildable/KPCLProducerBase.h"

void AKPCLReplicationActor_ProducerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLReplicationActor_ProducerBase, mInventorys);
}

void AKPCLReplicationActor_ProducerBase::InitReplicationDetailActor(AFGBuildable* owningActor) {
	Super::InitReplicationDetailActor(owningActor);

	AKPCLProducerBase* Building = Cast<AKPCLProducerBase>(mOwningBuildable);
	if(IsValid(Building)) {
		UE_LOG(LogKPCL, Warning, TEXT("InitReplicationDetailActor: %s | Owner: %s"), *GetName(), *GetOwningBuildable()->GetName())
		for(int32 Idx = 0; Idx < Building->mInventoryDatasSaved.Num(); ++Idx) {
			if(!IsValid(Building->mInventoryDatasSaved[Idx].GetInventory_Detailed())) {
				FString ComponentName = Building->mInventoryDatasSaved[Idx].mComponentName.ToString();

				UE_LOG(LogKPCL, Warning, TEXT("InitReplicationDetailActor inventory: %s | Owner: %s"), *FName( "ReplicationInventory_" + ComponentName ).ToString(), *this->GetName())

				UFGInventoryComponent* Component = UFGInventoryLibrary::CreateInventoryComponent(this, FName("ReplicationInventory_" + ComponentName));
				if(IsValid(Component)) {
					Component->SetIsReplicated(true);
					mInventorys.Add(Component);
					Building->mInventoryDatasSaved[Idx].SetReplicated(Component);
					Building->OnReplicatedInventoryIndex(Idx);
				} else {
					UE_LOG(LogKPCL, Warning, TEXT("InitReplicationDetailActor inventory WASINVALID!?!?!?!?!?: %s | Owner: %s"), *FName( "ReplicationInventory_" + ComponentName ).ToString(), *this->GetName())
				}
			}
			Building->RevalidateInventoryStateForReplication();
		}
	} else {
		UE_LOG(LogKPCL, Warning, TEXT("InitReplicationDetailActor: Invalid Owner! : %s"), *GetName())
	}
}

void AKPCLReplicationActor_ProducerBase::FlushReplicationActorStateToOwner() {
	AKPCLProducerBase* Building = Cast<AKPCLProducerBase>(mOwningBuildable);
	if(IsValid(Building)) {
		UE_LOG(LogKPCL, Warning, TEXT("FlushReplicationActorStateToOwner: %s | Owner: %s"), *GetName(), *GetOwningBuildable()->GetName())
		for(int32 Idx = 0; Idx < Building->mInventoryDatasSaved.Num(); ++Idx) {
			Building->mInventoryDatasSaved[Idx].FlushToOwner();
			Building->OnFlushInventoryIndex(Idx);
		}
	} else {
		UE_LOG(LogKPCL, Warning, TEXT("FlushReplicationActorStateToOwner: Invalid Owner! : %s"), *GetName())
	}

	Super::FlushReplicationActorStateToOwner();
}

void AKPCLReplicationActor_ProducerBase::RemoveDetailActorFromOwner() {
	AKPCLProducerBase* Building = Cast<AKPCLProducerBase>(mOwningBuildable);
	if(IsValid(Building)) {
		UE_LOG(LogKPCL, Warning, TEXT("RemoveDetailActorFromOwner: %s | Owner: %s"), *GetName(), *GetOwningBuildable()->GetName())
		for(int32 Idx = 0; Idx < Building->mInventoryDatasSaved.Num(); ++Idx) {
			Building->mInventoryDatasSaved[Idx].ClearReplication();
			Building->OnRemoveReplicatedInventoryIndex(Idx);
		}

		Building->RevalidateInventoryStateForReplication();
	} else {
		UE_LOG(LogKPCL, Warning, TEXT("RemoveDetailActorFromOwner: Invalid Owner! : %s"), *GetName())
	}

	Super::RemoveDetailActorFromOwner();
}

bool AKPCLReplicationActor_ProducerBase::HasCompletedInitialReplication() const {
	AKPCLProducerBase* Building = Cast<AKPCLProducerBase>(mOwningBuildable);
	if(IsValid(Building)) {
		//UE_LOG( LogKPCL, Warning, TEXT("HasCompletedInitialReplication: %s | Owner: %s"), *GetName(), *GetOwningBuildable()->GetName() )
		for(int32 Idx = 0; Idx < Building->mInventoryDatasSaved.Num(); ++Idx) {
			if(IsValid(Building->mInventoryDatasSaved[Idx].GetInventory_Detailed())) {
				return false;
			}
		}

		return Super::HasCompletedInitialReplication();
	}
	UE_LOG(LogKPCL, Warning, TEXT("HasCompletedInitialReplication: Invalid Owner! : %s"), *GetName())

	return false;
}

void AKPCLReplicationActor_ProducerBase::OnDismantleRefund() {
	Super::OnDismantleRefund();

	AKPCLProducerBase* Building = Cast<AKPCLProducerBase>(mOwningBuildable);
	if(IsValid(Building)) {
		UE_LOG(LogKPCL, Warning, TEXT("OnDismantleRefund: %s | Owner: %s"), *GetName(), *GetOwningBuildable()->GetName())
		for(int32 Idx = 0; Idx < Building->mInventoryDatasSaved.Num(); ++Idx) {
			Building->mInventoryDatasSaved[Idx].FlushToOwner();
			Building->OnFlushInventoryIndex(Idx);
		}
	} else {
		UE_LOG(LogKPCL, Warning, TEXT("OnDismantleRefund: Invalid Owner! : %s"), *GetName())
	}
}
