/**
 * @file domotica_cv.c
 * @brief Process CV numbers for the loconet domotica module
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#include "domotica_cv.h"

// ----------------------------------------------------------------------------
void loconet_cv_written_event(uint16_t lncv_number, uint16_t value)
{
  if (lncv_number == 0)
  {
    loconet_config.bit.ADDRESS = value;
  }
  else if (lncv_number == 2)
  {
    loconet_config.bit.PRIORITY = value;
  }
  else if (lncv_number == 3)
  {
    // Fast clock setting
  }
  else if (lncv_number >= DOMOTICA_LNCV_START_OUTPUT_BRIGHTNESS && lncv_number < DOMOTICA_LNCV_END_OUTPUT_BRIGHTNESS)
  {
    // Output light settings
  }
  // TODO: Refactor the % 5 = 0 to % 5 = DOMOTICA_INPUT_ADDRESS_Mask
  else if (lncv_number >= DOMOTICA_LNCV_START_INPUT_ADDRESSES && lncv_number < DOMOTICA_LNCV_END_INPUT_ADDRESSES)
  {
    // Update the address in the B2 address array
  }
  else if (lncv_number >= DOMOTICA_LNCV_START_FAST_CLOCK && lncv_number < DOMOTICA_LNCV_END_FAST_CLOCK)
  {
    // Update the Fast clock points
  }
}

// ----------------------------------------------------------------------------
uint8_t loconet_cv_write_allowed(uint16_t lncv_number, uint16_t value)
{
  (void)lncv_number;
  (void)value;
  return LOCONET_CV_ACK_OK;
}
