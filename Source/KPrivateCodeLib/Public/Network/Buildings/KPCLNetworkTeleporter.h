// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "Structures/KPCLNetworkStructures.h"
#include "KPCLNetworkTeleporter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeleporterDataUpdated, FTeleporterInformation, Data);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerTeleported, AFGCharacterPlayer*, Player);

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkTeleporter: public AKPCLNetworkBuildingBase {
	GENERATED_BODY()

	public:
		AKPCLNetworkTeleporter();

		// START: IFGUseableInterface
		virtual void  OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;
		virtual FText GetLookAtDecription_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) const override;
		// END: IFGUseableInterface

		// START: AActor
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		// END: AActor

		UFUNCTION(BlueprintCallable)
		void TeleportPlayer(AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter);

		UFUNCTION(NetMulticast, Reliable)
		void Multicast_TeleportPlayer(AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter);

		UFUNCTION(BlueprintCallable)
		void SetTeleporterData(FTeleporterInformation NewData);

		UFUNCTION(BlueprintPure)
		FTeleporterInformation GetTeleporterData();

		UFUNCTION(BlueprintCallable)
		bool CanTeleportToTeleporter(AFGCharacterPlayer* Character, AKPCLNetworkTeleporter* OtherTeleporter);

		UFUNCTION(BlueprintCallable)
		void GetCostToOtherTeleporter(AKPCLNetworkTeleporter* OtherTeleporter, TArray<FItemAmount>& OutCosts);

		UFUNCTION(BlueprintCallable)
		void GetAllOtherTeleporter(TArray<AKPCLNetworkTeleporter*>& OtherTeleporter);

		UFUNCTION(BlueprintPure)
		FVector GetTeleportLocation() const;

		UPROPERTY(BlueprintAssignable)
		FOnTeleporterDataUpdated mOnTeleporterDataUpdated;

		UPROPERTY(BlueprintAssignable)
		FOnPlayerTeleported mOnPlayerTeleported;

	protected:
		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UBoxComponent* mTriggerBox;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		USphereComponent* mPlayerSphere;

	private:
		UFUNCTION()
		void OnRep_TeleporterName();

		UPROPERTY(EditAnywhere, SaveGame, Replicated, ReplicatedUsing=OnRep_TeleporterName, Category="KMods|Teleporter")
		FTeleporterInformation mTeleporterData;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Teleporter")
		FText mToFarAwayText;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Teleporter")
		TArray<FItemAmount> mCosts;

		UPROPERTY(EditDefaultsOnly, Category="KMods|Teleporter")
		float mMultiplierPerRange = 1000.f;
};
