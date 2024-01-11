#pragma once

#include "retroarch.h"

void drivers_init(settings_t* settings, int flags, bool verbosity_enabled);
void driver_uninit(int flags);

#define DRIVERS_CMD_ALL 0
