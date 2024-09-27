// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/KPCLUnlockSubsystem.h"

#include "KPrivateCodeLibModule.h"
#include "UnrealNetwork.h"
#include "BFL/KBFL_Util.h"
#include "Components/KPCLNetworkPlayerComponent.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(KPCLUnlockSubsystemLog, Log, All)

DEFINE_LOG_CATEGORY(KPCLUnlockSubsystemLog)

AKPCLUnlockSubsystem::AKPCLUnlockSubsystem() {
	mShouldSave = true;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;

	mFluidItemsPerBytes = AKPCLNetworkCore::mFluidItemsPerBytes;
	mSolidItemsPerBytes = AKPCLNetworkCore::mSolidItemsPerBytes;

	mNetworkConnectionFluidBufferSize = 500;
	mNetworkConnectionSolidBufferSize = 1;
}

AKPCLUnlockSubsystem* AKPCLUnlockSubsystem::Get(UObject* worldContext) {
	return Cast<AKPCLUnlockSubsystem>(UKBFL_Util::GetSubsystemFromChild(worldContext, StaticClass()));
}

void AKPCLUnlockSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLUnlockSubsystem, mUnlockedNetworkTiers);
	DOREPLIFETIME(AKPCLUnlockSubsystem, mBuildedNexus);
}

void AKPCLUnlockSubsystem::BeginPlay() {
	Super::BeginPlay();

	UKBFLAssetDataSubsystem* Subsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	if(HasAuthority()) {
		AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
		if(IsValid(SchematicManager)) {
			bPassiveIsUnlocked = SchematicManager->IsSchematicPurchased(mSchematicToUnlockPassivPoints);
			if(!bPassiveIsUnlocked) {
				SchematicManager->PurchasedSchematicDelegate.AddUniqueDynamic(this, &AKPCLUnlockSubsystem::OnSchematicUnlocked);
			}
		}
	}
}

void AKPCLUnlockSubsystem::OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic) {
	if(UnlockedSchematic == mSchematicToUnlockPassivPoints) {
		bPassiveIsUnlocked = true;
	}
}

void AKPCLUnlockSubsystem::Init() {
	Super::Init();

	AKPCLNetworkCore::mFluidItemsPerBytes = mFluidItemsPerBytes;
	AKPCLNetworkCore::mSolidItemsPerBytes = mSolidItemsPerBytes;

	FNetworkConnectionInformations::mNetworkConnectionFluidBufferSize = mNetworkConnectionFluidBufferSize;
	FNetworkConnectionInformations::mNetworkConnectionSolidBufferSize = mNetworkConnectionSolidBufferSize;

	SetActorTickInterval(1 / 15);
}

void AKPCLUnlockSubsystem::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	for(AFGPlayerState* Player: mPlayerStates) {
		if(IsValid(Player)) {
			UKPCLNetworkPlayerComponent* Comp = UKPCLNetworkPlayerComponent::GetOrCreateNetworkComponentToPlayerState(GetWorld(), Player, mStateComponentClass);
			if(IsValid(Comp)) {
				Comp->CustomTick(DeltaSeconds);
			}
		}
	}
}

int32 AKPCLUnlockSubsystem::GetNetworkTier() const {
	int32 Tier = 1;
	if(mUnlockedNetworkTiers.Num() > 0) {
		for(UClass* UnlockedNetworkTier: mUnlockedNetworkTiers) {
			if(UnlockedNetworkTier) {
				Tier++;
			}
		}
	}
	return Tier;
}

void AKPCLUnlockSubsystem::UnlockNetworkTier(TSubclassOf<UFGSchematic> BoundedSchematic) {
	if(HasAuthority()) {
		if(mUnlockedNetworkTiers.AddUnique(BoundedSchematic) != INDEX_NONE) {
			OnNetworkTierUnlocked.Broadcast(GetNetworkTier());
		}
	}
}

void AKPCLUnlockSubsystem::OnNexusConstruct(AKPCLNetworkCore* Nexus) {
	mBuildedNexus.Add(Nexus);
}

void AKPCLUnlockSubsystem::OnNexusDeconstruct(AKPCLNetworkCore* Nexus) {
	if(mBuildedNexus.Contains(Nexus)) {
		mBuildedNexus.Remove(Nexus);
	}
}

int32 AKPCLUnlockSubsystem::GetGlobalNexusCount() const {
	return mBuildedNexus.Num();
}

int32 AKPCLUnlockSubsystem::GetMaxGlobalNexusCount() const {
	return mNetworkGlobalNexusMaxCount;
}

TArray<AKPCLNetworkCore*> AKPCLUnlockSubsystem::GetAllNexusInTheWorld() const {
	return mBuildedNexus;
}

void AKPCLUnlockSubsystem::RegisterPlayerState(AFGPlayerState* State) {
	if(IsValid(State)) {
		mPlayerStates.AddUnique(State);
		UE_LOG(LogKPCL, Warning, TEXT("Register PlayerState by BeginPlay! %s"), *State->GetName())
		UKPCLNetworkPlayerComponent::GetOrCreateNetworkComponentToPlayerState(GetWorld(), State, mStateComponentClass);
	}
}