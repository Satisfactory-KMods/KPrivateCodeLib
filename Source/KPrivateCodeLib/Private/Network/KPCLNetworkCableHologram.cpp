// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkCableHologram.h"

#include "KPrivateCodeLibModule.h"
#include "Buildables/FGBuildableConveyorAttachment.h"
#include "Buildables/FGBuildableConveyorBase.h"
#include "Buildables/FGBuildablePipeBase.h"
#include "Buildables/FGBuildablePipelineAttachment.h"
#include "Buildables/FGBuildablePipelinePump.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Buildables/FGPipeHyperStart.h"
#include "Hologram/FGPowerPoleHologram.h"
#include "Net/UnrealNetwork.h"
#include "Network/KPCLNetwork.h"
#include "Network/KPCLNetworkCable.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Network/Holograms/KPCLNetworkBuildingAttachmentHologram.h"

AKPCLNetworkCableHologram::AKPCLNetworkCableHologram() {
	PrimaryActorTick.bCanEverTick = false;
}

void AKPCLNetworkCableHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(AKPCLNetworkCableHologram, mAttachmentHologram);
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

AActor* AKPCLNetworkCableHologram::Construct(TArray<AActor*>& out_children, FNetConstructionID netConstructionID)
{
	AActor* SUPER = Super::Construct(out_children, netConstructionID);
	AKPCLNetworkCable* Cable = CastChecked<AKPCLNetworkCable>(SUPER);
	if(IsConnectedToChild())
	{
		AActor* Constructed = mAttachmentHologram->Construct(out_children, netConstructionID);
		AKPCLNetworkBuildingAttachment* Attachment = CastChecked<AKPCLNetworkBuildingAttachment>(Constructed);
		Attachment->AttachTo(mAttachmentHologram->mTarget);
		if(GetConnection(0) != Attachment->mNetworkConnectionComponent)
		{
			Cable->Connect(GetConnection(0), Attachment->mNetworkConnectionComponent);
		} else
		{
			Cable->Connect(GetConnection(1), Attachment->mNetworkConnectionComponent);
		}
		out_children.Add(Constructed);
	}
	return SUPER;
}

void AKPCLNetworkCableHologram::SpawnChildren(AActor* hologramOwner, FVector spawnLocation, APawn* hologramInstigator)
{
	Super::SpawnChildren(hologramOwner, spawnLocation, hologramInstigator);
	mAttachmentHologram = Cast<AKPCLNetworkBuildingAttachmentHologram>(SpawnChildHologramFromRecipe(this, mAttachmentRecipe, hologramOwner, spawnLocation, hologramInstigator));
	fgcheck(mAttachmentHologram);
	mAttachmentHologram->SetDisabled(true);
}

void AKPCLNetworkCableHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	Super::SetHologramLocationAndRotation(hitResult);

	if(IsConnectedToChild()) return;
	AFGBuildable* Buildable = Cast<AFGBuildable>(hitResult.GetActor());
	if(IsValid(Buildable))
	{
		UKPCLNetworkConnectionComponent* Connection = Buildable->GetComponentByClass<UKPCLNetworkConnectionComponent>();
		if(IsValid(Connection) || IsBuildingDisallowed(Buildable))
		{
			if(!mAttachmentHologram->IsDisabled())
			{
				ConnectToTarget(nullptr);
			}
			return Super::SetHologramLocationAndRotation(hitResult);
		}
		ConnectToTarget(Buildable);
	} else
	{
		if(!mAttachmentHologram->IsDisabled())
		{
			ConnectToTarget(nullptr);
		}
		return Super::SetHologramLocationAndRotation(hitResult);
	}
}

void AKPCLNetworkCableHologram::ConnectToTarget(AFGBuildable* target)
{
	if(!IsValid(target))
	{
		if(GetConnectionComponent() == GetConnection(GetConnectionToSet()))
		{
			SetConnection(GetConnectionToSet(), nullptr);
			SetActiveAutomaticPoleHologram(nullptr);
		}
		mAttachmentHologram->SetTarget(nullptr);
		mAttachmentHologram->SetDisabled(true);
		return;
	}

	mAttachmentHologram->SetDisabled(false);
	mAttachmentHologram->SetTarget(target);
	SetConnection(GetConnectionToSet(), mAttachmentHologram->mConnectionComponent);
	SetActorLocationAndRotation(mAttachmentHologram->mConnectionComponent->GetComponentLocation(), target->GetActorRotation());
	SetActiveAutomaticPoleHologram(mAttachmentHologram);
}

