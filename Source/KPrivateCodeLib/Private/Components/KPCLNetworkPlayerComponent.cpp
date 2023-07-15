// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Components/KPCLNetworkPlayerComponent.h"

#include "FGSchematicManager.h"
#include "KPrivateCodeLibModule.h"
#include "Buildables/FGBuildable.h"
#include "Network/KPCLNetwork.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

void UKPCLNetworkPlayerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mNextBuilding);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mDistanceStrength);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mNextCore);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mUnlockedPermissions);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mIsAllowedToSetOverflow);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mInventoryRules);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mUnlockedPermissions);
}

void UKPCLNetworkPlayerComponent::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("UKPCLNetworkPlayerComponent::BeginPlay: %d"), mUnlockedPermissions.Num())
	if(GetOwner()->HasAuthority()) {
		DoDistanceCheck();
		ReValidSchematics();

		AFGSchematicManager* Manager = AFGSchematicManager::Get(GetWorld());
		if(IsValid(Manager)) {
			Manager->PurchasedSchematicDelegate.AddUniqueDynamic(this, &UKPCLNetworkPlayerComponent::OnSchematicUnlocked);
		}
	}
}

void UKPCLNetworkPlayerComponent::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	if(GetOwner()->HasAuthority()) {
		DoDistanceCheck();
		ReValidSchematics();
	}
}

bool UKPCLNetworkPlayerComponent::ShouldSave_Implementation() const {
	return true;
}

void UKPCLNetworkPlayerComponent::DoDistanceCheck() {
	if(!IsStateActive()) {
		return;
	}

	FVector               Loc = GetPlayerCharacterLocation();
	const TArray<AActor*> ActorsToIgnore{};
	TArray<AActor*>       OutActors{};

	if(UKismetSystemLibrary::SphereOverlapActors(this, Loc, mMaxDistance, mObjectTypes, AFGBuildable::StaticClass(), ActorsToIgnore, OutActors)) {
		float                            LowestDistance = mMaxDistance;
		AFGBuildable*                    NearstActor = nullptr;
		UKPCLNetworkConnectionComponent* NearstNetworkComponent = nullptr;

		for(AActor* Actor: OutActors) {
			if(IsValid(Actor)) {
				if(UKismetSystemLibrary::DoesImplementInterface(Actor, UKPCLNetworkDataInterface::StaticClass())) {
					UKPCLNetworkConnectionComponent* Comp = Actor->FindComponentByClass<UKPCLNetworkConnectionComponent>();
					float                            Dist = FVector::Distance(Loc, Comp->GetComponentLocation());
					if(Dist <= LowestDistance) {
						NearstActor = Cast<AFGBuildable>(Actor);
						NearstNetworkComponent = Comp;
						if(IsValid(NearstActor)) {
							LowestDistance = Dist;
						}
					}
				}
			}
		}

		if(IsValid(NearstActor)) {
			if(IsValid(NearstNetworkComponent)) {
				const UKPCLNetwork* Network = Cast<UKPCLNetwork>(NearstNetworkComponent->GetPowerCircuit());
				if(IsValid(Network)) {
					if(Network->NetworkHasCore()) {
						mNextCore = Network->GetCore();
					}
				}
			}

			mNextBuilding = NearstActor;
			mDistanceStrength = FMath::Clamp<int32>(FMath::CeilToInt(FVector::Distance(Loc, NearstActor->FindComponentByClass<UKPCLNetworkConnectionComponent>()->GetComponentLocation()) / mMaxDistance * 100), 0, 100);

			OnRep_DistanceUpdated();
		}
	} else {
		mNextBuilding = nullptr;
		mNextCore = nullptr;
		mDistanceStrength = 0;

		OnRep_DistanceUpdated();
	}
}

void UKPCLNetworkPlayerComponent::CustomTick(float dt) {
	if(GetOwner()->HasAuthority()) {
		if(mDistanceCheckTimer.Tick(dt)) {
			DoDistanceCheck();
		}

		// We Queue the Player logic into the Core to make sure we dont have a Thread conflict here!
		AKPCLNetworkCore* Core = GetNextBuilding_Native<AKPCLNetworkCore>();
		if(IsValid(Core)) {
			if(mTimerForPushAndPullLogic.Tick(dt)) {
				Core->mNetworkPlayerComponentsThisFrame.Enqueue(this);
			}
		}
	}
}

