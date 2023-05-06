// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGColoredInstanceMeshProxy.h"
#include "Buildables/FGBuildableFactoryBuilding.h"
#include "Interfaces/KPCLDecorInterface.h"
#include "KPCLBuildableDecorActor.generated.h"

UCLASS( )
class KPRIVATECODELIB_API AKPCLBuildableDecorActor : public AFGBuildableFactoryBuilding, public IKPCLDecorInterface
{
	GENERATED_BODY()
	
public:
	AKPCLBuildableDecorActor( const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get() );
	
protected:
	// Start IKPCLDecorInterface
	virtual void StartLookingForDecoration_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player ) override;
	virtual void EndLookingForDecoration_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player ) override;
	virtual bool IsDecorationDataAllowed_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player ) override;
	virtual void ApplyDecorationData_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player ) override;
	virtual bool SetDecorationData_Implementation( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player ) override;
	// End IKPCLDecorInterface

	// Start AActor
	virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;
	virtual void BeginPlay() override;
	// End AActor
	
	// Start AFGBuildableFactoryBuildingLightweight
	virtual void ReApplyColorForIndex( int32 Idx, const FFactoryCustomizationData& customizationData );
	virtual void ApplyCustomizationData_Native(const FFactoryCustomizationData& customizationData) override;
	virtual void SetCustomizationData_Native(const FFactoryCustomizationData& customizationData) override;
	virtual void OnBuildEffectFinished() override;
	virtual void OnBuildEffectActorFinished() override;
	// End AFGBuildableFactoryBuildingLightweight

public:
	UFUNCTION( BlueprintCallable, Category="KMods" )
	bool AIO_OverwriteInstanceData( UStaticMesh* Mesh, int32 Idx );
	
	UFUNCTION( BlueprintCallable, Category="KMods" )
	bool AIO_OverwriteInstanceData_Transform( UStaticMesh* Mesh, FTransform NewRelativTransform, int32 Idx );
	
	UFUNCTION( BlueprintCallable, Category="KMods" )
	bool AIO_UpdateCustomFloat( int32 FloatIndex, float Data, int32 InstanceIdx, bool MarkDirty = true );
	
	UFUNCTION( BlueprintCallable, Category="KMods" )
	bool AIO_UpdateCustomFloatAsColor( int32 StartFloatIndex, FLinearColor Data, int32 InstanceIdx, bool MarkDirty = true );
	
	UFUNCTION( BlueprintCallable, Category="KMods" )
	bool AIO_SetInstanceHidden( int32 InstanceIdx, bool IsHidden );
	
	UFUNCTION( BlueprintCallable, Category="KMods" )
	bool AIO_SetInstanceWorldTransform( int32 InstanceIdx, FTransform Transform );
	
	UFUNCTION( BlueprintPure, Category="KMods" )
	TSubclassOf< UKPCLDecorationActorData > GetDecorationData() const;
	
	UFUNCTION( BlueprintCallable, Category="KMods" )
	void SetNewActorData( TSubclassOf< UKPCLDecorationRecipe > Data, AFGCharacterPlayer* Player );

protected:
	TMap< int32, TMap< int32, float > > mCachedCustomData;
	TMap< int32, FTransform > mCachedTransforms;
	// END: Advanced Instance Overwrite

protected:
	UFUNCTION()
	virtual void OnRep_DecorationData();
	
	UPROPERTY( EditDefaultsOnly, Category="KMods|Decor" )
	TSubclassOf< UKPCLDecorationActorData > mDefaultDecorationData;
	
	UPROPERTY( EditDefaultsOnly, Category="KMods|Decor" )
	TArray< TSubclassOf< UKPCLDecorationActorData > > mAllowedDecorationData;
	
	UPROPERTY( SaveGame, ReplicatedUsing=OnRep_DecorationData )
	TSubclassOf< UKPCLDecorationActorData > mDecorationData;
	
	UPROPERTY( ReplicatedUsing=OnRep_DecorationData )
	TSubclassOf< UKPCLDecorationActorData > mTempDecorationData;
};


UCLASS()
class KPRIVATECODELIB_API AKPCLBuildableDecorActorLight : public AKPCLBuildableDecorActor
{
	GENERATED_BODY()
	
public:
	AKPCLBuildableDecorActorLight( const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get() );
};