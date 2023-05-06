// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Description/Decor/KPCLDecorationActorData.h"

UKPCLDecorationActorData::UKPCLDecorationActorData()
{
	mCategory = UKPCLDecorMainCategory::StaticClass();
	mSubCategories.AddUnique( UKPCLDecorSubCategory::StaticClass() );
}

TArray< FKPCLInstanceDataOverwrite > UKPCLDecorationActorData::GetInstanceData( TSubclassOf<UKPCLDecorationActorData> Data )
{
	if( IsValid( Data ) )
	{
		return Data.GetDefaultObject()->mInstanceData;
	}
	return TArray< FKPCLInstanceDataOverwrite >();
}

TSubclassOf<UKPCLDecorationActorData> UKPCLDecorationRecipe::GetActorData( TSubclassOf<UKPCLDecorationRecipe> InClass )
{
	if( IsValid( InClass ) )
	{
		TArray< FItemAmount > Amounts = GetProducts( InClass );
		if( Amounts.IsValidIndex( 0 ) && IsValid( Amounts[ 0 ].ItemClass ) )
		{
			return TSubclassOf< UKPCLDecorationActorData >{ Amounts[ 0 ].ItemClass };
		}
	}
	return UKPCLDecorationActorData::StaticClass();
}