void UKPCLNetworkPlayerComponent::ReceiveTickFromNetwork(AKPCLNetworkCore* Core) {
	UFGInventoryComponent* PlayerInv;
	if(GetPlayerCharacterInventory(PlayerInv) && !IsPlayerDead()) {
		for(const FKPCLPlayerInventoryRules InventoryRule: mInventoryRules) {
			const int32 NumInPlayerInventory = PlayerInv->GetNumItems(InventoryRule.mItemAmount.ItemClass);

			// Logic for Core --> Player
			if(InventoryRule.mShouldPullFromNetwork) {
				if(NumInPlayerInventory >= InventoryRule.mItemAmount.Amount) {
					continue;
				}

				int32           InventoryIndex;
				FInventoryStack Stack;
				if(Core->GetStackFromNetwork(InventoryRule.mItemAmount.ItemClass, Stack, InventoryIndex)) {
					if(Stack.HasItems()) {
						Stack.NumItems = FMath::Min(Stack.NumItems, InventoryRule.mItemAmount.Amount - NumInPlayerInventory);
						if(Stack.HasItems()) {
							Core->GetInventory()->RemoveFromIndex(InventoryIndex, PlayerInv->AddStack(Stack, true));
						}
					}
				}
			}

			// Logic for Core <-- Player
			else if(Core->GetInventory()) {
				if(NumInPlayerInventory <= InventoryRule.mItemAmount.Amount) {
					continue;
				}

				const int32 Amount = (InventoryRule.mItemAmount.Amount - NumInPlayerInventory) * -1;

				if(Amount > 0) {
					FInventoryStack Stack;
					Stack.NumItems = Amount;
					Stack.Item.SetItemClass(InventoryRule.mItemAmount.ItemClass);

					float Solid;
					float Fluid;
					Core->GetFreeBytes(Fluid, Solid);


					TSubclassOf<UFGItemDescriptor> Item = InventoryRule.mItemAmount.ItemClass;
					if(FKPCLNetworkMaxData* Data = Core->mItemCountMap.FindByKey(FKPCLNetworkMaxData(Item, 0))) {
						if(Data->mItemClass) {
							if(FCoreInventoryData* CoreData = Core->mSlotMappingAll.FindByKey(Data->mItemClass)) {
								Core->GetInventory()->GetStackFromIndex(CoreData->mInventoryIndex, Stack);
								if(Stack.NumItems >= Data->mMaxItemCount) {
									continue;
								}
							}
						}
					}

					if(Solid > 0.0f || Fluid > 0.0f) {
						if(const FCoreInventoryData* CoreData = Core->mSlotMappingAll.FindByKey(Stack.Item.GetItemClass())) {
							const int32 MaxAmount = Core->GetMaxItemsByBytes(Stack.Item.GetItemClass(), Fluid, Solid);
							Stack.NumItems = FMath::Min(MaxAmount, Stack.NumItems);

							if(Stack.HasItems()) {
								PlayerInv->Remove(Stack.Item.GetItemClass(), Core->GetInventory()->AddStackToIndex(CoreData->mInventoryIndex, Stack));
							}
						}
					}
				}
			}
		}
	}
}

void UKPCLNetworkPlayerComponent::ReValidSchematics() {
	mUnlockableSchematics.Empty();

	mUnlockableSchematics.Add(mSchematicForOverflowSink);
	mUnlockableSchematics.Add(mSchematicsToUnlockAutoPull);
	mUnlockableSchematics.Add(mSchematicsToUnlockAutoPush);
	mUnlockableSchematics.Add(mSchematicsToUnlockDistanceAccess);

	const AFGSchematicManager* Manager = AFGSchematicManager::Get(GetWorld());
	if(IsValid(Manager)) {
		for(TSubclassOf<UFGSchematic> UnlockableSchematic: mUnlockableSchematics) {
			if(Manager->IsSchematicPurchased(UnlockableSchematic)) {
				mUnlockedPermissions.AddUnique(UnlockableSchematic);
			}
		}
	}
}

