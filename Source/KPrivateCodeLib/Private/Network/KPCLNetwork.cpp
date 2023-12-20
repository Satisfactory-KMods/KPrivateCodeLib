// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetwork.h"

#include "KPrivateCodeLibModule.h"

#include "Net/UnrealNetwork.h"

#include "Network/KPCLNetworkInfoComponent.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/Buildings/KPCLNetworkTeleporter.h"

void UKPCLNetwork::GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const {
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UKPCLNetwork, mCoreBuilding );
	DOREPLIFETIME( UKPCLNetwork, mNetworkBytesForSolids );
	DOREPLIFETIME( UKPCLNetwork, mNetworkBytesForFluids );
	DOREPLIFETIME( UKPCLNetwork, mConnectionBuildings );
	DOREPLIFETIME( UKPCLNetwork, mConnectionManufacturerBuildings );
	DOREPLIFETIME( UKPCLNetwork, mConnectedTeleporterInNetwork );

	DOREPLIFETIME( UKPCLNetwork, mNetworkMaxInputFluid );
	DOREPLIFETIME( UKPCLNetwork, mNetworkMaxInputSolid );
	DOREPLIFETIME( UKPCLNetwork, mNetworkMaxOutputFluid );
	DOREPLIFETIME( UKPCLNetwork, mNetworkMaxOutputSolid );
}

void UKPCLNetwork::TickCircuit( float dt ) {
	mHasPower = true;
	Super::TickCircuit( dt );
	mHasPower = true;
	mIsFuseTriggered = false;
	mPowerProduced = 50000000.f;

	if( HasAuthority( ) && CoreStateIsOk( ) ) {
		if( IsFuseTriggered( ) ) {
			ResetFuse( );
		}

		if( !IsValid( GetCore( ) ) ) {
			return;
		}

		UpdateBytesInNetwork( );
	}
}

void UKPCLNetwork::UpdateBytesInNetwork( ) {
	FCriticalSection Mutex;
	ReBuildGroups( );

	if( !IsValid( GetCore( ) ) ) {
		return;
	}

	const UKPCLNetworkInfoComponent* Core = GetCore( )->GetNetworkInfoComponent( );

	mNetworkMaxInputFluid = Core->GetMaxInput( true );
	mNetworkMaxOutputFluid = Core->GetMaxOutput( true );
	mNetworkMaxInputSolid = Core->GetMaxInput( );
	mNetworkMaxOutputSolid = Core->GetMaxOutput( );

	int32 NewBytes = 0;
	int32 NewFluidBytes = 0;

	for( TArray< UKPCLNetworkInfoComponent* > Group : mNetworkGroups ) {
		const int32 NumPerGroup = FMath::Max( FMath::DivideAndRoundUp( Group.Num( ), 7 ), 1 );
		ParallelFor( 7, [&, Group]( int32 Index ) {
			int32 Bytes = 0;
			int32 FluidBytes = 0;

			for( int32 Member = Index * NumPerGroup; Member < FMath::Min( ( Index + 1 ) * NumPerGroup, Group.Num( ) ); Member++ ) {
				if( UKPCLNetworkInfoComponent* NetWorkInfo = Cast< UKPCLNetworkInfoComponent >( Group[ Member ] ) ) {
					NetWorkInfo->SetHasCore( NetworkHasCore( ) );
					if( NetWorkInfo->IsFluidBytesHandler( ) ) {
						FluidBytes += NetWorkInfo->GetBytes( );
					}
					else {
						Bytes += NetWorkInfo->GetBytes( );
					}

					if( NetWorkInfo == Core ) {
						continue;
					}

					NetWorkInfo->SetMax( mNetworkMaxInputFluid, mNetworkMaxOutputFluid, true );
					NetWorkInfo->SetMax( mNetworkMaxInputSolid, mNetworkMaxOutputSolid );
				}
			}

			Mutex.Lock( );
			NewBytes += Bytes;
			NewFluidBytes += FluidBytes;
			Mutex.Unlock( );
		} );
	}

	mNetworkBytesForFluids = NewFluidBytes;
	mNetworkBytesForSolids = NewBytes;
}

