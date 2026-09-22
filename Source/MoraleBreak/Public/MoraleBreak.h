// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * MoraleBreak - squad morale, panic and rout.
 *
 * The module registers the console commands; the plugin itself is the component, the subsystem and the
 * pure rules they share.
 */
class FMoraleBreakModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
