#pragma once

#include "Config.h"

#if HIKOKI_BOARD == HIKOKI_BOARD_XIAO
#include "profiles/Xiao.h"
#elif HIKOKI_BOARD == HIKOKI_BOARD_RAYTAC_DEV_DONGLE
#include "profiles/RaytacDevDongle.h"
#endif
