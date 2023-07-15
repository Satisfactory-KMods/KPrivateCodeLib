#include "KPrivateCodeLibModule.h"

#include "Patching/NativeHookManager.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

DEFINE_LOG_CATEGORY(LogKPCL);

void PlayerStateBeginPlayer(CallScope<void(*)(AFGPlayerState*)>& scope, AFGPlayerState* State) {
	if(State->GetWorld()) {
		if(AKPCLUnlockSubsystem* Sub = AKPCLUnlockSubsystem::Get(State->GetWorld())) {
			Sub->RegisterPlayerState(State);
		}
	}
}

void FKPrivateCodeLib::StartupModule() {
#if !WITH_EDITOR
	// Config
	const TArray<FString> ModModuleNames = {
		"KBFL",
		"RSS",
		"AwesomeSinkStorage",
		"KDecoLib",
		"KLib",
		"KPrivateCodeLib",
		"KUI",
		"MicrowavePower",
		"PimpMyFactory",
		"SatisfactoryPlus",
		"PneumaticFrackingMachine"
	};

	// Logic
	int32 Added = 0;
	
	TArray<FString> NewLocalizationPaths;
	GConfig->GetArray( TEXT("Internationalization"), TEXT("LocalizationPaths"), NewLocalizationPaths, GGameIni );
	
	for (FString ModModuleName : ModModuleNames )
	{
		FString LocPath = "../../../FactoryGame/Mods/{ModuleName}/Localization/{ModuleName}";
		LocPath.ReplaceInline( *FString( "{ModuleName}" ), *ModModuleName );
		if(NewLocalizationPaths.AddUnique( LocPath ) > 0)
		{
			UE_LOG( LogTemp, Log, TEXT("Module: %s ; Added LocalizationPath: %s"), *FString("PrivateCodeLib"), *LocPath );
			Added++;
		}
	}

	GConfig->SetArray( TEXT("Internationalization"), TEXT("LocalizationPaths"), NewLocalizationPaths, GGameIni );
	GConfig->SetArray( TEXT("Internationalization"), TEXT("LocalizationPaths"), NewLocalizationPaths, GEngineIni );
#endif

#if !WITH_EDITOR
	SUBSCRIBE_METHOD_VIRTUAL( AFGPlayerState::BeginPlay, GetMutableDefault<AFGPlayerState>(), &PlayerStateBeginPlayer )
#endif
}

IMPLEMENT_GAME_MODULE(FKPrivateCodeLib, KPrivateCodeLib);
