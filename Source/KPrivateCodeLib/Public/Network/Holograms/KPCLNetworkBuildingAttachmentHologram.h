// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Hologram/FGFactoryHologram.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "KPCLNetworkBuildingAttachmentHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkBuildingAttachmentHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AKPCLNetworkBuildingAttachmentHologram();

	void SetTarget(AFGBuildable* Target);

	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

	void UpdateMeshes();
	void SetupOutlines();
	void ClearAllMeshes();
	void CreateMesh(FTransform Transform, UStaticMesh* Mesh);
	virtual void PostHologramPlacement(const FHitResult& hitResult) override;

	virtual void ConfigureComponents(AFGBuildable* inBuildable) const override;

	UPROPERTY(Replicated)
	AFGBuildable* mTarget;

	UPROPERTY(EditAnywhere, Category = "KMods|Attachment")
	UKPCLNetworkConnectionComponent* mConnectionComponent;
};
