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

#include "domotica.h"

#ifndef DOMOTICA_RX_INPUT_ADDRESS_SIZE
  #define DOMOTICA_RX_INPUT_ADDRESS_SIZE 6
#endif
#ifndef DOMOTICA_FAST_CLOCK_SIZE
  #define DOMOTICA_FAST_CLOCK_SIZE 6
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
#ifndef DOMOTICA_LNCV_FAST_CLOCK_START
  #define DOMOTICA_LNCV_FAST_CLOCK_START DOMOTICA_LNCV_INPUT_ADDRESSES_END
#endif
#ifndef DOMOTICA_LNCV_FAST_CLOCK
  #define DOMOTICA_LNCV_FAST_CLOCK_END (DOMOTICA_LNCV_FAST_CLOCK_START + 3 * DOMOTICA_FAST_CLOCK_SIZE)
#endif

#ifndef _DOMOTICA_CV_H_
#define _DOMOTICA_CV_H_


// ----------------------------------------------------------------------------
void loconet_cv_written_event(uint16_t lncv_number, uint16_t value);

// ----------------------------------------------------------------------------
uint8_t loconet_cv_write_allowed(uint16_t, uint16_t);

#endif
