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
#include "domotica_cv.h"
#include "domotica_rx.h"


// ----------------------------------------------------------------------------
#ifndef DOMOTICA_OUTPUT_SIZE
  #define DOMOTICA_OUTPUT_SIZE 16
#endif

#ifndef DOMOTICA_RX_INPUT_ADDRESS_SIZE
  #define DOMOTICA_RX_INPUT_ADDRESS_SIZE 6
#endif

#ifndef DOMOTICA_FASTCLOCK_SIZE
  #define DOMOTICA_FASTCLOCK_SIZE 60
#endif

// ----------------------------------------------------------------------------
// Define the LNCV start and end ranges of the different elements, if not
// specified in any other module.
#ifndef DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START
  #define DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START 14
#endif
#ifndef DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_END
  #define DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_END (DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START + DOMOTICA_OUTPUT_SIZE)
#endif
#ifndef DOMOTICA_LNCV_INPUT_ADDRESSES_START
  #define DOMOTICA_LNCV_INPUT_ADDRESSES_START DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_END
#endif
#ifndef DOMOTICA_LNCV_INPUT_ADDRESSES_END
  #define DOMOTICA_LNCV_INPUT_ADDRESSES_END (DOMOTICA_LNCV_INPUT_ADDRESSES_START + 5 * DOMOTICA_RX_INPUT_ADDRESS_SIZE)
#endif
#ifndef DOMOTICA_LNCV_FASTCLOCK_START
  #define DOMOTICA_LNCV_FASTCLOCK_START DOMOTICA_LNCV_INPUT_ADDRESSES_END
#endif
#ifndef DOMOTICA_LNCV_FASTCLOCK_END
  #define DOMOTICA_LNCV_FASTCLOCK_END (DOMOTICA_LNCV_FASTCLOCK_START + 3 * DOMOTICA_FASTCLOCK_SIZE)
#endif

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

// ------------------------------------------------------------------
// Sets the output to a certain brightness
void domotica_set_output_brightness(uint8_t output, uint8_t value);

// ------------------------------------------------------------------
// Gets the brightness of an output
uint8_t domotica_get_output_brightness(uint8_t output);


#endif // _DOMOTICA_H_
