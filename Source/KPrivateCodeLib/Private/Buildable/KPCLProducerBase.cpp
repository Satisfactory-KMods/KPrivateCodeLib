// ILikeBanas


#include "Buildable/KPCLProducerBase.h"

#include "AbstractInstanceManager.h"
#include "AkAcousticTextureSetComponent.h"
#include "FGFactoryConnectionComponent.h"
#include "FGItemPickup_Spawnable.h"
#include "FGPlayerController.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
#include "FGProductionIndicatorComponent.h"
#include "KPrivateCodeLibModule.h"
#include "BFL/KBFL_Player.h"
#include "Components/KPCLBetterIndicator.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Structures/KPCLFunctionalStructure.h"

AKPCLProducerBase::AKPCLProducerBase()
	: mFGPowerConnection(nullptr)
	, mCustomProductionStateIndicator(nullptr)
{
	bReplicates = true;
	mFactoryTickFunction.bCanEverTick = true;
	PrimaryActorTick.bCanEverTick = true;
}

#if WITH_EDITOR
void AKPCLProducerBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AKPCLProducerBase, mPowerOptions))
	{
		mPowerConsumption = mPowerOptions.GetMaxPowerConsume();
	}
}

void AKPCLProducerBase::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AKPCLProducerBase, mPowerOptions))
	{
		mPowerConsumption = mPowerOptions.GetMaxPowerConsume();
	}
}
#endif

bool AKPCLProducerBase::ShouldSave_Implementation() const
{
	return true;
}

void AKPCLProducerBase::InitInventories()
{
	if (!HasAuthority())
	{
		return;
	}

	mCachedInventorys.Empty();
	TArray<UFGInventoryComponent*> Components;
	GetComponents(Components);
	for (UFGInventoryComponent* Component : Components)
	{
		if(!IsValid(Component)) continue;
		FName ComponentName = FName(Component->GetName());
		if(mCachedInventorys.Contains(ComponentName))
		{
			UE_LOG(LogKPCL, Error, TEXT("InitInventories, with duplicate inventory %s in actor %s! > SKIP!"), *ComponentName.ToString(), *GetName());
			continue;
		}
		mCachedInventorys.Add(ComponentName, Component);
	}

	if (IsValid(GetInventory()))
	{
		InitInputInventory();
		GetInventory()->SetReplicationRelevancyOwner(this);
	}

	if (IsValid(GetOutputInventory()))
	{
		InitOutputInventory();
		GetOutputInventory()->SetReplicationRelevancyOwner(this);
	}

	if (IsValid(GetBoosterInventory()))
	{
		InitBoosterInventory();
		GetBoosterInventory()->SetReplicationRelevancyOwner(this);
	}

	SetBelts();
}

void AKPCLProducerBase::SetBelts()
{
	if (!IsValid(GetInventory()))
	{
		UE_LOG(LogKPCL, Warning, TEXT("SetBelts, with invalid inventory! > SKIP!"))
		return;
	}

	TArray<UFGFactoryConnectionComponent*> BeltsToSet = GetAllConv();
	if (BeltsToSet.Num() > 0)
	{
		for (UFGFactoryConnectionComponent* BeltConnection : BeltsToSet)
		{
			if (BeltConnection)
			{
				BeltConnection->SetInventory(GetInventory());
			}
		}
	}

	TArray<UFGPipeConnectionFactory*> PipesToSet = GetAllPipes();
	if (PipesToSet.Num() > 0)
	{
		for (UFGPipeConnectionFactory* PipeConnection : PipesToSet)
		{
			if (PipeConnection)
			{
				PipeConnection->SetInventory(GetInventory());
			}
		}
	}
}

void AKPCLProducerBase::HandleIndicator()
{
	if (!GetHasPower())
	{
		mCurrentState = ENewProductionState::NoPower;
		return;
	}
	if (IsProductionPaused())
	{
		mCurrentState = ENewProductionState::Paused;
		return;
	}
	if (IsProducing())
	{
		mCurrentState = ENewProductionState::Producing;
		return;
	}
	mCurrentState = ENewProductionState::Idle;
}

