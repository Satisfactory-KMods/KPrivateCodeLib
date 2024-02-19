// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGFactoryConnectionComponent.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "KPCLNetworkBuildingAttachment.generated.h"

USTRUCT(Blueprintable)
struct FConnectionPair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "KMods|Network")
	UFGConnectionComponent* mBuilding;

	UPROPERTY(EditAnywhere, Category = "KMods|Network")
	UFGConnectionComponent* mParent;
};

USTRUCT(Blueprintable)
struct FNetworkPowerOffset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "KMods|Network")
	TSubclassOf<AFGBuildable> mBuildingClass;

	UPROPERTY(EditAnywhere, Category = "KMods|Network")
	FTransform mOffset;
};

USTRUCT(Blueprintable)
struct FNetworkAttachmentRules
{
	GENERATED_BODY()

	FNetworkAttachmentRules()
	{
	}

	FNetworkAttachmentRules(UFGConnectionComponent* Connection)
	{
		mConnection = Connection;
	}

	UPROPERTY(SaveGame, EditAnywhere, Category = "KMods|Network")
	UFGConnectionComponent* mConnection;

	UPROPERTY(SaveGame, EditAnywhere, Category = "KMods|Network")
	TSubclassOf<UFGItemDescriptor> mItem;

	UPROPERTY(SaveGame, EditAnywhere, Category = "KMods|Network")
	int32 mMaxAmount;

	bool operator== (const FNetworkAttachmentRules& Other) const
	{
		return Other.mConnection == mConnection;
	}
};

USTRUCT()
struct FAbstractAttachmentMeshes
{
	GENERATED_BODY()

	FAbstractAttachmentMeshes(): mTargetClass(nullptr), mMesh(nullptr)
	{
	}

	FAbstractAttachmentMeshes(UClass* Connection): mMesh(nullptr)
	{
		mTargetClass = Connection;
	}

	UPROPERTY(EditAnywhere, Category = "KMods|Network")
	UClass* mTargetClass;

	UPROPERTY(EditAnywhere, Category = "KMods|Network")
	EFactoryConnectionDirection mBeltDirection = EFactoryConnectionDirection::FCD_ANY;

	UPROPERTY(EditAnywhere, Category = "KMods|Network")
	EPipeConnectionType EPipeConnectionType = EPipeConnectionType::PCT_ANY;

	UPROPERTY(EditAnywhere, Category = "KMods|Network")
	UStaticMesh* mMesh;

	bool operator== (const UFGConnectionComponent*& Other) const
	{
		if(!IsValid(mTargetClass)) return false;
		if(!IsValid(Other)) return false;
		return Other->GetClass()->IsChildOf(mTargetClass);
	}
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkBuildingAttachment : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AKPCLNetworkBuildingAttachment();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Factory_Tick(float dt) override;

private:
	void CacheDataForAttachment();
	void InitConnections();

public:
	/**
	 * Set the building this attachment is attached to
	 */
	void AttachTo(AFGBuildable* Building, bool ConnectFromHologram = false);

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	AFGBuildable* GetAttachedBuilding() const { return mAttachedBuilding; }

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	bool IsAttached() const { return IsValid(mAttachedBuilding); }

	bool GetRequiredItems(TArray<FItemAmount>& Items) ;
	bool PushToNetwork(TArray<FItemAmount>& ToPush, float MaxSolidBytes, float MaxFluidBytes);
	bool GetFromNetwork(const TArray<FItemAmount>& ToReceive);

	UFUNCTION(BlueprintPure	, Category = "KMods|Network")
	TArray<FNetworkAttachmentRules> GetAttachmentRules() const { return mNetworkAttachmentRules; }

	UFUNCTION(BlueprintCallable	, Category = "KMods|Network")
	void RemoveAttachmentRule(UFGConnectionComponent* Connection);

	UFUNCTION(BlueprintCallable	, Category = "KMods|Network")
	void SetOrOverwriteRule(FNetworkAttachmentRules Rule);


	UPROPERTY(EditDefaultsOnly, Category = "KMods|Network")
	TArray<FAbstractAttachmentMeshes> mAbstractMeshAttachments;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Network")
	TArray<FNetworkPowerOffset> mPowerConnectionOffsets;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Network")
	UKPCLNetworkConnectionComponent* mNetworkConnectionComponent;

	UFGPowerConnectionComponent* GetParentPowerConnection();

	/**
	 *Replication
	 */
public:
	UFUNCTION()
	void OnAttachmentUpdated();
	void UpdatePowerConnectionPosition();

private:
	UPROPERTY(SaveGame, ReplicatedUsing=OnAttachmentUpdated)
	AFGBuildable* mAttachedBuilding;

	UFUNCTION()
	TArray<UFGConnectionComponent*> GetAllAttachmentConnections() const;

	/**
	 * A list of rules for what can be attached to this building
	 * For example
	 * If the player wants to store items in a storage container, the storage container will have a rule that says hey I want to store X amount of Y item
	 * this rules depends on the selected connection component and will not overwrite if the attached building has rules on the connection!
	 */
	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Network", replicated)
	TArray<FNetworkAttachmentRules> mNetworkAttachmentRules;

	UPROPERTY()
	TArray<UFGFactoryConnectionComponent*> mCachedBeltConnectionsInput;

	UPROPERTY()
	TArray<UFGFactoryConnectionComponent*> mCachedBeltConnectionsOutput;

	UPROPERTY()
	UFGPowerConnectionComponent* mCachedPowerConnection;

	UPROPERTY()
	TArray<UFGPipeConnectionComponent*> mCachedPipeConnectionsInput;

	UPROPERTY()
	TArray<UFGPipeConnectionComponent*> mCachedPipeConnectionsOutput;

	UPROPERTY(SaveGame, Replicated)
	TArray<FConnectionPair> mConnections;

};
