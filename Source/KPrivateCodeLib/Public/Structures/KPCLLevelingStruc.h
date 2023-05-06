#pragma once
#include "FGPlayerState.h"
#include "DamageTypes/FGDamageType.h"
#include "Logging.h"

#include "KPCLLevelingStruc.generated.h"

UENUM( BlueprintType )
enum class EMathForm : uint8
{
	Plus UMETA( DisplayName = "Plus" ),
	Minus UMETA( DisplayName = "Minus" ),
	Multi UMETA( DisplayName = "Multi" ),
	Div UMETA( DisplayName = "Div" )};

USTRUCT( BlueprintType )
struct FKPCLLevelInformation
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, SaveGame, BlueprintReadWrite )
	float mTotalExp = 0;

	UPROPERTY( EditAnywhere, SaveGame, BlueprintReadWrite )
	int mLevel = 1;
};

USTRUCT( BlueprintType )
struct FKPCLDamageType
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere )
	TArray< TSubclassOf< UFGDamageType > > mToxicResDamageClasses;

	bool Contains( const UFGDamageType* DamageType ) const
	{
		for( TSubclassOf< UFGDamageType > SubClass : mToxicResDamageClasses )
		{
			if( SubClass.GetDefaultObject() == DamageType )
			{
				return true;
			}
		}
		return false;
	}
};

USTRUCT( BlueprintType )
struct FKPCLSkillInformation
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	float mDefault = 0;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mPointCostPerLevel = 0;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	float mAddPerLevel = 0;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mMaxLevel = 99;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	EMathForm mMathForm = EMathForm::Plus;

	UPROPERTY( SaveGame, BlueprintReadWrite )
	int mSkilled = 0;

	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite )
	bool mIsSkillActive = true;

	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite )
	TArray< FInventoryStack > mSkillCostPerLevel;

	void GetSkilledPoints( int& Skilled ) const
	{
		Skilled += mPointCostPerLevel * FMath::Clamp( mSkilled, 0, mMaxLevel );
		//UE_LOG(LogKLIB_Components, Warning, TEXT("GetSkilledPoints > %d * %d += %d"), mPointCostPerLevel, mSkilled, mPointCostPerLevel * mSkilled);
	}

	int GetSkilledPoints() const
	{
		return mPointCostPerLevel * FMath::Clamp( mSkilled, 0, mMaxLevel );
	}

	void ReApplyDefaults( FKPCLSkillInformation DefaultStruc )
	{
		mDefault = DefaultStruc.mDefault;
		mAddPerLevel = DefaultStruc.mAddPerLevel;
		mMaxLevel = DefaultStruc.mMaxLevel;
		mPointCostPerLevel = DefaultStruc.mPointCostPerLevel;
		mMathForm = DefaultStruc.mMathForm;
		mIsSkillActive = DefaultStruc.mIsSkillActive;
		mSkillCostPerLevel = DefaultStruc.mSkillCostPerLevel;
	}

	bool IsValidToSkill( int Points ) const
	{
		return mPointCostPerLevel <= Points && !IsMaxValueReached() && mIsSkillActive;
	}

	float GetValue() const
	{
		if( mSkilled == 0 )
		{
			return mDefault;
		}
		if( mMathForm == EMathForm::Div )
		{
			return mDefault / mAddPerLevel * FMath::Clamp( mSkilled, 0, mMaxLevel );
		}
		if( mMathForm == EMathForm::Multi )
		{
			return mDefault * mAddPerLevel * FMath::Clamp( mSkilled, 0, mMaxLevel );
		}
		if( mMathForm == EMathForm::Minus )
		{
			return mDefault - mAddPerLevel * FMath::Clamp( mSkilled, 0, mMaxLevel );
		}
		return mDefault + mAddPerLevel * FMath::Clamp( mSkilled, 0, mMaxLevel );
	}

	bool IsMaxValueReached() const
	{
		return FMath::Clamp( mSkilled, 0, mMaxLevel ) >= mMaxLevel;
	}

	int DoSkill()
	{
		mSkilled++;
		mSkilled = FMath::Clamp( mSkilled, 0, mMaxLevel );
		return mPointCostPerLevel;
	}

	int GetValueAsInt()
	{
		return GetValue();
	}

	void DoReskill()
	{
		mSkilled = 0;
	}
};

