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

#include <stdbool.h>
#include <stdint.h>
#include "components/fast_clock.h"
#include "loconet/loconet_cv.h"
#include "domotica.h"

// ----------------------------------------------------------------------------
// Sets the LNCV address to timestamp in the FAST_CLOCK react list.
void domotica_fastclock_set(uint16_t lncv, uint16_t timestamp);

// ----------------------------------------------------------------------------
// Removes the LNCV address from the FAST_CLOCK react list.
void domotica_fastclock_remove(uint16_t lncv);

// ----------------------------------------------------------------------------
// If enable is true, then the FAST_CLOCK react list is enabled.
void domotica_fastclock_enable(bool enable);

// ----------------------------------------------------------------------------
// This function reacts on the fast clock updates. Everytime, it checks whether
// the time passed equals a time stamp in the fast clock handle array
void fast_clock_handle_update(FAST_CLOCK_TIME_Type time);

#endif
