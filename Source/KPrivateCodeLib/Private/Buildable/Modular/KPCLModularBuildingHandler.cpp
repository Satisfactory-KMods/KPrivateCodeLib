﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildable/Modular/KPCLModularBuildingHandler.h"

#include <string>

#include "FGPowerConnectionComponent.h"
#include "Buildable/Modular/KPCLModularBuildingInterface.h"
#include "Kismet/KismetMathLibrary.h"

bool FAttachmentPointData::operator==( const AFGBuildable* other ) const
{
	return other == mSnappedActors;
}

// Start FAttachmentData
AFGBuildable* FAttachmentData::GetActorFromIndex( int Index ) const
{
	if( mAttachmentPointDatas.IsValidIndex( Index ) )
	{
		return mAttachmentPointDatas[ Index ].mSnappedActors;
	}
	return nullptr;
}

AFGBuildable* FAttachmentData::GetActorFromLocation( FTransform TestLocation, FTransform& OutLocation, float MaxDistance ) const
{
	AFGBuildable* ClosedActor = nullptr;
	OutLocation.SetLocation( FVector() );
	float LastHit = MaxDistance;
	if( mAttachmentPointDatas.Num() > 0 )
	{
		for( int i = 0; i < mAttachmentPointDatas.Num(); ++i )
		{
			float Distance = FVector::Distance( mAttachmentPointDatas[ i ].mLocations.GetLocation(), TestLocation.GetLocation() );
			if( Distance < LastHit )
			{
				OutLocation = mAttachmentPointDatas[ i ].mLocations;
				LastHit = Distance;
				ClosedActor = mAttachmentPointDatas[ i ].mSnappedActors;
			}
		}
	}
	return ClosedActor;
}

bool FAttachmentData::HasInRange( FTransform TestLocation, float MaxDistance ) const
{
	if( mAttachmentPointDatas.Num() > 0 )
	{
		for( int i = 0; i < mAttachmentPointDatas.Num(); ++i )
		{
			float Distance = FVector::Distance( mAttachmentPointDatas[ i ].mLocations.GetLocation(), TestLocation.GetLocation() );
			if( Distance <= MaxDistance && !mAttachmentPointDatas[ i ].IsAttached() )
			{
				return true;
			}
		}
	}
	return false;
}

bool FAttachmentData::IsLocationFree( FTransform TestLocation, FTransform& OutLocation, float MaxDistance ) const
{
	if( HasInRange( TestLocation, MaxDistance ) )
	{
		const bool DontAttached = GetActorFromLocation( TestLocation, OutLocation, MaxDistance ) == nullptr;
		return DontAttached && FVector::Distance( OutLocation.GetLocation(), FVector() ) > 200.f && OutLocation.GetLocation().Z > -20000000;
	}
	return false;
}

void FAttachmentData::RemoveActorFromData( AFGBuildable* Actor )
{
	FAttachmentPointData* Data = mAttachmentPointDatas.FindByKey( Actor );
	if( Data )
	{
		Data->Clear();
	}
}

int32 FAttachmentData::AddActorToData( AFGBuildable* Actor, FTransform Location )
{
	if( ensure( Actor ) )
	{
		for( int i = 0; i < mAttachmentPointDatas.Num(); ++i )
		{
			if( mAttachmentPointDatas[ i ].IsAttached() )
			{
				continue;
			}

			if( FVector::Dist( mAttachmentPointDatas[ i ].mLocations.GetLocation(), Location.GetLocation() ) < 50.f )
			{
				mAttachmentPointDatas[ i ].mSnappedActors = Actor;
				return i;
			}
		}
	}
	return INDEX_NONE;
}

void FAttachmentData::Init( TArray< FTransform >& Transforms )
{
	if( mAttachmentPointDatas.Num() != Transforms.Num() )
	{
		mAttachmentPointDatas.SetNum( Transforms.Num() );
	}

	for( int i = 0; i < Transforms.Num(); ++i )
	{
		mAttachmentPointDatas[ i ].mLocations = Transforms[ i ];
	}
}

bool FAttachmentData::HasSpace() const
{
	for( FAttachmentPointData PointData : mAttachmentPointDatas )
	{
		if( !PointData.mSnappedActors )
		{
			return true;
		}
	}
	return false;
}

// End FAttachmentData

// Start UKPCLModularBuildingHandler
UKPCLModularBuildingHandler::UKPCLModularBuildingHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	this->SetIsReplicatedByDefault( true );
	this->bAutoActivate = true;
}

void UKPCLModularBuildingHandler::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UKPCLModularBuildingHandler, mAttachmentDatas );
}

void UKPCLModularBuildingHandler::InitArrays()
{
	TMap< TSubclassOf< UKPCLModularAttachmentDescriptor >, FAttachmentLocations > LocMap;
	if( GetLocationMap( LocMap ) )
	{
		for( TTuple< TSubclassOf< UKPCLModularAttachmentDescriptor >, FAttachmentLocations > Map : LocMap )
		{
			int32 Index = FindAttachmentIndex( Map.Key );
			if( Index == INDEX_NONE )
			{
				int32 NewIndex = 0;
				for( FAttachmentInfos& Information : mAttachmentInformations )
				{
					if( !Information.mAttachmentClass )
					{
						Information.mAttachmentClass = Map.Key;
						Index = NewIndex;
						break;
					}
					NewIndex++;
				}
			}

			if( Index != INDEX_NONE )
			{
				FAttachmentInfos& Info = mAttachmentInformations[ Index ];
				TArray< FTransform > Transforms;
				Map.Value.GetTransformSortedByIndex( Transforms );
				Info.mSnapWorldLocations.Append( Transforms );
			}
		}
	}


	if( mAttachmentDatas.Num() != mAttachmentInformations.Num() )
	{
		mAttachmentDatas.SetNum( mAttachmentInformations.Num() );
	}

	for( int i = 0; i < mAttachmentInformations.Num(); ++i )
	{
		mAttachmentDatas[ i ].Init( mAttachmentInformations[ i ].mSnapWorldLocations );
		for( const FAttachmentPointData& AttachmentPointData : mAttachmentDatas[ i ].mAttachmentPointDatas )
		{
			if( AttachmentPointData.IsAttached() )
			{
				IKPCLModularBuildingInterface::Execute_SetMasterBuilding( AttachmentPointData.mSnappedActors, Cast< AFGBuildable >( GetOwner() ) );
			}
		}
	}
}

