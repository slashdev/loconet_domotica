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
  if (lncv_number < DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START) {
    // do nothing. No values are set her
  }
  // By def, we have lncv_number >= DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START
  else if (lncv_number < DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_END)
  {
    // Output light settings
    domotica_set_output_brightness(lncv_number - DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START, (uint8_t) value);
  }
  // By def, we have lncv_number >= DOMOTICA_LNCV_INPUT_ADDRESSES_START
  else if (lncv_number < DOMOTICA_LNCV_INPUT_ADDRESSES_END)
  {
    if (lncv_number % 5 == DOMOTICA_LNCV_INPUT_ADDRESS_POS_ADDRESS)
    {
      // Update the address in the B2 address array
      domotica_rx_set_input_address(lncv_number, value);
    }
  }
  // By def, we have lncv_number >= DOMOTICA_LNCV_FASTCLOCK_START
  else if (lncv_number < DOMOTICA_LNCV_FASTCLOCK_END)
  {
    if (lncv_number % 3 == DOMOTICA_LNCV_FASTCLOCK_POS_TIME)
    {
      // Update the time in the FAST_CLOCK array
      domotica_fastclock_set(lncv_number, value);
    }
  }
}

// ----------------------------------------------------------------------------
uint8_t loconet_cv_write_allowed(uint16_t lncv_number, uint16_t value)
{
  if ( lncv_number > 3 && lncv_number < DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START) {
    return LOCONET_CV_ACK_ERROR_INVALID_VALUE;
  }
  // By def we have lncv_number >= DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START
  else if (lncv_number < DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_END)
  {
    // Output light settings
    return (value <= DOMOTICA_OUTPUT_MAX_BRIGHTNESS) ? LOCONET_CV_ACK_OK : LOCONET_CV_ACK_ERROR_OUTOFRANGE;
  }
  // By def, we have lncv_number >= DOMOTICA_LNCV_INPUT_ADDRESSES_START
  else if (lncv_number < DOMOTICA_LNCV_INPUT_ADDRESSES_END)
  {
    return LOCONET_CV_ACK_OK;
  }
  // By def, we have lncv_number >= DOMOTICA_LNCV_FASTCLOCK_START
  else if (lncv_number < DOMOTICA_LNCV_FASTCLOCK_END)
  {
    return LOCONET_CV_ACK_OK;
  }

  return LOCONET_CV_ACK_ERROR_INVALID_VALUE;
}

// ----------------------------------------------------------------------------
// Initializes all CV values
void domotica_cv_init(void)
{
  // Initialize the FAST_CLOCK module

  // Initialize the FAST_CLOCK timestamps
  for (uint8_t lncv_number = DOMOTICA_LNCV_FASTCLOCK_START ; lncv_number < DOMOTICA_LNCV_FASTCLOCK_START ; lncv_number += 3)
  {
    domotica_fastclock_set(lncv_number, loconet_cv_get(lncv_number));
  }

  // Initialize B2 addresses
  for (uint16_t lncv_number = DOMOTICA_LNCV_INPUT_ADDRESSES_START ; lncv_number < DOMOTICA_LNCV_INPUT_ADDRESSES_END ; lncv_number += 5)
  {
    domotica_rx_set_input_address(lncv_number, loconet_cv_get(lncv_number));
  }

  // Initialize brightness outputs
  for (uint8_t index = 0 ; index < DOMOTICA_OUTPUT_SIZE ; index++)
  {
    domotica_set_output_brightness(index, (uint8_t) loconet_cv_get(DOMOTICA_LNCV_OUTPUT_BRIGHTNESS_START + index));
  }
}
