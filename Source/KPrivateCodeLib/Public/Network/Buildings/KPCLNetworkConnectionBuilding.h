#pragma once

#include "CoreMinimal.h"
#include "FGResourceSinkSubsystem.h"
#include "KPCLNetworkCore.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "Resources/FGNoneDescriptor.h"
#include "KPCLNetworkConnectionBuilding.generated.h"


UENUM(BlueprintType)
enum class EKPCLOverflowMode : uint8
{
	Ignore,
	Sink,
	Depot,
	DepotAndSink
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkConnectionBuilding : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkConnectionBuilding();

	// START: AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// END: AActor
	
private:
	friend class AKPCLNetworkSink;

	UPROPERTY(SaveGame)
	UFGInventoryComponent* mInventory;

	UPROPERTY( SaveGame, meta = ( FGReplicated ) )
	EKPCLOverflowMode mOverflowMode = EKPCLOverflowMode::Ignore;
};
