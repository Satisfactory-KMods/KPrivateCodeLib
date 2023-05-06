// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Description/Decor/KPCLDecorationActorData.h"
#include "UObject/Interface.h"
#include "KPCLDecorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE( Blueprintable )
class KPRIVATECODELIB_API UKPCLDecorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class KPRIVATECODELIB_API IKPCLDecorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION( BlueprintNativeEvent, Category="KMods|Decoration" )
	void StartLookingForDecoration( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player );
	
	UFUNCTION( BlueprintNativeEvent, Category="KMods|Decoration" )
	void EndLookingForDecoration( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player );
	
	UFUNCTION( BlueprintNativeEvent, Category="KMods|Decoration" )
	bool IsDecorationDataAllowed( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player );
	
	UFUNCTION( BlueprintNativeEvent, Category="KMods|Decoration" )
	void ApplyDecorationData( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player );
	
	UFUNCTION( BlueprintNativeEvent, Category="KMods|Decoration" )
	bool SetDecorationData( TSubclassOf< UKPCLDecorationRecipe > DecorationData, AFGCharacterPlayer* Player );
};
