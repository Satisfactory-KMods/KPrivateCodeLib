// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkCableHologram.h"

#include "Hologram/FGPowerPoleHologram.h"
#include "Kismet/KismetMathLibrary.h"
#include "Network/KPCLNetwork.h"
#include "Network/KPCLNetworkCable.h"
#include "Network/KPCLNetworkConnectionComponent.h"

AKPCLNetworkCableHologram::AKPCLNetworkCableHologram() {
	PrimaryActorTick.bCanEverTick = false;
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

void AKPCLNetworkCableHologram::SetHologramLocationAndRotation(const FHitResult& hitResult) {
	Super::SetHologramLocationAndRotation(hitResult);

	AdjustWire(hitResult);
}

bool AKPCLNetworkCableHologram::TrySnapToActor(const FHitResult& hitResult) {
	const bool Re = Super::TrySnapToActor(hitResult);

	if(Re) {
		AdjustWire(hitResult);
	}

	return Re;
}

void AKPCLNetworkCableHologram::AdjustWire(const FHitResult& hitResult) {
	if(IsValid(mWireMesh)) {
		if(!mWireMesh->bHiddenInGame) {
			FTransform     Transform;
			const FVector  Source = IsValid(mConnections[0]) ? mConnections[0]->GetComponentLocation() : hitResult.Location;
			FVector        TargetLocation = IsValid(mConnections[1]) ? mConnections[1]->GetComponentLocation() : hitResult.Location;
			const float    Lenght = FVector::Distance(Source, TargetLocation);
			const FRotator LotAtRotation = UKismetMathLibrary::FindLookAtRotation(Source, TargetLocation);

			Transform.SetLocation(UKismetMathLibrary::GetForwardVector(LotAtRotation) * (Lenght / 2) + Source);
			Transform.SetRotation(LotAtRotation.Quaternion());
			Transform.SetScale3D(FVector(Lenght / 100, 1, 1));

			mWireMesh->SetWorldTransform(Transform);
			mConnectionMesh->SetWorldLocation(UKismetMathLibrary::GetForwardVector(LotAtRotation) * (Lenght) + Source);

			if(GetActiveAutomaticPoleHologram()) {
				TArray<UStaticMeshComponent*> Meshes;
				GetActiveAutomaticPoleHologram()->GetComponents(Meshes);
				for(UStaticMeshComponent* Component: Meshes) {
					if(Component->GetName().Contains("StaticMeshComponent")) {
						Component->SetHiddenInGame(true);
					}
				}
			}
		} else {
			mWireMesh->SetWorldTransform(FTransform());
		}
	} else if(IsValid(mConnectionMesh)) {
		mConnectionMesh->SetRelativeLocation(FVector());
	}
}

bool AKPCLNetworkCableHologram::TryUpgrade(const FHitResult& hitResult) {
	bool Super = Super::TryUpgrade(hitResult);

	if(hitResult.IsValidBlockingHit() && Super) {
		AKPCLNetworkCable* OtherCable = Cast<AKPCLNetworkCable>(hitResult.GetActor());
		if(IsValid(OtherCable)) {
			SetConnection(0, OtherCable->GetConnection(0));
			SetConnection(1, OtherCable->GetConnection(1));
			AdjustWire(hitResult);
		}
	}

	return Super;
}
