// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Buildable/Holo/KPCLNetworkStorageHologram.h"

#include "Network/Buildings/KPCLNetworkStorage.h"


// Sets default values
AKPCLNetworkStorageHologram::AKPCLNetworkStorageHologram() {
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AKPCLNetworkStorageHologram::Scroll(int32 delta) {
	Super::Scroll(delta);

	mRotate = !mRotate;
}

bool AKPCLNetworkStorageHologram::IsValidHitResult(const FHitResult& hitResult) const {
	if(hitResult.IsValidBlockingHit() && hitResult.GetActor()) {
		if(hitResult.GetActor()->IsA(AKPCLNetworkStorage::StaticClass())) {
			return true;
		}
	}
	return Super::IsValidHitResult(hitResult);
}

void AKPCLNetworkStorageHologram::SetHologramLocationAndRotation(const FHitResult& hitResult) {
	Super::SetHologramLocationAndRotation(hitResult);

	const AKPCLNetworkStorage* AsStorage = Cast<AKPCLNetworkStorage>(hitResult.GetActor());
	const AKPCLNetworkStorage* Default = GetDefaultBuildable<AKPCLNetworkStorage>();

	if(AsStorage && Default) {
		FVector  Loc = AsStorage->GetActorLocation();
		FRotator Rot = AsStorage->GetActorRotation();

		Rot.Yaw += mRotate ? 180.f : 0.f;

		float ZMiddle = Loc.Z + AsStorage->mBuildingHeight / 2;
		float ZInpact = hitResult.ImpactPoint.Z;

		if(ZInpact >= ZMiddle) {
			Loc.Z += AsStorage->mBuildingHeight;
		} else {
			Loc.Z -= AsStorage->mBuildingHeight;
		}

		SetActorLocationAndRotation(Loc, Rot);
	}
}
