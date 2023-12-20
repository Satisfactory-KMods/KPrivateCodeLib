#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKPCL, Log, All);

class FKPrivateCodeLib: public IModuleInterface {
	public:
		/** IModuleInterface implementation */
		virtual void StartupModule() override;

		virtual bool IsGameModule() const override { return true; };
};
