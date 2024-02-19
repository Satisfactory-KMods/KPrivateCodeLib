// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/Buildings/KPCLNetworkBuildingAttachment.h"

#include "AbstractInstanceManager.h"
#include "FGFactoryConnectionComponent.h"
#include "KPrivateCodeLibModule.h"
#include "Buildables/FGBuildableWire.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"


void AKPCLNetworkBuildingAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(AKPCLNetworkBuildingAttachment, mAttachedBuilding);
	DOREPLIFETIME(AKPCLNetworkBuildingAttachment, mNetworkAttachmentRules);
	DOREPLIFETIME(AKPCLNetworkBuildingAttachment, mConnections);

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

// Sets default values
AKPCLNetworkBuildingAttachment::AKPCLNetworkBuildingAttachment()
{
	PrimaryActorTick.bCanEverTick = true;
	mAbstractMeshAttachments.Add(FAbstractAttachmentMeshes(UFGFactoryConnectionComponent::StaticClass()));
	mAbstractMeshAttachments.Add(FAbstractAttachmentMeshes(UFGPipeConnectionComponent::StaticClass()));
	mAbstractMeshAttachments.Add(FAbstractAttachmentMeshes(UKPCLNetworkConnectionComponent::StaticClass()));
	mNetworkConnectionComponent = CreateDefaultSubobject<UKPCLNetworkConnectionComponent>(TEXT("NetworkConnectionComponent"));
	mNetworkConnectionComponent->SetupAttachment(RootComponent);
	mNetworkConnectionComponent->Mobility = EComponentMobility::Movable;
}

void AKPCLNetworkBuildingAttachment::BeginPlay()
{
	Super::BeginPlay();
	CacheDataForAttachment();
}

void AKPCLNetworkBuildingAttachment::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	// If the attached building is no longer valid, dismantle this attachment
	// This also removes invalid attachments from the network for example if a mod was removed
	if(HasAuthority() && !IsValid(GetAttachedBuilding()))
	{
		UE_LOG(LogKPCL, Warning, TEXT("Dismantling %s Because Unknown mAttachedBuilding"), *GetName());
		FFunctionGraphTask::CreateAndDispatchWhenReady( [&]( ) {
			Execute_Dismantle(this);
		}, GET_STATID( STAT_TaskGraph_OtherTasks ), nullptr, ENamedThreads::GameThread );
	}
}

void AKPCLNetworkBuildingAttachment::RemoveAttachmentRule(UFGConnectionComponent* Connection)
{
	mNetworkAttachmentRules.Remove(FNetworkAttachmentRules(Connection));
}

void AKPCLNetworkBuildingAttachment::SetOrOverwriteRule(FNetworkAttachmentRules Rule)
{
	if(!Rule.mConnection) return;
	auto ExistingRule = mNetworkAttachmentRules.FindByKey(Rule);
	if(ExistingRule)
	{
		ExistingRule->mItem = Rule.mItem;
		ExistingRule->mMaxAmount = Rule.mMaxAmount;
	}
	else
	{
		mNetworkAttachmentRules.Add(Rule);
	}
}

UFGPowerConnectionComponent* AKPCLNetworkBuildingAttachment::GetParentPowerConnection()
{
	if(mCachedPowerConnection) return mCachedPowerConnection;
	if(IsValid(mAttachedBuilding))
	{
		mCachedPowerConnection = mAttachedBuilding->GetComponentByClass<UFGPowerConnectionComponent>();
		return mCachedPowerConnection;
	}
	return nullptr;
}

void AKPCLNetworkBuildingAttachment::OnAttachmentUpdated()
{
		UE_LOG(LogKPCL, Warning, TEXT("OnAttachmentUpdated"));
#if !UE_SERVER
	if(mInstanceHandles.Num() > 0)
	{
		UE_LOG(LogKPCL, Warning, TEXT("Remove Instance Handles"));
		AAbstractInstanceManager::RemoveInstances( GetWorld(), mInstanceHandles, true );
	}

	UpdatePowerConnectionPosition();

	for (UFGConnectionComponent* AttachmentConnection : GetAllAttachmentConnections())
	{
		FTransform Transform = UKismetMathLibrary::MakeRelativeTransform(AttachmentConnection->GetComponentTransform(), GetActorTransform());
		FAbstractAttachmentMeshes* MeshData = mAbstractMeshAttachments.FindByPredicate([&](const FAbstractAttachmentMeshes& Item)
		{
			if(!IsValid(Item.mTargetClass)) return false;
			if(!IsValid(AttachmentConnection)) return false;
			return AttachmentConnection->IsA(Item.mTargetClass);
		});
		if(MeshData)
		{
			UE_LOG(LogKPCL, Warning, TEXT("Creating Abstract Mesh Instance for AttachmentConnection"));
			FInstanceData InstanceData;
			InstanceData.StaticMesh = MeshData->mMesh;
			InstanceData.Mobility = EComponentMobility::Static;
			InstanceData.RelativeTransform = Transform;
			InstanceData.NumCustomDataFloats= 20;
			InstanceData.bCastShadows = false;
			InstanceData.bCastDistanceFieldShadows = false;
			for(int32 i = 0; i < 20; i++)
			{
				InstanceData.DefaultPerInstanceCustomData.Add(0.0f);
			}

			FInstanceHandle* Handle;
			AAbstractInstanceManager::SetInstanceFromDataStatic(this, GetActorTransform(), InstanceData, Handle);
			mInstanceHandles.Add(Handle);
		}
	}

	ApplyCustomizationData(mCustomizationData);
#endif
}

