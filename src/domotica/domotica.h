/*
 * @file domotica.h
 * @brief Loconet Domotica Module - receiving messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * To process the actual output change, implement function:
 *
 *     domotica_handle_output_change
 *
 * Add to the main loop of the program the following call:
 *
 *     domotica_loop();
 *
 * @author Jan Martijn van der Werf
 *
 * Timestamps
 * To represent timestamps, we use a hhmm notation stored in an int16. For
 * example timestamp 12:34 is internally represented as the int 1234.
 */

#ifndef DOMOTICA_CHANGE_BUFFER_Size
  #define DOMOTICA_CHANGE_BUFFER_Size 32
#endif

#ifndef _DOMOTICA_H_
#define _DOMOTICA_H_

#include <stdint.h>
#include <stdlib.h>
#include "domotica_rx.h"

// ------------------------------------------------------------------
// Initialize domotica
extern void domotica_init(void);

// ------------------------------------------------------------------
// Enqueue an output change so that it eventually will be processed
extern void domotica_enqueue_output_change(uint16_t mask_on, uint16_t mask_off);

// ------------------------------------------------------------------
// Should be added to the main loop of the program. If an output change is
// enqueued, this function pops the first, and handles it.
extern void domotica_loop(void);

#endif // _DOMOTICA_H_
