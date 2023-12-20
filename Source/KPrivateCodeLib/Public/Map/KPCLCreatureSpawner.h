﻿// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Creature/FGCreatureSpawner.h"
#include "GameFramework/Actor.h"
#include "Resources/FGResourceNodeBase.h"
#include "KPCLCreatureSpawner.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLCreatureSpawner: public AFGCreatureSpawner {
	GENERATED_BODY()

	public:
		AKPCLCreatureSpawner();

	protected:
		virtual void BeginPlay() override;
		virtual void Tick(float DeltaSeconds) override;

	public:
		UFUNCTION(BlueprintCallable)
		void CheckNodesInRange();

		UFUNCTION(BlueprintCallable)
		void ReApplyChangesOnCreatures();

		UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
		void ReApplyChangesOnCreature(AFGCreature* Creature);

	private:
		UPROPERTY(EditAnywhere, Category="KMods|NodeBoundedSpawner")
		bool mShouldCheckNodesInRange = false;

		UPROPERTY(EditAnywhere, Category="KMods|NodeBoundedSpawner", meta=(EditCondition=mShouldCheckNodesInRange))
		TArray<AFGResourceNodeBase*> mBoundedNodes;

		UPROPERTY(EditAnywhere, Category="KMods|CreatureOverwrite")
		TMap<int32 , UMaterialInterface*> mMaterialOverwrite;

		UPROPERTY(EditAnywhere, Category="KMods|CreatureOverwrite")
		float mHealthOverwrite;
};
