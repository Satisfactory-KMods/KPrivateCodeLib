#include "Structures/KPCLLevelingStruc.h"

float FKPCLLevelRangeInformation::GetStartExp() const
{
	return FMath::RoundUpToClosestMultiple( mLevelCurve->GetFloatValue( 0 ), 10 );
}

float FKPCLLevelRangeInformation::GetCurrentNeededExp( int Level ) const
{
	if( Level == mMaxLevel || !mLevelCurve )
	{
		return 4294967296;
	}

	if( Level < mMinLevel )
	{
		return 0.0f;
	}

	if( Level == mMinLevel )
	{
		return GetStartExp();
	}

	return FMath::RoundUpToClosestMultiple( mLevelCurve->GetFloatValue( ( Level + 1 ) * 10000 ), 10 );
}

bool FKPCLLevelRangeInformation::IsValidToLevelUp( int Level, float TotalExp ) const
{
	if( !mLevelCurve )
	{
		return false;
	}
	if( Level == mMaxLevel )
	{
		return false;
	}
	return GetCurrentNeededExp( Level ) <= TotalExp;
}

FKPCLLevelStepInformation FKPCLLevelRangeInformation::GetInformationAboutLevelStep( int Level ) const
{
	FKPCLLevelStepInformation Step;
	if( Level == mMaxLevel )
	{
		Step.mSkillPoints = mSkillPointsOnHitMax;
	}
	else if( 0 == Level % 10 )
	{
		Step.mSkillPoints = mSkillPointsOnHit10;
	}
	else
	{
		Step.mSkillPoints = mNormalSkillPoints;
	}
	Step.mIsMaxLevel = Level == mMaxLevel;
	Step.mExp = GetCurrentNeededExp( Level );
	return Step;
}

int FKPCLLevelRangeInformation::GetSkillPointsByLevel( int Level ) const
{
	if( mMinLevel == Level )
	{
		return 0;
	}

	int SkillPoints = 0;
	for( int i = 1; i < Level; ++i )
	{
		SkillPoints += GetFixedSkillPointsByLevel( i );
	}

	return SkillPoints;
}

int FKPCLLevelRangeInformation::GetFixedSkillPointsByLevel( int Level ) const
{
	if( Level == mMaxLevel )
	{
		return mSkillPointsOnHitMax;
	}
	if( 0 == Level % 10 )
	{
		return mSkillPointsOnHit10;
	}
	return mNormalSkillPoints;
}

float UKPCLLevelingStrucFunctionLib::LevelRange_GetCurrentNeededExp( FKPCLLevelRangeInformation Range, int Level )
{
	return Range.GetCurrentNeededExp( Level );
}

TArray< FInventoryStack > UKPCLLevelingStrucFunctionLib::GetSkillCosts( FKPCLSkillInformation Information )
{
	return Information.mSkillCostPerLevel;
}

TArray< FInventoryStack > UKPCLLevelingStrucFunctionLib::GetReskillCosts( FKPCLSkillInformation Information, int32 Divisor )
{
	TArray< FInventoryStack > Out = Information.mSkillCostPerLevel;

	for( FInventoryStack& InventoryStack : Out )
	{
		if( UFGItemDescriptor::GetStackSize( InventoryStack.Item.GetItemClass() ) == 0 )
		{
			InventoryStack.NumItems = FMath::Clamp< int32 >( FMath::DivideAndRoundUp< int32 >( InventoryStack.NumItems * Information.mSkilled, Divisor ), 1, 4 );
			continue;
		}
		InventoryStack.NumItems = FMath::DivideAndRoundUp< int32 >( InventoryStack.NumItems * Information.mSkilled, Divisor );
	}

	return Out;
}

bool UKPCLLevelingStrucFunctionLib::LevelRange_IsValidToLevelUp( FKPCLLevelRangeInformation Range, int Level, float TotalExp )
{
	return Range.IsValidToLevelUp( Level, TotalExp );
}

FKPCLLevelStepInformation UKPCLLevelingStrucFunctionLib::LevelRange_GetInformationAboutLevelStep( FKPCLLevelRangeInformation Range, int Level )
{
	return Range.GetInformationAboutLevelStep( Level );
}

int UKPCLLevelingStrucFunctionLib::LevelRange_GetSkillPointsByLevel( FKPCLLevelRangeInformation Range, int Level )
{
	return Range.GetSkillPointsByLevel( Level );
}

int UKPCLLevelingStrucFunctionLib::LevelRange_IsMaxLevel( FKPCLLevelRangeInformation Range, int Level )
{
	return Range.IsMaxLevel( Level );
}

int UKPCLLevelingStrucFunctionLib::LevelRange_IsnInRange( FKPCLLevelRangeInformation Range, int Level )
{
	return Range.IsInRange( Level );
}
