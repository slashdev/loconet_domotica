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

// TODO: Add variables to domotica_rx.h and domotica_fastclock.h, respectively.
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

#ifndef _DOMOTICA_CV_H_
#define _DOMOTICA_CV_H_

// ----------------------------------------------------------------------------
// The different LNCV numbers used in the modulo calculation to decide
// what function the LNCV number has in the input address range.
#define DOMOTICA_LNCV_INPUT_ADDRESS_POS_ADDRESS (DOMOTICA_LNCV_INPUT_ADDRESSES_START % 5)
#define DOMOTICA_LNCV_INPUT_ADDRESS_POS_MODH_ON ((DOMOTICA_LNCV_INPUT_ADDRESSES_START+1) % 5)
#define DOMOTICA_LNCV_INPUT_ADDRESS_POS_MODH_OFF ((DOMOTICA_LNCV_INPUT_ADDRESSES_START+2) % 5)
#define DOMOTICA_LNCV_INPUT_ADDRESS_POS_MODL_ON ((DOMOTICA_LNCV_INPUT_ADDRESSES_START+3) % 5)
#define DOMOTICA_LNCV_INPUT_ADDRESS_POS_MODL_OFF ((DOMOTICA_LNCV_INPUT_ADDRESSES_START+4) % 5)

// The different LNCV numbers used in the modulo calculation to decide
// what function the LNCV number has in the FAST CLOCK range.
#define DOMOTICA_LNCV_FASTCLOCK_POS_TIME (DOMOTICA_LNCV_FASTCLOCK_START % 3)
#define DOMOTICA_LNCV_FASTCLOCK_POS_MASK_ON ((DOMOTICA_LNCV_FASTCLOCK_START + 1) % 3)
#define DOMOTICA_LNCV_FASTCLOCK_POS_MASK_OFF ((DOMOTICA_LNCV_FASTCLOCK_START + 2) % 3)

// ----------------------------------------------------------------------------
void loconet_cv_written_event(uint16_t lncv_number, uint16_t value);

// ----------------------------------------------------------------------------
uint8_t loconet_cv_write_allowed(uint16_t, uint16_t);

#endif
