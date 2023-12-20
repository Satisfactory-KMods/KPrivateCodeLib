// ILikeBanas


#include "Buildable/KPCLProducerBase_2Slots.h"

#include "AkAcousticTextureSetComponent.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPlayerController.h"


AKPCLProducerBase_2Slots::AKPCLProducerBase_2Slots() : Super() {
	//pwr
	mInventoryDatas.Add(FKPCLInventoryStructure("Inv_Output"));
}

void AKPCLProducerBase_2Slots::ReconfigureInventory() {
	Super::ReconfigureInventory();

	if(GetOutputInventory()) {
		GetOutputInventory()->OnItemAddedDelegate.AddUniqueDynamic(this, &AKPCLProducerBase_2Slots::OnOutputItemAdded);
		GetOutputInventory()->OnItemAddedDelegate.AddUniqueDynamic(this, &AKPCLProducerBase_2Slots::OnOutputItemAdded);

		if(!GetOutputInventory()->mItemFilter.IsBoundToObject(this)) {
			GetOutputInventory()->mItemFilter.BindUObject(this, &AKPCLProducerBase_2Slots::FilterOutputInventory);
		}
		if(!GetOutputInventory()->mFormFilter.IsBoundToObject(this)) {
			GetOutputInventory()->mFormFilter.BindUObject(this, &AKPCLProducerBase_2Slots::FormFilterOutputInventory);
		}
	}
}

void AKPCLProducerBase_2Slots::SetBelts() {
	TArray<UFGFactoryConnectionComponent*> BeltsToSet = GetAllConv();
	for(UFGFactoryConnectionComponent* BeltConnection: BeltsToSet) {
		if(BeltConnection) {
			BeltConnection->SetInventory(BeltConnection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT ? GetInventory() : GetOutputInventory());
		}
	}

	TArray<UFGPipeConnectionFactory*> PipesToSet = GetAllPipes();
	for(UFGPipeConnectionFactory* PipeConnection: PipesToSet) {
		if(PipeConnection) {
			PipeConnection->SetInventory(PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_CONSUMER ? GetInventory() : GetOutputInventory());
		}
	}
}

UFGInventoryComponent* AKPCLProducerBase_2Slots::GetOutputInventory() const {
	return GetInventoryFromIndex(1);
}
