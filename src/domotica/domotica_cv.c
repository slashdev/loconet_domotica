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
  else if (lncv_number >= 14 && lncv_number < 30)
  {
    // Output light settings
  }
  else if (lncv_number >= 30 && lncv_number < 56 && lncv_number % 5 == 0 )
  {
    // Update the address in the B2 address array
  }
  else if (lncv_number >= 60 && lncv_number < 240)
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
