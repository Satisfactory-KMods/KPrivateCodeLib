// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "KBFLWorldModule.h"
#include "KPCLWorldModule.generated.h"


/**
 * This is a full native version for UKBFLWorldModule
 */
UCLASS(Blueprintable)
class KPRIVATECODELIB_API UKPCLWorldModule: public UKBFLWorldModule {
	GENERATED_BODY()

	public:
		UKPCLWorldModule();

		// Start: UKBFLWorldModule
		virtual void PostInitPhase_Implementation() override;
		virtual void InitPhase_Implementation() override;
		// End: UKBFLWorldModule

		/** Bind Events for send messages on unlock etc. */
		void BindUnlockEvents();

		/** Send a Message to all Player */
		void SendMessage(const TSubclassOf<UFGMessageBase> Message) const;

		UFUNCTION()
		void OnSchematicUnlocked(TSubclassOf<UFGSchematic> Schematic);

		UFUNCTION()
		void OnResearchTreeAccessUnlocked(TSubclassOf<UFGResearchTree> ResearchTree);

		UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Options")
		bool IsADAEnabled();

		UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Options")
		bool IsEasyNodesEnabled();

		UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Options")
		void ApplyEasyNodes(const TArray<TSubclassOf<UFGResearchTree>>& Nodes);


		UPROPERTY(EditDefaultsOnly, Category="KMods|ADA")
		TMap<TSubclassOf<UFGSchematic> , TSubclassOf<UFGMessageBase>> mUnlockSchematic;

		UPROPERTY(EditDefaultsOnly, Category="KMods|ADA")
		TMap<TSubclassOf<UFGResearchTree> , TSubclassOf<UFGMessageBase>> mUnlockAccessResearchTree;

		UPROPERTY(EditDefaultsOnly, Category="KMods|ADA")
		bool mUseEasyNodes = true;
};