void UKPCLNetwork::OnCircuitChanged( ) {
	Super::OnCircuitChanged( );

	FCriticalSection Mutex;
	AKPCLNetworkCore* NewCore = nullptr;

	TArray< AKPCLNetworkConnectionBuilding* > AllBuildings;
	TArray< AKPCLNetworkManufacturerConnection* > AllManuBuildings;
	TArray< AKPCLNetworkTeleporter* > AllTeleporter;

	const int32 NumPerGroup = FMath::Max( FMath::DivideAndRoundUp( mPowerInfos.Num( ), 7 ), 1 );
	ParallelFor( 7, [&]( int32 Index ) {
		TArray< AKPCLNetworkConnectionBuilding* > Buildings;
		TArray< AKPCLNetworkManufacturerConnection* > ManuBuildings;
		TArray< AKPCLNetworkTeleporter* > TeleBuildings;

		for( int32 Member = Index * NumPerGroup; Member < FMath::Min( ( Index + 1 ) * NumPerGroup, mPowerInfos.Num( ) ); Member++ ) {
			if( const UKPCLNetworkInfoComponent* NetWorkInfo = Cast< UKPCLNetworkInfoComponent >( mPowerInfos[ Member ] ) ) {
				if( ensure( NetWorkInfo->GetOwner() ) ) {
					if( AKPCLNetworkConnectionBuilding* Owner = Cast< AKPCLNetworkConnectionBuilding >( mPowerInfos[ Member ]->GetOwner( ) ) ) {
						Buildings.Add( Owner );
					}
					else if( AKPCLNetworkManufacturerConnection* ManuOwner = Cast< AKPCLNetworkManufacturerConnection >( mPowerInfos[ Member ]->GetOwner( ) ) ) {
						ManuBuildings.Add( ManuOwner );
					}
					else if( AKPCLNetworkTeleporter* TeleporterOwner = Cast< AKPCLNetworkTeleporter >( mPowerInfos[ Member ]->GetOwner( ) ) ) {
						UE_LOG( LogKPCL, Warning, TEXT(" ADD TeleporterOwner! ") )
						TeleBuildings.Add( TeleporterOwner );
					}
					else if( AKPCLNetworkCore* Core = Cast< AKPCLNetworkCore >( mPowerInfos[ Member ]->GetOwner( ) ) ) {
						if( Core->IsCore( ) ) {
							Mutex.Lock( );
							NewCore = Core;
							Mutex.Unlock( );
						}
					}
					else {
						UE_LOG( LogKPCL, Warning, TEXT(" Unknown Class: %s "), *mPowerInfos[ Member ]->GetOwner()->GetName() );
					}
				}
			}
		}

		Mutex.Lock( );
		AllBuildings.Append( Buildings );
		AllManuBuildings.Append( ManuBuildings );
		AllTeleporter.Append( TeleBuildings );
		Mutex.Unlock( );
	} );

	mConnectionBuildings = AllBuildings;
	mConnectionManufacturerBuildings = AllManuBuildings;
	mConnectedTeleporterInNetwork = AllTeleporter;
	UE_LOG( LogKPCL, Warning, TEXT( "mConnectedTeleporterInNetwork = %d" ), mConnectedTeleporterInNetwork.Num() )

	mConnectionBuildings.Sort( []( const AKPCLNetworkConnectionBuilding& A, const AKPCLNetworkConnectionBuilding& B ) {
		FNetworkConnectionInformations AInfo;
		FNetworkConnectionInformations BInfo;
		A.GetConnectionInformations( AInfo );
		B.GetConnectionInformations( BInfo );
		return AInfo.bIsInput > BInfo.bIsInput;
	} );

	mCoreBuilding = NewCore;

	bNetworkBuildingsAreDirty = true;
	bNetworkGroupsAreDirty = true;

	//UpdateBytesInNetwork();
}

bool UKPCLNetwork::NetworkHasCore( ) const {
	return mCoreBuilding != nullptr;
}

bool UKPCLNetwork::CoreStateIsOk( ) const {
	if( mCoreBuilding ) {
		return mCoreBuilding->IsProducing( );
	}
	return false;
}

AKPCLNetworkCore* UKPCLNetwork::GetCore( ) const {
	return mCoreBuilding;
}

int32 UKPCLNetwork::GetBytes( bool AsFluid ) const {
	if( NetworkHasCore( ) ) {
		if( AsFluid ) {
			return mNetworkBytesForFluids;
		}
		return mNetworkBytesForSolids;
	}
	return 0;
}

void UKPCLNetwork::GetAllTeleporter( TArray< AKPCLNetworkTeleporter* >& OtherTeleporter ) {
	OtherTeleporter = mConnectedTeleporterInNetwork;
}

bool UKPCLNetwork::IsNetworkDirty( ) const {
	return bNetworkBuildingsAreDirty;
}

TArray< AKPCLNetworkConnectionBuilding* > UKPCLNetwork::GetNetworkConnectionBuildings( ) {
	bNetworkBuildingsAreDirty = false;
	return mConnectionBuildings;
}

TArray< AKPCLNetworkManufacturerConnection* > UKPCLNetwork::GetNetworkManufacturerConnections( ) {
	bNetworkBuildingsAreDirty = false;
	return mConnectionManufacturerBuildings;
}

int32 UKPCLNetwork::GetMaxInput( bool IsFluid ) const {
	return IsFluid ? mNetworkMaxInputFluid : mNetworkMaxInputSolid;
}

int32 UKPCLNetwork::GetMaxOutput( bool IsFluid ) const {
	return IsFluid ? mNetworkMaxOutputFluid : mNetworkMaxOutputSolid;
}

void UKPCLNetwork::ReBuildGroups( ) {
	FCriticalSection Mutex;
	if( bNetworkGroupsAreDirty ) {
		mNetworkGroups.Empty( );
		mNetworkGroups.SetNum( 100 );

		if( mPowerInfos.Num( ) <= 0 ) {
			return;
		}

		const int32 NumPerGroup = FMath::Max( FMath::DivideAndRoundUp( mPowerInfos.Num( ), 7 ), 1 );
		ParallelFor( 7, [&]( int32 Index ) {
			TArray< TArray< UKPCLNetworkInfoComponent* > > Grouping;
			Grouping.SetNum( 100 );
			for( int32 Member = Index * NumPerGroup; Member < FMath::Min( ( Index + 1 ) * NumPerGroup, mPowerInfos.Num( ) ); Member++ ) {
				const int32 GroupIndex = fmod( Index, 100 );
				if( UKPCLNetworkInfoComponent* Mem = Cast< UKPCLNetworkInfoComponent >( mPowerInfos[ Member ] ) ) {
					Grouping[ GroupIndex ].Add( Mem );
				}
			}

			for( int i = 0; i < Grouping.Num( ); ++i ) {
				if( Grouping[ i ].Num( ) <= 0 ) {
					continue;
				}

				Mutex.Lock( );
				mNetworkGroups[ i ].Append( Grouping[ i ] );
				Mutex.Unlock( );
			}
		} );

		bNetworkGroupsAreDirty = false;
	}
}
