// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGInventoryComponent.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "UObject/Object.h"
#include "KPCLBlueprintFunctionLib.generated.h"

/**
 * 
 */
UCLASS( )
class KPRIVATECODELIB_API UKPCLBlueprintFunctionLib : public UBlueprintFunctionLibrary {
	GENERATED_BODY( )

	public:
		// Cpp
		static void SetAllowOnIndex_ThreadSafe( UFGInventoryComponent* Component, int32 Index, TSubclassOf< UFGItemDescriptor > ItemClass );

		//BP
		UFUNCTION( BlueprintCallable, Category="KMods|BPFL", meta = (DeterminesOutputType = "InClass") )
		static UObject* GetDefaultSilent( TSubclassOf< UObject > InClass );

		static void ResolveHitResult( UObject* Context, const FHitResult& InHitResult, FHitResult& OutHitResult );

		static void ResolveOverlapResult( UObject* Context, const FOverlapResult& InOverlapResult, FOverlapResult& OutOverlapResult );
};
