// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KPCLModularAttachmentDescriptor.generated.h"

/**
 * This class is used to define the attachment points for modular buildings
 */
UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API UKPCLModularAttachmentDescriptor : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ModularAttachmentDescriptor|Hologram")
	static TArray<TSubclassOf<UKPCLModularAttachmentDescriptor>> GetAttachmentDependencies(TSubclassOf<UKPCLModularAttachmentDescriptor> InClass);

	UFUNCTION(BlueprintPure, Category = "ModularAttachmentDescriptor|Hologram")
	static FText GetAttachmentName(TSubclassOf<UKPCLModularAttachmentDescriptor> InClass);

protected:
	/**
	 * Name of the attachment
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Descriptor")
	FText mName;

	/**
	 * Dependencies for this attachment for example a Boiler need first a Heater to be attached
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Hologram")
	TArray<TSubclassOf<UKPCLModularAttachmentDescriptor>> mDependencies;
};