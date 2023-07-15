#include "Replication/KPCLReplicationActor_ExtractorBase.h"

#include "FGInventoryLibrary.h"
#include "KPrivateCodeLibModule.h"
#include "Logging.h"
#include "Buildable/KPCLProducerBase.h"
#include "Buildable/Modular/KPCLModularExtractorBase.h"

void AKPCLReplicationActor_ExtractorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKPCLReplicationActor_ExtractorBase, mInventorys);
}

void AKPCLReplicationActor_ExtractorBase::InitReplicationDetailActor(AFGBuildable* owningActor) {
	Super::InitReplicationDetailActor(owningActor);

	AKPCLExtractorBase* Building = Cast<AKPCLExtractorBase>(mOwningBuildable);
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

void AKPCLReplicationActor_ExtractorBase::FlushReplicationActorStateToOwner() {
	AKPCLExtractorBase* Building = Cast<AKPCLExtractorBase>(mOwningBuildable);
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

void AKPCLReplicationActor_ExtractorBase::RemoveDetailActorFromOwner() {
	AKPCLExtractorBase* Building = Cast<AKPCLExtractorBase>(mOwningBuildable);
	if(IsValid(Building)) {
		UE_LOG(LogKPCL, Warning, TEXT("RemoveDetailActorFromOwner: %s | Owner: %s"), *GetName(), *GetOwningBuildable()->GetName())
		for(int32 Idx = 0; Idx < Building->mInventoryDatasSaved.Num(); ++Idx) {
			Building->mInventoryDatasSaved[Idx].ClearReplication();
			Building->OnRemoveReplicatedInventoryIndex(Idx);
		}

		Building->RevalidateInventoryStateForReplication();
		UE_LOG(LogKPCL, Warning, TEXT("RemoveDetailActorFromOwner Building->onFlushToOwner(): %s | Owner: %s"), *GetName(), *GetOwningBuildable()->GetName())
	} else {
		UE_LOG(LogKPCL, Warning, TEXT("RemoveDetailActorFromOwner: Invalid Owner! : %s"), *GetName())
	}

	Super::RemoveDetailActorFromOwner();
}

bool AKPCLReplicationActor_ExtractorBase::HasCompletedInitialReplication() const {
	AKPCLExtractorBase* Building = Cast<AKPCLExtractorBase>(mOwningBuildable);
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

void AKPCLReplicationActor_ExtractorBase::OnDismantleRefund() {
	Super::OnDismantleRefund();

	AKPCLExtractorBase* Building = Cast<AKPCLExtractorBase>(mOwningBuildable);
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