void UKPCLNetworkPlayerComponent::OnSchematicUnlocked(TSubclassOf<UFGSchematic> Schematic) {
	ReValidSchematics();
}

FVector UKPCLNetworkPlayerComponent::GetPlayerCharacterLocation() const {
	if(IsValid(GetPlayerPawn())) {
		return GetPlayerPawn()->GetActorLocation();
	}
	return FVector();
}

AFGPlayerState* UKPCLNetworkPlayerComponent::GetPlayerState() const {
	return Cast<AFGPlayerState>(GetOwner());
}

AFGCharacterPlayer* UKPCLNetworkPlayerComponent::GetPlayerCharacter() const {
	if(IsStateActive()) {
		return Cast<AFGCharacterPlayer>(GetPlayerState()->GetOwnedPawn());
	}

	return nullptr;
}

bool UKPCLNetworkPlayerComponent::IsPlayerDead() const {
	AFGCharacterPlayer* Player = GetPlayerCharacter();
	if(IsValid(Player)) {
		return !Player->IsAliveAndWell();
	}

	return true;
}

APawn* UKPCLNetworkPlayerComponent::GetPlayerPawn() const {
	if(IsStateActive()) {
		return GetPlayerState()->GetOwnedPawn();
	}

	return nullptr;
}

bool UKPCLNetworkPlayerComponent::IsStateActive() const {
	if(IsValid(GetPlayerState())) {
		return GetPlayerState()->IsInPlayerArray();
	}
	return false;
}

bool UKPCLNetworkPlayerComponent::GetPlayerCharacterInventory(UFGInventoryComponent*& OutInventory) {
	if(!IsStateActive()) {
		return false;
	}

	if(!IsValid(mCachedPlayerCharacterInventory)) {
		if(IsValid(GetPlayerCharacter())) {
			mCachedPlayerCharacterInventory = GetPlayerCharacter()->GetInventory();
		}
	}

	OutInventory = mCachedPlayerCharacterInventory;
	return IsValid(OutInventory);
}

bool UKPCLNetworkPlayerComponent::DistanceAccessUnlocked() const {
	return mUnlockedPermissions.Contains(mSchematicsToUnlockDistanceAccess);
}

bool UKPCLNetworkPlayerComponent::PullLogicUnlocked() const {
	return mUnlockedPermissions.Contains(mSchematicsToUnlockAutoPull);
}

bool UKPCLNetworkPlayerComponent::PushLogicUnlocked() const {
	return mUnlockedPermissions.Contains(mSchematicsToUnlockAutoPush);
}

UKPCLNetworkPlayerComponent* UKPCLNetworkPlayerComponent::GetOrCreateNetworkComponentToPlayerState(UObject* WorldContextObject, AFGPlayerState* State, TSubclassOf<UKPCLNetworkPlayerComponent> ComponentClass) {
	// Try to get Player State if it's invalid
	if(!IsValid(State) && IsValid(WorldContextObject)) {
		if(IsValid(WorldContextObject->GetWorld())) {
			if(IsValid(WorldContextObject->GetWorld()->GetFirstPlayerController())) {
				State = WorldContextObject->GetWorld()->GetFirstPlayerController()->GetPlayerState<AFGPlayerState>();
			}
		}
	}

	// If still invalid return nullptr
	if(!IsValid(State)) {
		UE_LOG(LogKPCL, Error, TEXT("GetOrCreateNetworkComponentToPlayerState: Invalid Player State!"))
		return nullptr;
	}

	UKPCLNetworkPlayerComponent* Component = State->FindComponentByClass<UKPCLNetworkPlayerComponent>();
	if(!IsValid(Component)) {
		if(!State->HasAuthority()) {
			return Component;
		}

		TSubclassOf<UKPCLNetworkPlayerComponent> Class = !IsValid(ComponentClass) ? StaticClass() : ComponentClass;
		if(IsValid(Class)) {
			Component = NewObject<UKPCLNetworkPlayerComponent>(State, Class, FName("NetworkComp"));
			Component->SetIsReplicatedByDefault(true);
			Component->SetIsReplicated(true);
			Component->RegisterComponentWithWorld(State->GetWorld());
			Component->SetIsReplicated(true);
			State->ForceNetUpdate();
			UE_LOG(LogKPCL, Warning, TEXT("GetOrCreateNetworkComponentToPlayerState: New Component Created!"))
		} else {
			UE_LOG(LogKPCL, Error, TEXT("GetOrCreateNetworkComponentToPlayerState: Invalid Class!"))
		}
	}

	return Component;
}

