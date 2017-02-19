/*
 * @file domotica_fastclock.h
 * @brief Loconet Domotica Module - listen to the fast clock
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 */

#ifndef _DOMOTICA_FASTCLOCK_H_
#define _DOMOTICA_FASTCLOCK_H_

#include <stdint.h>
#include "components/fast_clock.h"

void fast_clock_handle_update(FAST_CLOCK_TIME_Type time);

#endif
