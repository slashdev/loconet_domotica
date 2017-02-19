/*
 * @file domotica_fastclock.c
 * @brief Loconet Domotica Module - listen to the fast clock
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 */

#include "domotica_fastclock.h"

// ----------------------------------------------------------------------------
// This function reacts on the fast clock updates. Everytime, it checks whether
// the time passed equals a time stamp in the fast clock handle array
void fast_clock_handle_update(FAST_CLOCK_TIME_Type time){
  (void) time;
}
