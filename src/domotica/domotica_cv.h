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
#include "loconet/loconet_cv.h"

#include "domotica.h"
#include "domotica_rx.h"
#include "domotica_fastclock.h"

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

// ----------------------------------------------------------------------------
// Initializes all cv numbers
void domotica_cv_init(void);

#endif
