// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Description/KPCLNetworkDrive.h"

int32 UKPCLNetworkDrive::GetMultiplier(TSubclassOf<UKPCLNetworkDrive> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mFaxitStorageMultiplier;
	}
	return 0;
}

float UKPCLNetworkDrive::GetPowerConsume(TSubclassOf<UKPCLNetworkDrive> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mPowerConsume;
	}
	return 0.f;
}

FText UKPCLNetworkDrive::GetItemDescriptionInternal() const
{
	FText MainTxt = GetItemDescriptionInternal_BP();

	FFormatNamedArguments FormatPatternArgs;
	FormatPatternArgs.Empty();
	FormatPatternArgs.Add(TEXT("Multiplier"), FText::FromString(FString::FromInt(mFaxitStorageMultiplier)));
	return FText::Format(MainTxt, FormatPatternArgs);
}

FText UKPCLNetworkDrive::GetItemNameInternal() const
{
	FText MainTxt = GetItemNameInternal_BP();

	FFormatNamedArguments FormatPatternArgs;
	FormatPatternArgs.Empty();
	FormatPatternArgs.Add(TEXT("Multiplier"), FText::FromString(FString::FromInt(mFaxitStorageMultiplier)));
	return FText::Format(MainTxt, FormatPatternArgs);
}

FText UKPCLNetworkDrive::GetItemNameInternal_BP_Implementation() const
{
	return mDisplayName;
}

FText UKPCLNetworkDrive::GetItemDescriptionInternal_BP_Implementation() const
{
	return mDescription;
}