bool UKPCLModularBuildingHandler::AddNewActorToAttachment( AFGBuildable* Actor, TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform Location, float Distance )
{
	const int AttachmentIndex = FindAttachmentIndex( Attachment );

	if( AttachmentIndex == INDEX_NONE )
	{
		return false;
	}

	FTransform Dummy;
	if( mAttachmentDatas[ AttachmentIndex ].IsLocationFree( Location, Dummy, Distance ) )
	{
		int32 ModularIndex = mAttachmentDatas[ AttachmentIndex ].AddActorToData( Actor, Location );
		if( ModularIndex != INDEX_NONE )
		{
			IKPCLModularBuildingInterface::Execute_SetMasterBuilding( Actor, Cast< AFGBuildable >( GetOwner() ) );
			IKPCLModularBuildingInterface::Execute_ApplyModularIndex( Actor, ModularIndex );
			TryToConnectPower( Actor );
			OnRep_AttachmentDatas();
			return true;
		}
	}
	return false;
}

void UKPCLModularBuildingHandler::AttachedActorRemoved( AFGBuildable* Actor )
{
	for( FAttachmentData& AttachmentData : mAttachmentDatas )
	{
		AttachmentData.RemoveActorFromData( Actor );
	}
	Super::AttachedActorRemoved( Actor );
	OnRep_AttachmentDatas();
}

bool UKPCLModularBuildingHandler::CanAttachToLocation( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform TestLocation, FTransform& OutLocation, float Distance ) const
{
	if( CanAttach( Attachment ) )
	{
		const int AttachmentIndex = FindAttachmentIndex( Attachment );
		if( AttachmentIndex != INDEX_NONE )
		{
			if( !mAttachmentDatas[ AttachmentIndex ].HasSpace() )
			{
				return false;
			}
			return mAttachmentDatas[ AttachmentIndex ].IsLocationFree( TestLocation, OutLocation, Distance );
		}
	}
	return false;
}

bool UKPCLModularBuildingHandler::GetSnapPointInRange( FTransform TestLocation, FTransform& SnapLocation, float AllowedDistance, TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment )
{
	if( Attachment )
	{
		const int SnapIndex = FindAttachmentIndex( Attachment );
		if( SnapIndex > 0 )
		{
			if( mAttachmentInformations[ SnapIndex ].mSnapWorldLocations.Num() > 0 )
			{
				for( int i = 0; mAttachmentInformations[ SnapIndex ].mSnapWorldLocations.Num() < i; ++i )
				{
					FTransform CheckLocation = mAttachmentInformations[ SnapIndex ].mSnapWorldLocations[ i ];
					if( FVector::Distance( CheckLocation.GetLocation(), TestLocation.GetLocation() ) <= AllowedDistance )
					{
						SnapLocation = CheckLocation;
						return true;
					}
				}
			}
		}
	}
	return false;
}

AFGBuildable* UKPCLModularBuildingHandler::GetAttachedActorByClass( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment )
{
	const int AttachmentIndex = FindAttachmentIndex( Attachment );
	if( AttachmentIndex >= 0 )
	{
		if( mAttachmentDatas[ AttachmentIndex ].mAttachmentPointDatas.IsValidIndex( 0 ) )
		{
			return mAttachmentDatas[ AttachmentIndex ].mAttachmentPointDatas[ 0 ].mSnappedActors;
		}
	}
	return nullptr;
}

TArray< AFGBuildable* > UKPCLModularBuildingHandler::GetAttachedActorsByClass( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment )
{
	TArray< AFGBuildable* > Out;
	const int AttachmentIndex = FindAttachmentIndex( Attachment );
	if( AttachmentIndex >= 0 )
	{
		for( FAttachmentPointData AttachmentPointData : mAttachmentDatas[ AttachmentIndex ].mAttachmentPointDatas )
		{
			if( AttachmentPointData.IsAttached() )
			{
				Out.Add( AttachmentPointData.mSnappedActors );
			}
		}
	}
	return Out;
}

void UKPCLModularBuildingHandler::GetAttachedActors( TArray< AFGBuildable* >& Out )
{
	for( FAttachmentData Data : mAttachmentDatas )
	{
		for( FAttachmentPointData AttachmentPointData : Data.mAttachmentPointDatas )
		{
			if( AttachmentPointData.IsAttached() )
			{
				Out.Add( AttachmentPointData.mSnappedActors );
			}
		}
	}
}

int UKPCLModularBuildingHandler::FindAttachmentIndex( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) const
{
	if( Attachment == nullptr )
	{
		return INDEX_NONE;
	}

	return mAttachmentInformations.IndexOfByKey( Attachment );
}

void UKPCLModularBuildingHandler::OnRep_AttachmentDatas()
{
	IKPCLModularBuildingInterface::Execute_OnModulesUpdated( GetOwner() );
}

// End UKPCLModularBuildingHandler