void AKPCLProducerBase::ApplyNewProductionState(ENewProductionState NewState)
{
	mLastState = NewState;
	mCurrentState = NewState;

	if (mCustomProductionStateIndicator)
	{
		mCustomProductionStateIndicator->SetState(NewState);
	}

	bOneStateWasNotApplied = false;
	for (const int32 CustomIndicatorHandleIndex : mCustomIndicatorHandleIndexes)
	{
		if (mInstanceHandles.IsValidIndex(CustomIndicatorHandleIndex))
		{
			if (!mInstanceHandles[CustomIndicatorHandleIndex]->IsInstanced())
			{
				bOneStateWasNotApplied = true;
				continue;
			}
			AIO_UpdateCustomFloat(mDefaultIndex, mIntensity, CustomIndicatorHandleIndex, true);
			AIO_UpdateCustomFloatAsColor(mDefaultIndex + 1, mStateColors[static_cast<uint8>(NewState)],
				CustomIndicatorHandleIndex, true);
			AIO_UpdateCustomFloat(mDefaultIndex + 4, mPulsStates.Contains(NewState), CustomIndicatorHandleIndex, true);
		}
	}
}

UFGFactoryConnectionComponent* AKPCLProducerBase::GetConv(int Index, ECKPCLDirection Direction) const
{
	if ((Direction == KPCLInput || Direction == KPCLAny) && mConnectionMap[EKPCLConnectionType::ConvIn].
		IsValidIndex(Index))
	{
		return Cast<UFGFactoryConnectionComponent>(mConnectionMap[EKPCLConnectionType::ConvIn][Index]);
	}

	if ((Direction == KPCLOutput || Direction == KPCLAny) && mConnectionMap[EKPCLConnectionType::ConvOut].
		IsValidIndex(Index))
	{
		return Cast<UFGFactoryConnectionComponent>(mConnectionMap[EKPCLConnectionType::ConvOut][Index]);
	}

	return nullptr;
}

TArray<UFGFactoryConnectionComponent*> AKPCLProducerBase::GetAllConv(ECKPCLDirection Direction) const
{
	TArray<UFGFactoryConnectionComponent*> Return;
	TArray<UFGConnectionComponent*>        All;

	if (Direction == KPCLInput || Direction == KPCLAny)
	{
		All.Append(mConnectionMap[EKPCLConnectionType::ConvIn]);
	}
	if (Direction == KPCLOutput || Direction == KPCLAny)
	{
		All.Append(mConnectionMap[EKPCLConnectionType::ConvOut]);
	}

	for (UFGConnectionComponent* Conv : All)
	{
		if (IsValid(Conv))
		{
			Return.AddUnique(Cast<UFGFactoryConnectionComponent>(Conv));
		}
	}

	return Return;
}

UFGPipeConnectionFactory* AKPCLProducerBase::GetPipe(int Index, ECKPCLDirection Direction) const
{
	if ((Direction == KPCLInput || Direction == KPCLAny) && mConnectionMap[EKPCLConnectionType::PipeIn].
		IsValidIndex(Index))
	{
		return Cast<UFGPipeConnectionFactory>(mConnectionMap[EKPCLConnectionType::PipeIn][Index]);
	}

	if ((Direction == KPCLOutput || Direction == KPCLAny) && mConnectionMap[EKPCLConnectionType::PipeOut].
		IsValidIndex(Index))
	{
		return Cast<UFGPipeConnectionFactory>(mConnectionMap[EKPCLConnectionType::PipeOut][Index]);
	}

	return nullptr;
}

TArray<UFGPipeConnectionFactory*> AKPCLProducerBase::GetAllPipes(ECKPCLDirection Direction) const
{
	TArray<UFGPipeConnectionFactory*> Return;
	TArray<UFGConnectionComponent*>   All;

	if (Direction == KPCLInput || Direction == KPCLAny)
	{
		All.Append(mConnectionMap[EKPCLConnectionType::PipeIn]);
	}
	if (Direction == KPCLOutput || Direction == KPCLAny)
	{
		All.Append(mConnectionMap[EKPCLConnectionType::PipeOut]);
	}

	for (UFGConnectionComponent* Conv : All)
	{
		if (IsValid(Conv))
		{
			Return.AddUnique(Cast<UFGPipeConnectionFactory>(Conv));
		}
	}

	return Return;
}

void AKPCLProducerBase::BeginPlay()
{
	Super::BeginPlay();
	mAssetSubsystem = UKAPIDataAssetSubsystem::GetChecked(GetWorld());

	InitComponents();

	if (HasAuthority())
	{
		InitInventories();
		HandlePowerInit();
		SetBelts();
	}

	if (!bDeferBeginPlay)
	{
		ReadyForVisuelUpdate();
	}

	InitAudioConfig();
}

