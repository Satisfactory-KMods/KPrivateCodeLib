// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Resources/FGItemDescriptor.h"
#include "KPCLNetworkDrive.generated.h"

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLNetworkDrive : public UFGItemDescriptor
{
	GENERATED_BODY()

public:
	UFUNCTION( BlueprintPure, Category="Network" )
	static int32 GetFicsitBytes( TSubclassOf< UKPCLNetworkDrive > InClass );

	UFUNCTION( BlueprintPure, Category="Network" )
	static bool GetIsFluidDrive( TSubclassOf< UKPCLNetworkDrive > InClass );

	UFUNCTION( BlueprintPure, Category="Network" )
	static int32 GetDriveTier( TSubclassOf< UKPCLNetworkDrive > InClass );

	UFUNCTION( BlueprintPure, Category="Network" )
	static float GetPowerConsume( TSubclassOf< UKPCLNetworkDrive > InClass );

protected:
	virtual FText GetItemDescriptionInternal() const override;
	virtual FText GetItemNameInternal() const override;

	UFUNCTION( BlueprintNativeEvent )
	FText GetItemDescriptionInternal_BP() const;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="Network" )
	int32 mFicsitBytes = 1;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="Network" )
	int32 mDriveTier = 1;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="Network" )
	float mPowerConsume = 2.f;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="Network" )
	bool mFluidDrive = false;
};
