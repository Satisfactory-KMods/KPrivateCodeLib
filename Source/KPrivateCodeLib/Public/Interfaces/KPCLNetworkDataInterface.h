// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Resources/FGItemDescriptor.h"
#include "Subsystem/KPCLFaxitSubsystem.h"
#include "UObject/Interface.h"
#include "KPCLNetworkDataInterface.generated.h"

USTRUCT(BlueprintType)
struct FNetworkUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanTransferItems = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EResourceForm mDefaultFilter = EResourceForm::RF_INVALID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanSwitchForm = true;
};

// This class does not need to be modified.
UINTERFACE()
class UKPCLNetworkDataInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class KPRIVATECODELIB_API IKPCLNetworkDataInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
	class AKPCLNetworkCore* GetCore() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
	bool HasCore() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
	bool HasCoreInNetwork() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
	class UKPCLNetwork* GetNetwork() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
	FNetworkUIData GetUIDData() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="KPCLCustomDataInterface")
	FKPCLFaxitNetwork GetNetworkData() const;
};