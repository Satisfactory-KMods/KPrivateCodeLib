// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGCircuitConnectionComponent.h"
#include "KPCLModularAttachmentDescriptor.h"
#include "KPCLModularBuildingHandlerBase.h"
#include "Buildables/FGBuildable.h"

#include "KPCLModularBuildingHandler.generated.h"

USTRUCT( BlueprintType )
struct FAttachmentInfos
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadOnly )
	TSubclassOf< UKPCLModularAttachmentDescriptor > mAttachmentClass;

	TArray< FTransform > mSnapWorldLocations;

	bool operator==( TSubclassOf< UKPCLModularAttachmentDescriptor > other ) const
	{
		return mAttachmentClass == other;
	};
};

USTRUCT( BlueprintType )
struct FAttachmentPointData
{
	GENERATED_BODY()

	UPROPERTY( BlueprintReadOnly )
	FTransform mLocations = {};

	UPROPERTY( SaveGame, BlueprintReadOnly )
	AFGBuildable* mSnappedActors = nullptr;

	void Clear()
	{
		mSnappedActors = nullptr;
	}

	bool IsAttached() const
	{
		return mSnappedActors != nullptr;
	}

	bool operator==( const AFGBuildable* other ) const;
};

USTRUCT( BlueprintType )
struct FAttachmentData
{
	GENERATED_BODY()

public:
	UPROPERTY( SaveGame, BlueprintReadOnly )
	TArray< FAttachmentPointData > mAttachmentPointDatas;

	AFGBuildable* GetActorFromIndex( int Index ) const;
	AFGBuildable* GetActorFromLocation( FTransform TestLocation, FTransform& OutLocation, float MaxDistance ) const;
	bool IsLocationFree( FTransform TestLocation, FTransform& OutLocation, float MaxDistance ) const;
	void RemoveActorFromData( AFGBuildable* Actor );
	int32 AddActorToData( AFGBuildable* Actor, FTransform Location );
	void Init( TArray< FTransform >& Transforms );
	bool HasSpace() const;
	bool HasInRange( FTransform TestLocation, float MaxDistance ) const;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KPRIVATECODELIB_API UKPCLModularBuildingHandler : public UKPCLModularBuildingHandlerBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UKPCLModularBuildingHandler();

	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

	virtual void InitArrays() override;

	virtual bool AddNewActorToAttachment( AFGBuildable* Actor, TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform Location, float Distance = 500.0f ) override;

	virtual void AttachedActorRemoved( AFGBuildable* Actor ) override;

	virtual bool CanAttachToLocation( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform TestLocation, FTransform& OutLocation, float Distance = 500.0f ) const override;
	virtual bool GetSnapPointInRange( FTransform TestLocation, FTransform& SnapLocation, float AllowedDistance, TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) override;
	virtual AFGBuildable* GetAttachedActorByClass( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) override;

	template< class T >
	T* GetClosedActorFromLocation( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform Location );

	template< class T >
	T* GetActorFromModularIndex( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, int32 Index );

	virtual TArray< AFGBuildable* > GetAttachedActorsByClass( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) override;
	virtual void GetAttachedActors( TArray< AFGBuildable* >& Out ) override;
	virtual int FindAttachmentIndex( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) const override;

	UFUNCTION()
	void OnRep_AttachmentDatas();

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="Modular Handler", meta=(EditFixedSize) )
	TArray< FAttachmentInfos > mAttachmentInformations = { FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos() };

	UPROPERTY( SaveGame, BlueprintReadOnly, Category="Modular Handler", Replicated, ReplicatedUsing=OnRep_AttachmentDatas, meta=(EditFixedSize) )
	TArray< FAttachmentData > mAttachmentDatas = { FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData() };
};

template< class T >
T* UKPCLModularBuildingHandler::GetClosedActorFromLocation( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform Location )
{
	const int AttachmentIndex = FindAttachmentIndex( Attachment );
	if( AttachmentIndex >= 0 )
	{
		FTransform PH;
		return Cast< T >( mAttachmentDatas[ AttachmentIndex ].GetActorFromLocation( Location, PH, 1000000.f ) );
	}
	return nullptr;
}

template< class T >
T* UKPCLModularBuildingHandler::GetActorFromModularIndex( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, int32 Index )
{
	const int AttachmentIndex = FindAttachmentIndex( Attachment );
	if( AttachmentIndex >= 0 )
	{
		if( mAttachmentDatas[ AttachmentIndex ].mAttachmentPointDatas.IsValidIndex( Index ) )
		{
			return Cast< T >( mAttachmentDatas[ AttachmentIndex ].mAttachmentPointDatas[ Index ].mSnappedActors );
		}
	}
	return nullptr;
}