void AKPCLProducerBase::OnBuildEffectFinished()
{
	Super::OnBuildEffectFinished();
	ReadyForVisuelUpdate();
}

void AKPCLProducerBase::OnBuildEffectActorFinished()
{
	Super::OnBuildEffectActorFinished();
	ReadyForVisuelUpdate();
}

void AKPCLProducerBase::ReadyForVisuelUpdate()
{
	InitMeshOverwriteInformation();
	ApplyNewProductionState(GetCurrentProductionState());
}

void AKPCLProducerBase::InitAudioConfig()
{
	UConfigPropertyFloat* Property = mAudioConfig.GetPropertyAsType(GetWorld());
	if (IsValid(Property))
	{
		TArray<UAudioComponent*> AudioComponents;
		GetComponents(AudioComponents);

		if (AudioComponents.Num() > 0)
		{
			for (UAudioComponent* AudioComponent : AudioComponents)
			{
				mAudioComponents.Add(FKPCLAudioComponent(AudioComponent));
			}
			Property->OnPropertyValueChanged.AddUniqueDynamic(this, &AKPCLProducerBase::OnAudioConfigChanged_Native);
			OnAudioConfigChanged_Native();
		}
	}
}

void AKPCLProducerBase::OnAudioConfigChanged_Native()
{
	for (FKPCLAudioComponent AudioComponent : mAudioComponents)
	{
		AudioComponent.SetVolumePercent(mAudioConfig.GetValue(GetWorld()));
	}
	OnAudioConfigChanged();
}

void AKPCLProducerBase::StartIsLookedAtForDismantle_Implementation(AFGCharacterPlayer* byCharacter)
{
	UpdateInstancesForOutline();
	Super::StartIsLookedAtForDismantle_Implementation(byCharacter);
}

void AKPCLProducerBase::StartIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state)
{
	UpdateInstancesForOutline();
	Super::StartIsLookedAt_Implementation(byCharacter, state);
}

void AKPCLProducerBase::StartIsAimedAtForColor_Implementation(AFGCharacterPlayer* byCharacter, bool isValid)
{
	UpdateInstancesForOutline();
	Super::StartIsAimedAtForColor_Implementation(byCharacter, isValid);
}

void AKPCLProducerBase::StartIsLookedAtForConnection(AFGCharacterPlayer* byCharacter,
	UFGCircuitConnectionComponent*                                       overlappingConnection)
{
	UpdateInstancesForOutline();
	Super::StartIsLookedAtForConnection(byCharacter, overlappingConnection);
}

void AKPCLProducerBase::UpdateInstancesForOutline() const
{
	TArray<UKPCLColoredStaticMesh*> ColoredStaticMeshes;
	GetComponents<UKPCLColoredStaticMesh>(ColoredStaticMeshes);
	for (UKPCLColoredStaticMesh* ColoredStaticMesh : ColoredStaticMeshes)
	{
		if (ColoredStaticMesh)
		{
			ColoredStaticMesh->ApplyTransformToComponent();
		}
	}
}

void AKPCLProducerBase::ReApplyColorForIndex(int32 Idx, const FFactoryCustomizationData& customizationData)
{
	if (!mInstanceHandles.IsValidIndex(Idx) || !DoesContainLightweightInstances_Native())
	{
		return;
	}

	if (mInstanceHandles[Idx]->IsInstanced())
	{
		int32 NewNum = FMath::Clamp(mInstanceDataCDO->GetInstanceData()[Idx].NumCustomDataFloats, 0, 100);
		TArray<float> Datas = customizationData.Data;
		Datas.SetNum(NewNum);
		if (mCachedCustomData.Contains(Idx))
		{
			for (TTuple<int, float> Result : mCachedCustomData[Idx])
			{
				Datas[Result.Key] = Result.Value;
			}
		}
		AAbstractInstanceManager::SetCustomPrimitiveDataOnHandle(mInstanceHandles[Idx], Datas, true);
	}
}

void AKPCLProducerBase::ApplyCustomizationData_Native(const FFactoryCustomizationData& customizationData)
{
	Super::ApplyCustomizationData_Native(customizationData);

	for (int32 Idx = 0; Idx < mInstanceHandles.Num(); ++Idx)
	{
		ReApplyColorForIndex(Idx, customizationData);
	}
}