bool UKPCLNetworkPlayerComponent::CanAccessTo() const {
	return IsValid(GetNextBuilding()) && DistanceAccessUnlocked();
}

bool UKPCLNetworkPlayerComponent::SinkOverFlowUnlocked() const {
	return mUnlockedPermissions.Contains(mSchematicForOverflowSink);
}

int32 UKPCLNetworkPlayerComponent::GetDistanceStrength() const {
	if(!CanAccessTo()) {
		return 0;
	}

	return mDistanceStrength;
}

AFGBuildable* UKPCLNetworkPlayerComponent::GetNextBuilding() const {
	if(IsValid(mNextCore)) {
		return Cast<AFGBuildable>(mNextCore);
	}

	return mNextBuilding;
}

TArray<FKPCLPlayerInventoryRules> UKPCLNetworkPlayerComponent::GetRules(EKPCLPlayerRuleFilter Filter) const {
	if(Filter == All) {
		return mInventoryRules;
	}

	TArray<FKPCLPlayerInventoryRules> Rules;

	if(Filter == Push) {
		int32 Idx = 0;
		for(FKPCLPlayerInventoryRules InventoryRule: mInventoryRules) {
			if(!InventoryRule.mShouldPullFromNetwork) {
				InventoryRule.mUIOnly_Index = Idx;
				Rules.Add(InventoryRule);
			}
			Idx++;
		}
	}

	if(Filter == Pull) {
		int32 Idx = 0;
		for(FKPCLPlayerInventoryRules InventoryRule: mInventoryRules) {
			if(InventoryRule.mShouldPullFromNetwork) {
				InventoryRule.mUIOnly_Index = Idx;
				Rules.Add(InventoryRule);
			}
			Idx++;
		}
	}

	return Rules;
}

void UKPCLNetworkPlayerComponent::AddRule(FKPCLPlayerInventoryRules Rule) {
	if(GetOwner()->HasAuthority()) {
		if(Rule.IsValid()) {
			mInventoryRules.Add(Rule);
			OnRep_RulesUpdated();
		}
	} else if(UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld())) {
		RCO->Server_AddRuleFromNetworkComponent(this, Rule);
	}
}

void UKPCLNetworkPlayerComponent::RemoveRule(int32 RuleIndex) {
	if(GetOwner()->HasAuthority()) {
		if(mInventoryRules.IsValidIndex(RuleIndex)) {
			mInventoryRules.RemoveAt(RuleIndex);
			OnRep_RulesUpdated();
		}
	} else if(UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld())) {
		RCO->Server_RemoveRuleFromNetworkComponent(this, RuleIndex);
	}
}

void UKPCLNetworkPlayerComponent::EditRule(int32 RuleIndex, FKPCLPlayerInventoryRules Rule) {
	if(GetOwner()->HasAuthority()) {
		if(mInventoryRules.IsValidIndex(RuleIndex) && Rule.IsValid()) {
			mInventoryRules[RuleIndex] = Rule;
			OnRep_RulesUpdated();
		}
	} else if(UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld())) {
		RCO->Server_EditRuleFromNetworkComponent(this, RuleIndex, Rule);
	}
}

void UKPCLNetworkPlayerComponent::OnRep_DistanceUpdated() {
	if(mOnDistanceUpdated.IsBound()) {
		mOnDistanceUpdated.Broadcast(mDistanceStrength);
	}
}

void UKPCLNetworkPlayerComponent::OnRep_RulesUpdated() {
	if(mOnRulesUpdated.IsBound()) {
		mOnRulesUpdated.Broadcast();
	}
}