USTRUCT( BlueprintType )
struct FKPCLOnceSkillInformation
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mPointCost = 50;

	UPROPERTY( SaveGame, BlueprintReadWrite )
	bool mSkilled = false;

	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite )
	bool mIsSkillActive = true;

	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite )
	TArray< FInventoryStack > mSkillCostPerLevel;

	void ReApplyDefaults( FKPCLOnceSkillInformation DefaultStruc )
	{
		mPointCost = DefaultStruc.mPointCost;
		mIsSkillActive = DefaultStruc.mIsSkillActive;
		mSkillCostPerLevel = DefaultStruc.mSkillCostPerLevel;
	}

	bool IsValidToSkill( int Points ) const
	{
		return mPointCost <= Points && !mSkilled && mIsSkillActive;
	}

	void GetSkilledPoints( int& Skilled ) const
	{
		if( mSkilled )
		{
			Skilled += mPointCost;
		}
	}

	int GetSkilledPoints() const
	{
		return mPointCost;
	}

	bool IsSkilled() const
	{
		return mSkilled;
	}

	int DoSkill()
	{
		mSkilled = true;
		return mPointCost;
	}

	void DoReskill()
	{
		mSkilled = false;
	}
};

USTRUCT( BlueprintType )
struct FKPCLLevelStepInformation
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	float mExp = 200;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mSkillPoints = 5;

	UPROPERTY( BlueprintReadWrite )
	bool mIsMaxLevel = true;

	UPROPERTY( BlueprintReadWrite )
	bool mIsValid = false;
};

USTRUCT( BlueprintType )
struct FKPCLLevelRangeInformation
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mMinLevel = 1;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mMaxLevel = 100;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mNormalSkillPoints = 5;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mSkillPointsOnHit10 = 5;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mSkillPointsOnHitMax = 5;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	UCurveFloat* mLevelCurve;

	float GetStartExp() const;
	float GetCurrentNeededExp( int Level ) const;
	bool IsValidToLevelUp( int Level, float TotalExp ) const;
	FKPCLLevelStepInformation GetInformationAboutLevelStep( int Level ) const;
	int GetSkillPointsByLevel( int Level ) const;
	int GetFixedSkillPointsByLevel( int Level ) const;

	bool IsInRange( int Level ) const
	{
		return Level <= mMaxLevel && Level >= mMinLevel;
	}

	bool IsMaxLevel( int Level ) const
	{
		return Level >= mMaxLevel;
	}
};


UCLASS()
class KPRIVATECODELIB_API UKPCLLevelingStrucFunctionLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION( BlueprintCallable, Category="KMods|LevelingStruc" )
	static float LevelRange_GetCurrentNeededExp( FKPCLLevelRangeInformation Range, int Level );

	UFUNCTION( BlueprintCallable, Category="KMods|LevelingStruc" )
	static TArray< FInventoryStack > GetSkillCosts( FKPCLSkillInformation Information );

	UFUNCTION( BlueprintCallable, Category="KMods|LevelingStruc" )
	static TArray< FInventoryStack > GetReskillCosts( FKPCLSkillInformation Information, int32 Divisor = 4 );

	UFUNCTION( BlueprintCallable, Category="KMods|LevelingStruc" )
	static bool LevelRange_IsValidToLevelUp( FKPCLLevelRangeInformation Range, int Level, float TotalExp );

	UFUNCTION( BlueprintCallable, Category="KMods|LevelingStruc" )
	static FKPCLLevelStepInformation LevelRange_GetInformationAboutLevelStep( FKPCLLevelRangeInformation Range, int Level );

	UFUNCTION( BlueprintCallable, Category="KMods|LevelingStruc" )
	static int LevelRange_GetSkillPointsByLevel( FKPCLLevelRangeInformation Range, int Level );

	UFUNCTION( BlueprintCallable, Category= "KMods|LevelingStruc" )
	static int LevelRange_IsMaxLevel( FKPCLLevelRangeInformation Range, int Level );

	UFUNCTION( BlueprintCallable, Category= "KMods|LevelingStruc" )
	static int LevelRange_IsnInRange( FKPCLLevelRangeInformation Range, int Level );
};


UENUM( BlueprintType )
enum class ELevelCheat : uint8
{
	Exp UMETA( DisplayName = "Exp" ),
	Level UMETA( DisplayName = "Level" )};

USTRUCT( BlueprintType )
struct FKPCLCheating
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	ELevelCheat mType = ELevelCheat::Exp;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int mLevel = -1;

	UPROPERTY( BlueprintReadWrite )
	int mExp = -1;
};