void AKPCLProducerBase::SetCustomizationData_Native(const FFactoryCustomizationData& customizationData,
	bool                                                                             skipCombine)
{
	Super::SetCustomizationData_Native(customizationData, skipCombine);

	if (DoesContainLightweightInstances_Native())
	{
		for (int32 Idx = 0; Idx < mInstanceHandles.Num(); ++Idx)
		{
			ReApplyColorForIndex(Idx, customizationData);
		}
	}
}

void AKPCLProducerBase::InitMeshOverwriteInformation()
{
	if (!DoesContainLightweightInstances_Native())
	{
		return;
	}

	if (mInstanceHandles.Num() <= 0)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AKPCLProducerBase::InitMeshOverwriteInformation);
		return;
	}

	if (!mInstanceHandles[0]->IsInstanced())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AKPCLProducerBase::InitMeshOverwriteInformation);
		return;
	}

	for (int32 Idx = 0; Idx < mInstanceHandles.Num(); ++Idx)
	{
		ApplyMeshOverwriteInformation(Idx);

		FKPCLMeshOverwriteInformation Information;
		if (ShouldOverwriteIndexHandle(Idx, Information))
		{
			ApplyMeshInformation(Information);
		}
	}
	ApplyNewProductionState(GetCurrentProductionState());
}

void AKPCLProducerBase::ApplyMeshOverwriteInformation(int32 Idx)
{
	for (FKPCLMeshOverwriteInformation MeshOverwriteInformation : mDefaultMeshOverwriteInformations)
	{
		if (MeshOverwriteInformation.mOverwriteHandleIndex == Idx)
		{
			ApplyMeshInformation(MeshOverwriteInformation);
			return;
		}
	}

	for (FKPCLMeshOverwriteInformation MeshOverwriteInformation : mMeshOverwriteInformations)
	{
		if (MeshOverwriteInformation.mOverwriteHandleIndex == Idx)
		{
			ApplyMeshInformation(MeshOverwriteInformation);
			return;
		}
	}
}

void AKPCLProducerBase::ApplyMeshInformation(FKPCLMeshOverwriteInformation Information)
{
	if (!Information.mUseCustomTransform)
	{
		AIO_OverwriteInstanceData(Information.mOverwriteMesh, Information.mOverwriteHandleIndex);
	}
	else
	{
		AIO_OverwriteInstanceData_Transform(Information.mOverwriteMesh, Information.mCustomTransform,
			Information.mOverwriteHandleIndex);
	}
}

bool AKPCLProducerBase::ShouldOverwriteIndexHandle(int32 Idx, FKPCLMeshOverwriteInformation& Information)
{
	return false;
}

bool AKPCLProducerBase::AIO_OverwriteInstanceData(UStaticMesh* Mesh, int32 Idx)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, Mesh, Idx ]() {
			AIO_OverwriteInstanceData(Mesh, Idx);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (Idx > INDEX_NONE && IsValid(Mesh))
	{
		TArray<FInstanceData> Datas = mInstanceDataCDO->GetInstanceData();
		if (Datas.IsValidIndex(Idx))
		{
			return AIO_OverwriteInstanceData_Transform(Mesh, Datas[Idx].RelativeTransform, Idx);
		}
	}
	return false;
}

bool AKPCLProducerBase::AIO_OverwriteInstanceData_Transform(UStaticMesh* Mesh, FTransform NewRelativTransform,
	int32                                                                Idx)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, Mesh, NewRelativTransform, Idx ]() {
			AIO_OverwriteInstanceData_Transform(Mesh, NewRelativTransform, Idx);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (Idx > INDEX_NONE && IsValid(Mesh))
	{
		AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager(GetWorld());
		TArray<FInstanceData>     Datas = mInstanceDataCDO->GetInstanceData();
		if (Datas.IsValidIndex(Idx) && mInstanceHandles.IsValidIndex(Idx) && IsValid(Manager))
		{
			FInstanceData Data = Datas[Idx];
			Data.RelativeTransform = NewRelativTransform;
			Data.StaticMesh = Mesh;

			if (mInstanceHandles[Idx]->IsInstanced())
			{
				Manager->RemoveInstance(mInstanceHandles[Idx]);
			}
			mInstanceHandles[Idx] = new FInstanceHandle();
			Manager->SetInstanced(this, GetActorTransform(), Data, mInstanceHandles[Idx]);
			mCachedTransforms.Add(Idx, NewRelativTransform * GetActorTransform());

			ApplyCustomizationData_Native(mCustomizationData);
			SetCustomizationData_Native(mCustomizationData, false);

			return true;
		}
	}

	return false;
}

