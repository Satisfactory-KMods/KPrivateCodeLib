// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildable/Modular/KPCLModularAttachmentDescriptor.h"

TArray<TSubclassOf<UKPCLModularAttachmentDescriptor>> UKPCLModularAttachmentDescriptor::GetAttachmentDependencies(
	TSubclassOf<UKPCLModularAttachmentDescriptor> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mDependencies;
	}
	return TArray<TSubclassOf<UKPCLModularAttachmentDescriptor>>();
}

FText UKPCLModularAttachmentDescriptor::GetAttachmentName(TSubclassOf<UKPCLModularAttachmentDescriptor> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mName;
	}
	return FText::GetEmpty();
}