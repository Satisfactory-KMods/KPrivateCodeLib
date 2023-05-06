// Fill out your copyright notice in the Description page of Project Settings.


#include "Description/KPCLDebuff.h"

#include "FGCharacterMovementComponent.h"
#include "BFL/KBFL_Player.h"
#include "Subsystem/KPCLLevelingSubsystem.h"

bool UKPCLDebuff_Base::IsSupportedForNetworking() const
{
	return true;
}

void UKPCLDebuff_Base::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	UObject::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UKPCLDebuff_Base, mActive );
}

float UKPCLDebuff_Base::ValidateEXP( float Exp, bool Auth )
{
	return Exp;
}

float UKPCLDebuff_Base::GetDebuffValue_Implementation() const
{
	return 0.0f;
}

bool UKPCLDebuff_Base::IsActive_Implementation() const
{
	return mActive;
}


void UKPCLDebuffExp::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	UObject::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UKPCLDebuffExp, mDebuffValue );
}

float UKPCLDebuffExp::OnExpAdded( float Exp, bool Auth )
{
	UE_LOG( LogKLIB, Log, TEXT("%f <= %f, %d"), Exp, GetDebuffValue(), Exp <= GetDebuffValue() );
	if( Exp <= GetDebuffValue() )
	{
		if( Auth )
		{
			mDebuffValue -= Exp;
		}
		return 0.0f;
	}

	float Return = Exp - mDebuffValue;
	if( Auth )
	{
		mDebuffValue = 0.0f;
	}
	return Return;
}

float UKPCLDebuffExp::OnExpLost( float Exp )
{
	mDebuffValue += Exp;
	return mDebuffValue;
}

float UKPCLDebuffExp::ValidateEXP( float Exp, bool Auth )
{
	if( Exp >= 0.0f )
	{
		return OnExpAdded( Exp, Auth );
	}
	return OnExpLost( Exp );
}

void UKPCLBuffExp::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const
{
	UObject::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UKPCLBuffExp, mExpBuffValue );
}

float UKPCLBuffExp::ValidateEXP( float Exp, bool Auth )
{
	return Super::ValidateEXP( Exp, Auth ) * mExpBuffValue;
}