bool AKPCLProducerBase::AIO_UpdateCustomFloat(int32 FloatIndex, float Data, int32 InstanceIdx, bool MarkDirty)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, FloatIndex, Data, InstanceIdx, MarkDirty ]() {
			AIO_UpdateCustomFloat(FloatIndex, Data, InstanceIdx, MarkDirty);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (mInstanceHandles.IsValidIndex(InstanceIdx))
	{
		if (mInstanceHandles[InstanceIdx]->IsInstanced())
		{
			//UE_LOG( LogKPCL, Error, TEXT("mInstanceHandles[ %d ]->SetPrimitiveDataByID( %f, %d, %d ); IsValid(%d), Component(%d), Owner(%d)"), InstanceIdx, Data, FloatIndex, MarkDirty, mInstanceHandles[ InstanceIdx ]->IsValid(), IsValid( mInstanceHandles[ InstanceIdx ]->GetInstanceComponent() ), mInstanceHandles[ InstanceIdx ]->GetOwner() == this )
			mInstanceHandles[InstanceIdx]->SetPrimitiveDataByID(Data/** float that we want to set */,
				FloatIndex /** Index where we want to set */, true);
			if (mCachedCustomData.Contains(InstanceIdx))
			{
				mCachedCustomData[InstanceIdx].Add(FloatIndex, Data);
			}
			else
			{
				TMap<int32, float> Map;
				Map.Add(FloatIndex, Data);
				mCachedCustomData.Add(InstanceIdx, Map);
			}
			return true;
		}
	}

	return false;
}

bool AKPCLProducerBase::AIO_UpdateCustomFloatAsColor(int32 StartFloatIndex, FLinearColor Data, int32 InstanceIdx,
	bool                                                   MarkDirty)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, StartFloatIndex, Data, InstanceIdx, MarkDirty ]() {
			AIO_UpdateCustomFloatAsColor(StartFloatIndex, Data, InstanceIdx, MarkDirty);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	AIO_UpdateCustomFloat(StartFloatIndex, Data.R, InstanceIdx, MarkDirty);
	AIO_UpdateCustomFloat(StartFloatIndex + 1, Data.G, InstanceIdx, MarkDirty);
	return AIO_UpdateCustomFloat(StartFloatIndex + 2, Data.B, InstanceIdx, MarkDirty);
}

bool AKPCLProducerBase::AIO_SetInstanceHidden(int32 InstanceIdx, bool IsHidden)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, InstanceIdx, IsHidden ]() {
			AIO_SetInstanceHidden(InstanceIdx, IsHidden);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (mInstanceHandles.IsValidIndex(InstanceIdx))
	{
		if (mInstanceHandles[InstanceIdx]->IsInstanced())
		{
			FTransform T = mCachedTransforms.Contains(InstanceIdx)
				? mCachedTransforms[InstanceIdx]
				: mInstanceDataCDO->GetInstanceData()[InstanceIdx].RelativeTransform *
				GetActorTransform();
			T.SetScale3D(!IsHidden ? T.GetScale3D() : FVector(0.001f));
			T.SetLocation(!IsHidden ? T.GetLocation() : FVector(0.001f));

			mInstanceHandles[InstanceIdx]->UpdateTransform(T);
			return true;
		}
	}

	return false;
}

bool AKPCLProducerBase::AIO_SetInstanceWorldTransform(int32 InstanceIdx, FTransform Transform)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, InstanceIdx, Transform ]() {
			AIO_SetInstanceWorldTransform(InstanceIdx, Transform);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (mInstanceHandles.IsValidIndex(InstanceIdx))
	{
		if (mInstanceHandles[InstanceIdx]->IsInstanced())
		{
			mInstanceHandles[InstanceIdx]->UpdateTransform(Transform);
			mCachedTransforms.Add(InstanceIdx, Transform);
			return true;
		}
	}

	return false;
}


