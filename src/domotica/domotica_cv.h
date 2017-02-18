/**
 * @file domotica_cv.h
 * @brief Process CV numbers for the loconet domotica module
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#include <stdint.h>
#include "loconet/loconet.h"

// ----------------------------------------------------------------------------
// Define the LNCV start and end ranges of the different elements, if not
// specified in any other module.
#ifndef DOMOTICA_LNCV_START_OUTPUT_BRIGHTNESS
  #define DOMOTICA_LNCV_START_OUTPUT_BRIGHTNESS 14
#endif
#ifndef DOMOTICA_LNCV_END_OUTPUT_BRIGHTNESS
  #define DOMOTICA_LNCV_END_OUTPUT_BRIGHTNESS (DOMOTICA_LNCV_START_OUTPUT_BRIGHTNESS + DOMOTICA_OUTPUT_SIZE)
#endif
#ifndef DOMOTICA_LNCV_START_INPUT_ADDRESSES
  #define DOMOTICA_LNCV_START_INPUT_ADDRESSES DOMOTICA_LNCV_END_OUTPUT_BRIGHTNESS
#endif
#ifndef DOMOTICA_LNCV_END_INPUT_ADDRESSES
  #define DOMOTICA_LNCV_END_INPUT_ADDRESSES (DOMOTICA_START_INPUT_ADDRESSES + 5 * DOMOTICA_RX_INPUT_ADDRESS_SIZE)
#endif
#ifndef DOMOTICA_LNCV_START_FAST_CLOCK
  #define DOMOTICA_LNCV_START_FAST_CLOCK DOMOTICA_END_INPUT_ADDRESSES
#endif
#ifndef DOMOTICA_LNCV_END_FAST_CLOCK
  #define DOMOTICA_LNCV_END_FAST_CLOCK (DOMOTICA_START_FAST_CLOCK + 3 * DOMOTICA_FAST_CLOCK_SIZE)
#endif

// ----------------------------------------------------------------------------
void loconet_cv_written_event(uint16_t lncv_number, uint16_t value);

// ----------------------------------------------------------------------------
uint8_t loconet_cv_write_allowed(uint16_t, uint16_t);
