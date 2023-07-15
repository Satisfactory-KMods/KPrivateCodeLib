// ILikeBanas

#include "Buildable/Modular/KPCLModularBuildingBase.h"

#include "AkAcousticTextureSetComponent.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPlayerController.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
#include "KPrivateCodeLibModule.h"
#include "Logging.h"

AKPCLModularBuildingBase::AKPCLModularBuildingBase() {
}

FText AKPCLModularBuildingBase::GetLookAtDecription_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) const {
	return mShouldUseUiFromMaster && GetMasterBuildable() != this && IsValid(GetMasterBuildable()) ? Execute_GetLookAtDecription(GetMasterBuildable(), byCharacter, state) : Super::GetLookAtDecription_Implementation(byCharacter, state);
}

bool AKPCLModularBuildingBase::IsUseable_Implementation() const {
	return mShouldUseUiFromMaster && GetMasterBuildable() != this && IsValid(GetMasterBuildable()) ? Execute_IsUseable(GetMasterBuildable()) : Super::IsUseable_Implementation();
}

void AKPCLModularBuildingBase::OnUseStop_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& State) {
	if(GetMasterBuildable() != this && mShouldUseUiFromMaster && IsValid(GetMasterBuildable())) {
		Execute_OnUseStop(mMasterBuilding.Get(), byCharacter, State);
		return;
	}
	Super::OnUseStop_Implementation(byCharacter, State);
}

void AKPCLModularBuildingBase::OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& State) {
	if(GetMasterBuildable() != this && mShouldUseUiFromMaster && IsValid(GetMasterBuildable())) {
		Execute_OnUse(mMasterBuilding.Get(), byCharacter, State);
		return;
	}
	Super::OnUse_Implementation(byCharacter, State);
}

TSubclassOf<UKPCLModularAttachmentDescriptor> AKPCLModularBuildingBase::GetModularAttachmentClass_Implementation() {
	return mUpgradeClass;
}

int32 AKPCLModularBuildingBase::GetModularIndex_Implementation() {
	return mModularIndex;
}

void AKPCLModularBuildingBase::ApplyModularIndex_Implementation(int32 Index) {
	mModularIndex = Index;
}

void AKPCLModularBuildingBase::SetAttachedActor_Implementation(AFGBuildable* Actor) {
	if(Actor) {
		mMasterBuilding = Actor;
		OnMasterBuildingReceived(Actor);
	}
}

void AKPCLModularBuildingBase::RemoveAttachedActor_Implementation(AFGBuildable* Actor) {
	if(mModularHandler) {
		mModularHandler->AttachedActorRemoved(Actor);
		OnModulesWasUpdated();
		if(OnModulesUpdated.IsBound()) {
			OnModulesUpdated.Broadcast();
		}
	} else {
		if(mMasterBuilding.IsValid()) {
			Execute_RemoveAttachedActor(mMasterBuilding.Get(), Actor);
		}
	}
}

bool AKPCLModularBuildingBase::AttachedActor_Implementation(AFGBuildable* Actor, TSubclassOf<UKPCLModularAttachmentDescriptor> Attachment, FTransform Location, float Distance) {
	if(mModularHandler) {
		if(mModularHandler->AddNewActorToAttachment(Actor, Attachment, Location, Distance)) {
			OnModulesWasUpdated();
			if(OnModulesUpdated.IsBound()) {
				OnModulesUpdated.Broadcast();
			}
			return true;
		}
	}
	return false;
}

void AKPCLModularBuildingBase::OnModulesUpdated_Implementation() {
	OnMeshUpdate();
	Event_OnMeshUpdate();
}

void AKPCLModularBuildingBase::ReadyForVisuelUpdate() {
	Super::ReadyForVisuelUpdate();
	OnMeshUpdate();
	Event_OnMeshUpdate();
}

void AKPCLModularBuildingBase::GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const {
	Super::GetChildDismantleActors_Implementation(out_ChildDismantleActors);

	UKPCLModularBuildingHandlerBase* Base = GetHandler<UKPCLModularBuildingHandlerBase>();
	if(IsValid(Base) && mDismantleAllChilds) {
		TArray<AFGBuildable*> Buildables;
		Base->GetAttachedActors(Buildables);
		for(AFGBuildable* Buildable: Buildables) {
			if(IsValid(Buildable)) {
				out_ChildDismantleActors.AddUnique(Buildable);
				Execute_GetChildDismantleActors(Buildable, out_ChildDismantleActors);
			}
		}
	}
}

void AKPCLModularBuildingBase::BeginPlay() {
	Super::BeginPlay();
	if(HasAuthority()) {
		HandlePowerInit();
	}

	GetHandler()->OnHandlerTriggerUpdate.AddUniqueDynamic(this, &AKPCLModularBuildingBase::OnMeshUpdate);
}

void AKPCLModularBuildingBase::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	if(!mModularHandler) {
		if(mMasterBuilding.IsValid()) {
			Execute_RemoveAttachedActor(mMasterBuilding.Get(), this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AKPCLModularBuildingBase::OnModulesWasUpdated_Implementation() {
}

void AKPCLModularBuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//UI
	DOREPLIFETIME(AKPCLModularBuildingBase, mMasterBuilding);
	DOREPLIFETIME(AKPCLModularBuildingBase, mModularHandler);
}

void AKPCLModularBuildingBase::PostInitializeComponents() {
	Super::PostInitializeComponents();
	mModularHandler = FindComponentByClass<UKPCLModularBuildingHandlerBase>();
}