void AKPCLNetworkBuildingAttachment::UpdatePowerConnectionPosition()
{
#if !UE_SERVER
	if(IsValid(GetParentPowerConnection()) || IsValid(GetAttachedBuilding()))
	{
		FTransform Transform = IsValid(GetParentPowerConnection()) ? GetParentPowerConnection()->GetComponentTransform() : GetAttachedBuilding()->GetActorTransform();
		FAbstractAttachmentMeshes* MeshData = mAbstractMeshAttachments.FindByPredicate([&](const FAbstractAttachmentMeshes& Item)
		{
			return Item.mTargetClass == UFGPowerConnectionComponent::StaticClass();
		});
		if(MeshData)
		{
			FNetworkPowerOffset* OffsetData = mPowerConnectionOffsets.FindByPredicate([&](const FNetworkPowerOffset& Item)
			{
				if(!IsValid(Item.mBuildingClass)) return false;
				if(!IsValid(GetAttachedBuilding())) return false;
				return GetAttachedBuilding()->IsA(Item.mBuildingClass);
			});
			if(OffsetData)
			{
				Transform = Transform * OffsetData->mOffset;
			}

			mNetworkConnectionComponent->SetMobility(EComponentMobility::Movable);
			mNetworkConnectionComponent->SetWorldTransform(Transform);
			mNetworkConnectionComponent->SetMobility(EComponentMobility::Static);

			TInlineComponentArray< AFGBuildableWire* > out_wires;
			mNetworkConnectionComponent->GetWires( out_wires );
			for (AFGBuildableWire* Out_Wire : out_wires)
			{
				if(Out_Wire)
				{
					// We want to do that in the next tick to all things are set correctly
					GetWorldTimerManager().SetTimerForNextTick([&, Out_Wire]()
					{
						Out_Wire->UpdateWireMeshes();
					});
				}
			}

			if(!GetParentPowerConnection())
			{
				UE_LOG(LogKPCL, Warning, TEXT("Creating Abstract Mesh Instance for PowerPole"));
				FInstanceData InstanceData;
				InstanceData.StaticMesh = MeshData->mMesh;
				InstanceData.Mobility = EComponentMobility::Static;
				InstanceData.RelativeTransform = Transform;
				InstanceData.NumCustomDataFloats= 20;
				InstanceData.bCastShadows = false;
				InstanceData.bCastDistanceFieldShadows = false;
				for(int32 i = 0; i < 20; i++)
				{
					InstanceData.DefaultPerInstanceCustomData.Add(0.0f);
				}

				FInstanceHandle* Handle;
				AAbstractInstanceManager::SetInstanceFromDataStatic(this, GetActorTransform(), InstanceData, Handle);
				mInstanceHandles.Add(Handle);
			}
		}
	}
#endif
}

TArray<UFGConnectionComponent*> AKPCLNetworkBuildingAttachment::GetAllAttachmentConnections() const
{
	TArray<UFGConnectionComponent*> Attachments;
	Attachments.Append(mCachedBeltConnectionsInput);
	Attachments.Append(mCachedBeltConnectionsOutput);
	Attachments.Append(mCachedPipeConnectionsInput);
	Attachments.Append(mCachedPipeConnectionsOutput);
	return Attachments;
}

