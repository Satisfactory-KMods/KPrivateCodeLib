// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGCategory.h"
#include "FGRecipe.h"
#include "InstanceData.h"
#include "Resources/FGBuildingDescriptor.h"
#include "UObject/Object.h"
#include "KPCLDecorationActorData.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLDecorMainCategory: public UFGCategory {
	GENERATED_BODY()
};

UCLASS()
class KPRIVATECODELIB_API UKPCLDecorSubCategory: public UFGCategory {
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FKPCLInstanceDataOverwrite {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 InstanceIndex = 0;

	UPROPERTY(EditAnywhere)
	bool OverwriteMesh = false;

	UPROPERTY(EditAnywhere, meta=( EditCondition=OverwriteMesh, EditConditionHides ))
	UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere)
	bool OverwriteLocation = false;

	UPROPERTY(EditAnywhere, meta=( EditCondition=OverwriteLocation, EditConditionHides ))
	FTransform RelativeLocation;

	UPROPERTY(EditAnywhere)
	bool OverwriteMaterials = false;

	UPROPERTY(EditAnywhere, meta=( EditCondition=OverwriteMaterials, EditConditionHides ))
	TMap<UMaterialInterface* , UMaterialInterface*> MaterialOverwriteMap;

	UPROPERTY(EditAnywhere, meta=( EditCondition=OverwriteMaterials, EditConditionHides ))
	TMap<int32 , UMaterialInterface*> MaterialIndexOverwriteMap;
};

/**
 * 
 */
UCLASS(hidecategories=(Building, Scanning))
class KPRIVATECODELIB_API UKPCLDecorationActorData: public UFGBuildingDescriptor {
	GENERATED_BODY()

	public:
		UKPCLDecorationActorData();

		UFUNCTION(BlueprintPure)
		static TArray<FKPCLInstanceDataOverwrite> GetInstanceData(TSubclassOf<UKPCLDecorationActorData> Data);

	private:
		UPROPERTY(EditDefaultsOnly, Category="Decoration")
		TArray<FKPCLInstanceDataOverwrite> mInstanceData;
};

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLDecorationRecipe: public UFGRecipe {
	GENERATED_BODY()

	public:
		UKPCLDecorationRecipe() {
		}

		UFUNCTION(BlueprintPure)
		static TSubclassOf<UKPCLDecorationActorData> GetActorData(TSubclassOf<UKPCLDecorationRecipe> InClass);
};
