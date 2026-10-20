// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** Log category of NsSession */
NSSESSION_API DECLARE_LOG_CATEGORY_EXTERN(LogNsSession, Log, All);

/** Module of NsSession. It has no startup work */
class FNsSessionModule : public IModuleInterface
{
};