void AKPCLNetworkBuildingAttachment::CacheDataForAttachment()
{
	UE_LOG(LogKPCL, Warning, TEXT("CacheDataForAttachment"));
	if(HasAuthority() && IsAttached())
	{
		UE_LOG(LogKPCL, Warning, TEXT("IsAttached to %s"), *mAttachedBuilding->GetName());
		TArray<UFGConnectionComponent*> Connections;
		mAttachedBuilding->GetComponents(Connections);

		for (UFGConnectionComponent* Connection : Connections)
		{
			if (UFGFactoryConnectionComponent* FactoryConnection = Cast<UFGFactoryConnectionComponent>(Connection))
			{
				if(FactoryConnection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT)
				{
					mCachedBeltConnectionsInput.Add(FactoryConnection);
				}
				else if(FactoryConnection->GetDirection() == EFactoryConnectionDirection::FCD_OUTPUT)
				{
					mCachedBeltConnectionsOutput.Add(FactoryConnection);
				}
			}
			else if (UFGPipeConnectionComponent* PipeConnection = Cast<UFGPipeConnectionComponent>(Connection))
			{
				if(PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_CONSUMER)
				{
					mCachedPipeConnectionsInput.Add(PipeConnection);
				}
				else if(PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_PRODUCER)
				{
					mCachedPipeConnectionsOutput.Add(PipeConnection);
				}
			}
		}
		InitConnections();
	} else if(!IsAttached()) {
		UE_LOG(LogKPCL, Error, TEXT("Attachment %s is not attached to a building!"), *GetName());
	}
	OnAttachmentUpdated();
}

void AKPCLNetworkBuildingAttachment::InitConnections()
{
	if(!HasAuthority() || !IsValid(mAttachedBuilding)) return;
	TArray<UFGConnectionComponent*> Connections = GetAllAttachmentConnections();
	GetInventory()->Resize(FMath::Max(1, Connections.Num()));
	int32 Idx = 0;

	for (UFGConnectionComponent* AttachmentConnection : Connections)
	{
		FConnectionPair* ExistingConnection = mConnections.FindByPredicate([&](const FConnectionPair& Item)
		{
			return AttachmentConnection == Item.mParent;
		});
		if(!ExistingConnection)
		{
			FConnectionPair NewPair;
			NewPair.mParent = AttachmentConnection;
			if (UFGFactoryConnectionComponent* FactoryConnection = Cast<UFGFactoryConnectionComponent>(AttachmentConnection))
			{
				if(FactoryConnection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT || FactoryConnection->GetDirection() == EFactoryConnectionDirection::FCD_OUTPUT)
				{
					UFGFactoryConnectionComponent* BuildingConnection = NewObject<UFGFactoryConnectionComponent>(this, FName("Slave_" + FactoryConnection->GetName()));
					NewPair.mBuilding = BuildingConnection;

					BuildingConnection->AttachToComponent(GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, false), NAME_None);
					BuildingConnection->mDirection = FactoryConnection->GetCompatibleSnapDirection();
					BuildingConnection->SetRelativeTransform(FactoryConnection->GetRelativeTransform());
					BuildingConnection->SetupAttachment(GetRootComponent());
					BuildingConnection->SetInventory(GetInventory());
					BuildingConnection->SetInventoryAccessIndex(Idx);
					BuildingConnection->RegisterComponentWithWorld(GetWorld());

					FactoryConnection->ClearConnection();
					FactoryConnection->SetConnection(BuildingConnection);
				}
			}
			else if (UFGPipeConnectionComponent* PipeConnection = Cast<UFGPipeConnectionComponent>(AttachmentConnection))
			{
				if(PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_CONSUMER || PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_PRODUCER)
				{
					UFGPipeConnectionComponent* BuildingConnection = NewObject<UFGPipeConnectionComponent>(this, FName("Slave_" + PipeConnection->GetName()));
					NewPair.mBuilding = BuildingConnection;

					BuildingConnection->AttachToComponent(GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, false), NAME_None);
					BuildingConnection->SetPipeConnectionType(PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_CONSUMER ? EPipeConnectionType::PCT_PRODUCER : EPipeConnectionType::PCT_CONSUMER);
					BuildingConnection->SetupAttachment(GetRootComponent());
					BuildingConnection->SetRelativeTransform(PipeConnection->GetRelativeTransform());
					BuildingConnection->SetInventory(GetInventory());
					BuildingConnection->SetInventoryAccessIndex(Idx);
					BuildingConnection->RegisterComponentWithWorld(GetWorld());

					PipeConnection->ClearConnection();
					PipeConnection->SetConnection(BuildingConnection);
				}
			}

			if( IsValid(NewPair.mBuilding))
			{
				mConnections.Add(NewPair);
			} 
		}
		Idx++;
	}
}

void AKPCLNetworkBuildingAttachment::AttachTo(AFGBuildable* Building, bool ConnectFromHologram)
{
	mAttachedBuilding = Building;
	if(!ConnectFromHologram)
	{
		CacheDataForAttachment();
	}
}

bool AKPCLNetworkBuildingAttachment::GetRequiredItems(TArray<FItemAmount>& Items)
{
	return false;
}

bool AKPCLNetworkBuildingAttachment::PushToNetwork(TArray<FItemAmount>& ToPush, float MaxSolidBytes,
	float MaxFluidBytes)
{
	return false;
}

bool AKPCLNetworkBuildingAttachment::GetFromNetwork(const TArray<FItemAmount>& ToReceive)
{
	return false;
}

