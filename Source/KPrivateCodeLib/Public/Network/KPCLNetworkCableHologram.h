// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGConstructDisqualifier.h"
#include "Buildings/KPCLNetworkBuildingAttachment.h"
#include "Buildings/KPCLNetworkCore.h"
#include "Hologram/FGWireHologram.h"
#include "Holograms/KPCLNetworkBuildingAttachmentHologram.h"
#include "KPCLNetworkCableHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCableHologram: public AFGWireHologram {
	GENERATED_BODY()

	public:
		AKPCLNetworkCableHologram();

		virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

		virtual AActor* Construct( TArray< AActor* >& out_children, FNetConstructionID netConstructionID ) override;
		virtual void SpawnChildren(AActor* hologramOwner, FVector spawnLocation, APawn* hologramInstigator) override;
		virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
		virtual void BeginPlay() override;
		virtual void CheckValidPlacement() override;
		virtual bool TryUpgrade(const FHitResult& hitResult) override;
		void ConnectToTarget(AFGBuildable* target);
		int32 GetConnectionToSet() const;
		bool IsConnectedToChild() const;

		bool IsBuildingDisallowed(AFGBuildable* Buildable) const;

		UKPCLNetworkConnectionComponent* GetConnectionComponent() const;

	private:
		UPROPERTY()
		UStaticMeshComponent* mConnectionMesh;

		UPROPERTY(Replicated)
		AKPCLNetworkBuildingAttachmentHologram* mAttachmentHologram;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Attachment")
		TSubclassOf<UFGRecipe> mAttachmentRecipe;

		UPROPERTY(EditDefaultsOnly, Category = "KMods|Attachment")
		TArray<TSubclassOf<AFGBuildable>> mDisabledBuildings;
};

UCLASS()
class KPRIVATECODELIB_API UKPCLCDUniqueCore: public UFGConstructDisqualifier {
	GENERATED_BODY()

	UKPCLCDUniqueCore() {
		mDisqfualifyingText = NSLOCTEXT("KPrivateCodeLib", "ConstructDisqualifier_UniqueCore", "Both Circuits has a Nexus! Only one per network is allowed!");
	}
};