void AKPCLProducerBase::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (bCustomFactoryTickPreCustomLogic)
	{
		if (HasAuthority())
		{
			Factory_TickAuthOnly(dt);
		}
		else
		{
			Factory_TickClientOnly(dt);
		}
	}

	if (HasAuthority())
	{
		BeltPipeGrab(dt);
		HandleUiTick(dt);

		if (mProductionHandle.TickHandle(dt, IsProducing()))
		{
			EndProductionTime();
			onProducingFinal();
		}

		if (!mPowerOptions.bWasInit)
		{
			HandlePowerInit();
		}

		if (GetPowerInfo() && mPowerOptions.bWasInit)
		{
			HandlePower(dt);
		}
	}

	if (mCustomProductionStateIndicator || mCustomIndicatorHandleIndexes.Num() > 0)
	{
		if (HasAuthority())
		{
			HandleIndicator();
		}
		if (GetCurrentCompProductionState() != GetCurrentProductionState() || bOneStateWasNotApplied)
		{
			ApplyNewProductionState(GetCurrentProductionState());
		}
	}

	if (!bCustomFactoryTickPreCustomLogic)
	{
		if (HasAuthority())
		{
			Factory_TickAuthOnly(dt);
		}
		else
		{
			Factory_TickClientOnly(dt);
		}
	}
}

bool AKPCLProducerBase::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect())
	{
		return false;
	}

	return IsProductionPaused();
}

void AKPCLProducerBase::EndProductionTime()
{
	if (GetCurrentPotential() != GetPendingPotential())
	{
		SetCurrentPotential(GetPendingPotential());
	}
}

void AKPCLProducerBase::BeltPipeGrab(float dt) {}

void AKPCLProducerBase::HandlePower(float dt)
{
	mPowerOptions.bHasPower = HasPower();
	mPowerOptions.StructureTick(dt, IsProducing());
	GetPowerInfo()->SetTargetConsumption(mPowerOptions.mIsProducer
		? FMath::Max(mPowerOptions.mForcePowerConsume, 0.1f)
		: mPowerOptions.GetPowerConsume());
	GetPowerInfo()->SetMaximumTargetConsumption(mPowerOptions.mIsProducer
		? FMath::Max(mPowerOptions.mForcePowerConsume, 0.1f)
		: mPowerOptions.GetMaxPowerConsume());
	GetPowerInfo()->SetBaseProduction(
		!mPowerOptions.mIsProducer || (!mPowerOptions.mIsProducer && mPowerOptions.mIsDynamicProducer)
		? 0.0f
		: mPowerOptions.GetMaxPowerConsume());
	GetPowerInfo()->SetDynamicProductionCapacity(!mPowerOptions.mIsProducer || !mPowerOptions.mIsDynamicProducer
		? 0.0f
		: mPowerOptions.GetMaxPowerConsume());
	GetPowerInfo()->SetFullBlast((!mPowerOptions.mIsProducer || !mPowerOptions.mIsDynamicProducer));
}

void AKPCLProducerBase::HandlePowerInit()
{
	mPowerOptions.Init();
}

void AKPCLProducerBase::HandleUiTick(float dt) {}

void AKPCLProducerBase::InitComponents()
{
	mCustomProductionStateIndicator = FindComponentByClass<UKPCLBetterIndicator>();

	mConnectionMap.Add(EKPCLConnectionType::ConvIn, {});
	mConnectionMap.Add(EKPCLConnectionType::ConvOut, {});
	mConnectionMap.Add(EKPCLConnectionType::PipeIn, {});
	mConnectionMap.Add(EKPCLConnectionType::PipeOut, {});

	TArray<UFGPowerConnectionComponent*> Infos;
	GetComponents<UFGPowerConnectionComponent>(Infos);
	for (UFGPowerConnectionComponent* Info : Infos)
	{
		if (UFGPowerConnectionComponent* AsPower = ExactCast<UFGPowerConnectionComponent>(Info))
		{
			mFGPowerConnection = AsPower;
		}
	}

	TArray<UActorComponent*> Components;
	GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		// Belts
		if (UFGFactoryConnectionComponent* ConveyorConnection = Cast<UFGFactoryConnectionComponent>(Component))
		{
			if (ConveyorConnection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT)
			{
				mConnectionMap[EKPCLConnectionType::ConvIn].Add(ConveyorConnection);
			}
			else if (ConveyorConnection->GetDirection() == EFactoryConnectionDirection::FCD_OUTPUT)
			{
				mConnectionMap[EKPCLConnectionType::ConvOut].Add(ConveyorConnection);
			}
		}

		// Pipes
		if (UFGPipeConnectionFactory* PipeConnection = Cast<UFGPipeConnectionFactory>(Component))
		{
			if (PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_CONSUMER)
			{
				mConnectionMap[EKPCLConnectionType::PipeIn].Add(PipeConnection);
			}
			else if (PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_PRODUCER)
			{
				mConnectionMap[EKPCLConnectionType::PipeOut].Add(PipeConnection);
			}

			// make sure that this building use AdditionalPressure!!!
			if (!PipeConnection->mApplyAdditionalPressure)
			{
				PipeConnection->mApplyAdditionalPressure = true;
			}
		}
	}
}