void AKPCLNetworkCableHologram::BeginPlay() {
	Super::BeginPlay();

	mConnectionMesh = Cast<UStaticMeshComponent>(GetComponentsByTag(UStaticMeshComponent::StaticClass(), FName("ConInd"))[0]);
}

void AKPCLNetworkCableHologram::CheckValidPlacement() {
	Super::CheckValidPlacement();

	if(HasAuthority()) {
		const UKPCLNetworkConnectionComponent* NetworkConnection1 = Cast<UKPCLNetworkConnectionComponent>(mConnections[0]);
		const UKPCLNetworkConnectionComponent* NetworkConnection2 = Cast<UKPCLNetworkConnectionComponent>(mConnections[1]);
		if(IsValid(NetworkConnection1) && IsValid(NetworkConnection2)) {
			const UKPCLNetwork* Network1 = Cast<UKPCLNetwork>(NetworkConnection1->GetPowerCircuit());
			const UKPCLNetwork* Network2 = Cast<UKPCLNetwork>(NetworkConnection2->GetPowerCircuit());
			if(IsValid(Network1) && IsValid(Network2)) {
				if(Network1->GetCircuitID() != Network2->GetCircuitID()) {
					if(Network1->NetworkHasCore() && Network2->NetworkHasCore() && Network1->GetCore() != Network2->GetCore()) {
						AddConstructDisqualifier(UKPCLCDUniqueCore::StaticClass());
					}
				}
			} else if(IsValid(Network1)) {
				if(Network1->NetworkHasCore() && Cast<AKPCLNetworkCore>(NetworkConnection2->GetOwner())) {
					AddConstructDisqualifier(UKPCLCDUniqueCore::StaticClass());
				}
			} else if(IsValid(Network2)) {
				if(Network2->NetworkHasCore() && Cast<AKPCLNetworkCore>(NetworkConnection1->GetOwner())) {
					AddConstructDisqualifier(UKPCLCDUniqueCore::StaticClass());
				}
			}
		}
	}
}

bool AKPCLNetworkCableHologram::TryUpgrade(const FHitResult& hitResult) {
	bool Super = Super::TryUpgrade(hitResult);

	if(hitResult.IsValidBlockingHit() && Super) {
		AKPCLNetworkCable* OtherCable = Cast<AKPCLNetworkCable>(hitResult.GetActor());
		if(IsValid(OtherCable)) {
			SetConnection(0, OtherCable->GetConnection(0));
			SetConnection(1, OtherCable->GetConnection(1));
		}
	}

	return Super;
}

int32 AKPCLNetworkCableHologram::GetConnectionToSet() const
{
	return mCurrentConnection;
	//return IsValid(GetConnection(0)) ? 0 : 1;
}

bool AKPCLNetworkCableHologram::IsConnectedToChild() const
{
	if(GetConnectionToSet() == 0) return false;
	return GetConnectionComponent() == GetConnection(0);
}

bool AKPCLNetworkCableHologram::IsBuildingDisallowed(AFGBuildable* Buildable) const
{
	if(!IsValid(Buildable)) return true;
	if(
		Buildable->IsA(AKPCLNetworkBuildingBase::StaticClass()) ||
		Buildable->IsA(AFGBuildableConveyorBase::StaticClass()) ||
		Buildable->IsA(AFGBuildablePipeBase::StaticClass()) ||
		Buildable->IsA(AFGBuildableConveyorAttachment::StaticClass()) ||
		Buildable->IsA(AFGBuildablePowerPole::StaticClass()) ||
		Buildable->IsA(AFGBuildablePipeHyperAttachment::StaticClass()) ||
		Buildable->IsA(AFGBuildablePipelineAttachment::StaticClass()) ||
		Buildable->IsA(AFGPipeHyperStart::StaticClass()) ||
		Buildable->IsA(AFGBuildablePipeHyperJunction::StaticClass()) ||
		Buildable->IsA(AFGBuildablePipeHyperPart::StaticClass()) ||
		Buildable->IsA(AFGBuildablePipelinePump::StaticClass())
	)
	{
			return true;
	}

	TArray<UFGConnectionComponent*> ConnectionComponents;
	TArray<UFGFactoryConnectionComponent*> FCC;
	TArray<UFGPipeConnectionComponent*> PC;

	Buildable->GetComponents(FCC);
	Buildable->GetComponents(PC);

	ConnectionComponents.Append(FCC);
	ConnectionComponents.Append(PC);

	if(ConnectionComponents.IsEmpty())
	{
		return true;
	}

	return false;
}

UKPCLNetworkConnectionComponent* AKPCLNetworkCableHologram::GetConnectionComponent() const
{
	if(IsValid(mAttachmentHologram)) return mAttachmentHologram->mConnectionComponent;
	return nullptr;
}
