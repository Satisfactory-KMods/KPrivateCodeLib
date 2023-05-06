// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/Buildings/KPCLNetworkTerminal.h"

#include "Network/Buildings/KPCLNetworkCore.h"

AKPCLNetworkTerminal::AKPCLNetworkTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AKPCLNetworkTerminal::IsUseable_Implementation() const
{
	return true;
}

void AKPCLNetworkTerminal::OnUse_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state )
{
	if( IsProducing() )
	{
		Execute_OnUse( Execute_GetCore( this ), byCharacter, state );
	}
}

FText AKPCLNetworkTerminal::GetLookAtDecription_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state ) const
{
	if( IsProducing() )
	{
		return Super::GetLookAtDecription_Implementation( byCharacter, state );
	}
	return mNoCoreText;
}