float AKPCLProducerBase::GetMaxPowerConsume() const
{
	return mPowerOptions.GetMaxPowerConsume();
}

float AKPCLProducerBase::GetPowerConsume() const
{
	return mPowerOptions.GetPowerConsume();
}

bool AKPCLProducerBase::GetHasPower() const
{
	return mPowerOptions.bHasPower;
}

UFGPowerConnectionComponent* AKPCLProducerBase::GetPowerConnection() const
{
	return mFGPowerConnection;
}

float AKPCLProducerBase::GetCurrentPowerMultiplier() const
{
	return mPowerOptions.mPowerMultiplier;
}

void AKPCLProducerBase::SetPowerMultiplier(float NewMultiplier)
{
	mPowerOptions.mPowerMultiplier = NewMultiplier;
}

float AKPCLProducerBase::GetDefaultProductionCycleTime() const
{
	return mProductionHandle.mProductionTime;
}

float AKPCLProducerBase::GetProductionProgress() const
{
	return mProductionHandle.mCurrentTime / mProductionHandle.mProductionTime;
}

void AKPCLProducerBase::SetPendingPotential(float NewPendingPotential)
{
	Super::SetPendingPotential(NewPendingPotential);
	mProductionHandle.mPendingPotential = NewPendingPotential;
}

float AKPCLProducerBase::CalcProductionCycleTimeForPotential(float potential) const
{
	return mProductionHandle.mProductionTime / (FMath::Max(mProductionHandle.mExtraPotential,
		mProductionHandle.mPendingExtraPotential) + potential);
}

float AKPCLProducerBase::GetProducingPowerConsumptionBase() const
{
	return mPowerOptions.mNormalPowerConsume;
}

void AKPCLProducerBase::CollectBelts() {}

void AKPCLProducerBase::CollectAndPushPipes(float dt, bool IsPush) {}


void AKPCLProducerBase::Factory_CollectInput_Implementation()
{
	Super::Factory_CollectInput_Implementation();

	if (HasAuthority())
	{
		CollectBelts();
	}
}

void AKPCLProducerBase::Factory_PullPipeInput_Implementation(float dt)
{
	Super::Factory_PullPipeInput_Implementation(dt);

	if (HasAuthority())
	{
		CollectAndPushPipes(dt, false);
	}
}

void AKPCLProducerBase::Factory_PushPipeOutput_Implementation(float dt)
{
	Super::Factory_PushPipeOutput_Implementation(dt);

	if (HasAuthority())
	{
		CollectAndPushPipes(dt, true);
	}
}

int32 AKPCLProducerBase::GetFullestStackIndex(UFGInventoryComponent* InventoryComponent)
{
	int32 Idx = -1;

	if (InventoryComponent)
	{
		if (!InventoryComponent->IsEmpty())
		{
			int32 Fullest = 0;
			for (int32 Index = 0; Index < InventoryComponent->GetSizeLinear(); ++Index)
			{
				if (!InventoryComponent->IsIndexEmpty(Index))
				{
					FInventoryStack Stack;
					InventoryComponent->GetStackFromIndex(Index, Stack);
					if (Fullest <= Stack.NumItems)
					{
						Idx = Index;
						Fullest = Stack.NumItems;
					}
				}
			}
		}
	}

	return Idx;
}

void AKPCLProducerBase::ResetProduction()
{
	mProductionHandle.Reset();
}

void AKPCLProducerBase::SetProductionTime(float NewTime, bool ShouldResetProduction)
{
	mProductionHandle.SetNewTime(NewTime, ShouldResetProduction);
}

void AKPCLProducerBase::SetPowerOption(FPowerOptions NewPowerOption)
{
	mPowerOptions.OverWritePowerOptions(NewPowerOption);
}

FPowerOptions AKPCLProducerBase::GetPowerOption() const
{
	return mPowerOptions;
}

FPowerOptions& AKPCLProducerBase::GetPowerOptionRef()
{
	return mPowerOptions;
}

void AKPCLProducerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Production
	DOREPLIFETIME(AKPCLProducerBase, mProductionHandle);
	DOREPLIFETIME(AKPCLProducerBase, mPowerOptions);
	DOREPLIFETIME(AKPCLProducerBase, mCurrentState);
	DOREPLIFETIME(AKPCLProducerBase, mMeshOverwriteInformations);
}

