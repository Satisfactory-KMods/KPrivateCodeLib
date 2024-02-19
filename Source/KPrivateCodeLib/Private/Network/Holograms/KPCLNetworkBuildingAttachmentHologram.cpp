// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/Holograms/KPCLNetworkBuildingAttachmentHologram.h"

#include "KPrivateCodeLibModule.h"
#include "BFL/KBFL_Player.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkBuildingAttachment.h"


AKPCLNetworkBuildingAttachmentHologram::AKPCLNetworkBuildingAttachmentHologram(): mTarget(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	mConnectionComponent = CreateDefaultSubobject<UKPCLNetworkConnectionComponent>(TEXT("ConnectionComponent"));
	mConnectionComponent->Mobility = EComponentMobility::Movable;
	mConnectionComponent->SetupAttachment(RootComponent);
}

void AKPCLNetworkBuildingAttachmentHologram::SetTarget(AFGBuildable* Target)
{
	if(mTarget != Target)
	{
		mTarget = Target;
		if(IsValid(mTarget))
		{
			SetActorTransform(mTarget->GetTransform());
			UpdateMeshes();
			OnSnap();
		} else
		{
			ClearAllMeshes();
		}
		SetupOutlines();
	}
}

void AKPCLNetworkBuildingAttachmentHologram::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(AKPCLNetworkBuildingAttachmentHologram, mTarget);
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AKPCLNetworkBuildingAttachmentHologram::UpdateMeshes()
{
	ClearAllMeshes();
	AKPCLNetworkBuildingAttachment* Attachment = GetDefaultBuildable<AKPCLNetworkBuildingAttachment>();

	UFGPowerConnectionComponent* CachedPowerConnection = mTarget->GetComponentByClass<UFGPowerConnectionComponent>();

	if(IsValid(CachedPowerConnection) || IsValid(mTarget))
	{
		FTransform Transform = CachedPowerConnection ? CachedPowerConnection->GetComponentTransform() : mTarget->GetActorTransform();
		FAbstractAttachmentMeshes* MeshData = Attachment->mAbstractMeshAttachments.FindByPredicate([&](const FAbstractAttachmentMeshes& Item)
		{
			return Item.mTargetClass == UFGPowerConnectionComponent::StaticClass();
		});

		if(MeshData)
		{
			FNetworkPowerOffset* OffsetData = Attachment->mPowerConnectionOffsets.FindByPredicate([&](const FNetworkPowerOffset& Item)
			{
				if(!IsValid(Item.mBuildingClass)) return false;
				return mTarget->IsA(Item.mBuildingClass);
			});
			if(OffsetData)
			{
				Transform = Transform + OffsetData->mOffset;
			}

			UE_LOG(LogKPCL, Error, TEXT("Set Relative Transform for Power Connection Component"));
			mConnectionComponent->SetWorldTransform(Transform);
			if(!CachedPowerConnection)
			{
				CreateMesh(Transform, MeshData->mMesh);
			}
		}
	} else
	{
		UE_LOG(LogKPCL, Error, TEXT("No Power Connection Component found"));
	}

	TArray<UFGConnectionComponent*> ConnectionComponents;
	mTarget->GetComponents(ConnectionComponents);

	for (UFGConnectionComponent* AttachmentConnection : ConnectionComponents)
	{
		FTransform Transform = AttachmentConnection->GetComponentTransform();
		FAbstractAttachmentMeshes* MeshData = Attachment->mAbstractMeshAttachments.FindByPredicate([&](const FAbstractAttachmentMeshes& Item)
		{
			if(!IsValid(Item.mTargetClass)) return false;
			if(!IsValid(AttachmentConnection)) return false;
			return AttachmentConnection->GetClass()->IsChildOf(Item.mTargetClass);
		});
		if(MeshData)
		{
			CreateMesh(Transform, MeshData->mMesh);
		}
	}
}

void AKPCLNetworkBuildingAttachmentHologram::SetupOutlines()
{
	AFGCharacterPlayer* Player = UKBFL_Player::GetFGCharacter(GetWorld());
	if(IsValid(Player))
	{
		UFGOutlineComponent* OutlineComponent = Player->GetOutline();
		if(IsValid(OutlineComponent))
		{
			if(IsValid(mTarget))
			{
				OutlineComponent->ShowOutline(mTarget, EOutlineColor::OC_HOLOGRAMLINE);
			} else
			{
				OutlineComponent->HideOutline();
			}
		}
	}
}

void AKPCLNetworkBuildingAttachmentHologram::ClearAllMeshes()
{
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);
	for(UStaticMeshComponent* Mesh : Meshes)
	{
		Mesh->DestroyComponent();
	}
}

void AKPCLNetworkBuildingAttachmentHologram::CreateMesh(FTransform Transform, UStaticMesh* Mesh)
{
	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this);
	MeshComponent->SetStaticMesh(Mesh);
	MeshComponent->SetWorldTransform(Transform);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->RegisterComponent();
}

void AKPCLNetworkBuildingAttachmentHologram::PostHologramPlacement(const FHitResult& hitResult)
{
}

void AKPCLNetworkBuildingAttachmentHologram::ConfigureComponents(AFGBuildable* inBuildable) const
{
	AKPCLNetworkBuildingAttachment* Attachment = Cast<AKPCLNetworkBuildingAttachment>(inBuildable);
	fgcheck(Attachment);
	Attachment->mNetworkConnectionComponent->SetMobility(EComponentMobility::Movable);
	Attachment->mNetworkConnectionComponent->SetWorldTransform(mConnectionComponent->GetComponentTransform());
	Attachment->mNetworkConnectionComponent->SetMobility(EComponentMobility::Static);
	Attachment->AttachTo(mTarget, true);
	Super::ConfigureActor(inBuildable);
}

