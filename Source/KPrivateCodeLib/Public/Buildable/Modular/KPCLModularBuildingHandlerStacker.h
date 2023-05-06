// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGCircuitConnectionComponent.h"
#include "KPCLModularAttachmentDescriptor.h"
#include "KPCLModularBuildingHandlerBase.h"

#include "KPCLModularBuildingHandlerStacker.generated.h"

USTRUCT( BlueprintType )
struct KPRIVATECODELIB_API FAttachmentInfosStacker
{
	GENERATED_BODY()
	FAttachmentInfosStacker()
	{
	};

	UPROPERTY( EditAnywhere, BlueprintReadOnly )
	TSubclassOf< UKPCLModularAttachmentDescriptor > mAttachmentClass = nullptr;

	int32 mMaxStackingModuleCount = 5;
	FTransform mWorldMainSnapPoint = FTransform();

	bool operator==( TSubclassOf< UKPCLModularAttachmentDescriptor > other ) const
	{
		return mAttachmentClass == other;
	};
};

USTRUCT( BlueprintType )
struct KPRIVATECODELIB_API FAttachmentDataStacker
{
	GENERATED_BODY()
	FAttachmentDataStacker()
	{
	};

public:
	UPROPERTY( SaveGame, BlueprintReadOnly )
	FTransform mMainSnapLocations = {};

	UPROPERTY( SaveGame, BlueprintReadOnly )
	TArray< AFGBuildable* > mSnappedActors = {};

	uint32 GetIndexFromActor( AFGBuildable* Actor ) const;
	bool CanSnapTo( int32 MaxModuleCount = 5 ) const;
	FTransform GetSnapLocation() const;
	float GetAllHeights() const;
	AFGBuildable* GetActorFromIndex( int32 Index ) const;
	void RemoveActorFromData( AFGBuildable* Actor );
	bool AddActorToData( AFGBuildable* Actor );
	TArray< AFGBuildable* > GetActorsUpperIndex( int32 Index );
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KPRIVATECODELIB_API UKPCLModularBuildingHandlerStacker : public UKPCLModularBuildingHandlerBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UKPCLModularBuildingHandlerStacker();

	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

	virtual void BeginPlay() override;
	virtual void InitArrays() override;

	virtual bool AddNewActorToAttachment( AFGBuildable* Actor, TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform Location, float Distance = 500.0f ) override;

	virtual void AttachedActorRemoved( AFGBuildable* Actor ) override;

	virtual bool CanAttachToLocation( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment, FTransform TestLocation, FTransform& OutLocation, float Distance = 500.0f ) const override;
	virtual bool GetSnapPointInRange( FTransform TestLocation, FTransform& SnapLocation, float AllowedDistance, TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) override;
	virtual AFGBuildable* GetAttachedActorByClass( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) override;
	virtual TArray< AFGBuildable* > GetAttachedActorsByClass( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) override;
	virtual void GetAttachedActors( TArray< AFGBuildable* >& Out ) override;

	virtual int FindAttachmentIndex( TSubclassOf< UKPCLModularAttachmentDescriptor > Attachment ) const override;

	UFUNCTION()
	void OnRep_AttachmentDatas();

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="Modular Handler", meta=(EditFixedSize) )
	TArray< FAttachmentInfosStacker > mAttachmentInformations = { FAttachmentInfosStacker(), FAttachmentInfosStacker(), FAttachmentInfosStacker(), FAttachmentInfosStacker(), FAttachmentInfosStacker(), FAttachmentInfosStacker(), FAttachmentInfosStacker(), FAttachmentInfosStacker(), FAttachmentInfosStacker(), FAttachmentInfosStacker() };

	UPROPERTY( SaveGame, BlueprintReadOnly, Replicated, ReplicatedUsing=OnRep_AttachmentDatas, meta=(EditFixedSize) )
	TArray< FAttachmentDataStacker > mAttachmentDatas = { FAttachmentDataStacker(), FAttachmentDataStacker(), FAttachmentDataStacker(), FAttachmentDataStacker(), FAttachmentDataStacker(), FAttachmentDataStacker(), FAttachmentDataStacker(), FAttachmentDataStacker(), FAttachmentDataStacker(), FAttachmentDataStacker() };
};
