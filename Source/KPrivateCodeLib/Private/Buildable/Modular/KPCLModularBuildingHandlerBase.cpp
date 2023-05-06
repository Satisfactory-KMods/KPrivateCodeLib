// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildable/Modular/KPCLModularBuildingHandlerBase.h"

#include "FGPowerConnectionComponent.h"
#include "Buildable/Modular/KPCLModularBuildingBase.h"
#include "Buildable/Modular/KPCLModularBuildingInterface.h"
#include "Buildable/Modular/KPCLModularSnapPoint.h"
#include "Network/KPCLNetwork.h"
#include "Subsystems/ResourceNodes/KBFLResourceNodeDescriptor_ResourceNode.h"

bool UKPCLModularBuildingHandlerBase::HasAuthority() const
{
	if( GetOwner() )
	{
		return GetOwner()->HasAuthority();
	}
	return false;
}

// Start UKPCLModularBuildingHandlerBase
UKPCLModularBuildingHandlerBase::UKPCLModularBuildingHandlerBase()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Should we move that to Replication Detail actor?
	this->SetIsReplicatedByDefault( true );
	this->bAutoActivate = true;
}

void UKPCLModularBuildingHandlerBase::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
}

void UKPCLModularBuildingHandlerBase::GetAttachedActorsOfType( TArray< AFGBuildable* >& Out, uint8 Type )
{
	GetAttachedActors( Out );
	if( Out.Num() > 0 )
	{
		for( AFGBuildable* Element : Out )
		{
			if( !IKPCLModularBuildingInterface::Execute_IsModuleTypeOf( Element, Type ) )
			{
				Out.Remove( Element );
			}
		}
	}
}

void UKPCLModularBuildingHandlerBase::BeginPlay()
{
	Super::BeginPlay();
	InitArrays();

	if( HasAuthority() )
	{
		this->SetIsReplicatedByDefault( true );
	}
}

int UKPCLModularBuildingHandlerBase::FindAttachmentIndex( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) const
{
	return INDEX_NONE;
}

void UKPCLModularBuildingHandlerBase::AttachedActorRemoved( AFGBuildable* Actor )
{
	UE_LOG( LogTemp, Warning, TEXT("AttachedActorRemoved") );

	// Call on owner that removed was done
	if( UKismetSystemLibrary::DoesImplementInterface( GetOwner(), UKPCLModularBuildingInterface::StaticClass() ) )
	{
		IKPCLModularBuildingInterface::Execute_POST_RemoveAttachedActor( GetOwner() );
	}
}

void UKPCLModularBuildingHandlerBase::TryToConnectPower( AFGBuildable* Actor )
{
	if( OverwriteTryToConnectPower.IsBound() )
	{
		OverwriteTryToConnectPower.Execute( Actor );
		return;
	}

	if( GetOwner() && Actor )
	{
		UFGPowerConnectionComponent* ConnectionA = Cast< UFGPowerConnectionComponent >( GetOwner()->GetComponentByClass( UFGPowerConnectionComponent::StaticClass() ) );
		UFGPowerConnectionComponent* ConnectionB = Cast< UFGPowerConnectionComponent >( Actor->GetComponentByClass( UFGPowerConnectionComponent::StaticClass() ) );
		if( ConnectionB && ConnectionA )
		{
			if( !ConnectionA->HasHiddenConnection( ConnectionB ) && ( ConnectionB->IsHidden() || ConnectionA->IsHidden() ) )
			{
				ConnectionA->AddHiddenConnection( ConnectionB );
			}
		}
	}
}

bool UKPCLModularBuildingHandlerBase::GetLocationMap( TMap< TSubclassOf< UKPCLModularAttachmentDescriptor >, FAttachmentLocations >& OutMap )
{
	OutMap.Empty();
	AFGBuildable* Building = Cast< AFGBuildable >( GetOwner() );

	TSubclassOf< AFGDecorationTemplate > Template = Building->GetDecorationTemplate();
	if( Template )
	{
		TArray< UKPCLModularSnapPoint* > SnapPoints = AFGDecorationTemplate::GetComponentsFromSubclass< UKPCLModularSnapPoint >( Template );
		if( SnapPoints.Num() > 0 )
		{
			for( UKPCLModularSnapPoint* SnapPoint : SnapPoints )
			{
				if( SnapPoint->mAttachmentClass )
				{
					UKPCLModularSnapPoint* SnapComponent = NewObject< UKPCLModularSnapPoint >( Building, NAME_None, RF_NoFlags, SnapPoint );
					if( SnapComponent )
					{
						SnapComponent->SetRelativeTransform( SnapPoint->GetRelativeTransform() );
						SnapComponent->SetupAttachment( Building->GetRootComponent() );
						SnapComponent->RegisterComponent();
						FTransform WorldTransform = SnapComponent->GetComponentTransform();

						FAttachmentPointLocation LocationInformation;
						LocationInformation.mLocation = WorldTransform;
						LocationInformation.mIndex = SnapComponent->mIndex;
						LocationInformation.mMaxStackCount = SnapComponent->mStackerCount;
						if( FAttachmentLocations* Array = OutMap.Find( SnapPoint->mAttachmentClass ) )
						{
							Array->mLocations.Add( LocationInformation );
						}
						else
						{
							FAttachmentLocations TransformArray = FAttachmentLocations();
							TransformArray.mLocations.Add( LocationInformation );
							OutMap.Add( SnapPoint->mAttachmentClass, TransformArray );
						}
						SnapComponent->DestroyComponent();
					}
				}
			}
		}

		return OutMap.Num() > 0;
	}
	return false;
}

bool UKPCLModularBuildingHandlerBase::CanAttach( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) const
{
	return FindAttachmentIndex( Attachment ) != INDEX_NONE;
}

AFGBuildable* UKPCLModularBuildingHandlerBase::GetAttachedActorByClass( TSubclassOf<UKPCLModularAttachmentDescriptor> Attachment )
{
	return nullptr;
}

TArray<AFGBuildable*> UKPCLModularBuildingHandlerBase::GetAttachedActorsByClass( TSubclassOf<UKPCLModularAttachmentDescriptor> Attachment )
{
	return {};
}

void UKPCLModularBuildingHandlerBase::GetAttachedActorsByIndex( TArray<AFGBuildable*>& Out, uint8 index )
{
}

void UKPCLModularBuildingHandlerBase::GetAttachedActors( TArray<AFGBuildable*>& Out )
{
}

// End UKPCLModularBuildingHandlerBase