float AKPCLProducerBase::GetProductionTime() const
{
	return mProductionHandle.GetProductionTime();
}

float AKPCLProducerBase::GetPendingProductionTime() const
{
	return mProductionHandle.GetPendingProductionTime();
}


FFullProductionHandle AKPCLProducerBase::GetProductionHandle() const
{
	return mProductionHandle;
}

void AKPCLProducerBase::FlushFluids()
{
	if (HasAuthority())
	{
		Server_DoFlush();
	}
	else
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld()))
		{
			RCO->Server_FlushFluids(this);
		}
	}
}

float AKPCLProducerBase::GetProductionCycleTime() const
{
	return mProductionHandle.GetProductionTime();
}

UFGInventoryComponent* AKPCLProducerBase::GetInventory() const
{
	if(!mCachedInventorys.Contains(FKPCLInventoryStructure::InputName)) return nullptr;
	return mCachedInventorys.FindChecked(FKPCLInventoryStructure::InputName);
}

void AKPCLProducerBase::InitInputInventory()
{
	GetInventory()->OnItemAddedDelegate_Native.BindUObject(this, &AKPCLProducerBase::OnInputItemAdded);
	GetInventory()->OnItemRemovedDelegate_Native.BindUObject(this, &AKPCLProducerBase::OnInputItemRemoved);

	if (!GetInventory()->mItemFilter.IsBoundToObject(this))
	{
		GetInventory()->mItemFilter.BindUObject(this, &AKPCLProducerBase::FilterInputInventory);
	}
	if (!GetInventory()->mFormFilter.IsBoundToObject(this))
	{
		GetInventory()->mFormFilter.BindUObject(this, &AKPCLProducerBase::FormFilterInputInventory);
	}
}

UFGInventoryComponent* AKPCLProducerBase::GetOutputInventory() const
{
	if(!mCachedInventorys.Contains(FKPCLInventoryStructure::OutputName)) return nullptr;
	return mCachedInventorys.FindChecked(FKPCLInventoryStructure::OutputName);
}

void AKPCLProducerBase::InitOutputInventory()
{
	GetOutputInventory()->OnItemAddedDelegate_Native.BindUObject(this, &AKPCLProducerBase::OnOutputItemAdded);
	GetOutputInventory()->OnItemRemovedDelegate_Native.BindUObject(this, &AKPCLProducerBase::OnOutputItemRemoved);

	if (!GetOutputInventory()->mItemFilter.IsBoundToObject(this))
	{
		GetOutputInventory()->mItemFilter.BindUObject(this, &AKPCLProducerBase::FilterOutputInventory);
	}
	if (!GetOutputInventory()->mFormFilter.IsBoundToObject(this))
	{
		GetOutputInventory()->mFormFilter.BindUObject(this, &AKPCLProducerBase::FormFilterOutputInventory);
	}
}

UFGInventoryComponent* AKPCLProducerBase::GetBoosterInventory() const
{
	if(!mCachedInventorys.Contains(FKPCLInventoryStructure::BoosterName)) return nullptr;
	return mCachedInventorys.FindChecked(FKPCLInventoryStructure::BoosterName);
}

void AKPCLProducerBase::InitBoosterInventory()
{
	GetBoosterInventory()->OnItemAddedDelegate.AddUniqueDynamic(this, &AKPCLProducerBase::OnBoosterItemAdded);
	GetBoosterInventory()->OnItemRemovedDelegate.AddUniqueDynamic(this, &AKPCLProducerBase::OnBoosterItemRemoved);

	if (!GetBoosterInventory()->mItemFilter.IsBoundToObject(this))
	{
		GetBoosterInventory()->mItemFilter.BindUObject(this, &AKPCLProducerBase::FilterBoosterInventory);
	}
	if (!GetBoosterInventory()->mFormFilter.IsBoundToObject(this))
	{
		GetBoosterInventory()->mFormFilter.BindUObject(this, &AKPCLProducerBase::FormFilterBoosterInventory);
	}
}

UFGInventoryComponent* AKPCLProducerBase::GetInventoryFromType(EKPCLInventoryType Type) const
{
	switch (Type)
	{
		case EKPCLInventoryType::Booster:
			return GetBoosterInventory();
		case EKPCLInventoryType::Output:
			return GetBoosterInventory();
		default:
			return GetInventory();
	}
}