// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Description/KPCLNetworkDrive.h"

int32 UKPCLNetworkDrive::GetFicsitBytes(TSubclassOf<UKPCLNetworkDrive> InClass) {
	if(IsValid(InClass)) {
		return InClass.GetDefaultObject()->mFicsitBytes;
	}
	return 0;
}

bool UKPCLNetworkDrive::GetIsFluidDrive(TSubclassOf<UKPCLNetworkDrive> InClass) {
	if(IsValid(InClass)) {
		return InClass.GetDefaultObject()->mFluidDrive;
	}
	return false;
}

int32 UKPCLNetworkDrive::GetDriveTier(TSubclassOf<UKPCLNetworkDrive> InClass) {
	if(IsValid(InClass)) {
		return InClass.GetDefaultObject()->mDriveTier;
	}
	return 1;
}

float UKPCLNetworkDrive::GetPowerConsume(TSubclassOf<UKPCLNetworkDrive> InClass) {
	if(IsValid(InClass)) {
		return InClass.GetDefaultObject()->mPowerConsume;
	}
	return 0.f;
}

FText UKPCLNetworkDrive::GetItemDescriptionInternal() const {
	return GetItemDescriptionInternal_BP();
}

FText UKPCLNetworkDrive::GetItemNameInternal() const {
	FText MainTxt = Super::GetItemNameInternal();

	FFormatNamedArguments FormatPatternArgs;
	FormatPatternArgs.Empty();
	FormatPatternArgs.Add(TEXT("Tier"), FText::FromString(FString::FromInt(mDriveTier)));
	return FText::Format(MainTxt, FormatPatternArgs);
}

FText UKPCLNetworkDrive::GetItemDescriptionInternal_BP_Implementation() const {
	return mDescription;
}
