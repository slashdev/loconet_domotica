/*
 * @file outputhandler.c
 * @brief Loconet Output handler for domotica
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 */

#include "outputhandler.h"

// ------------------------------------------------------------------
// Prototypes
void outputhandler_set_output_state_dummy(uint8_t output, uint8_t brightness);

// ----------------------------------------------------------------------------
// This prototype is used for setting the actual output to the given brightness
__attribute__ ((weak, alias ("outputhandler_set_output_state_dummy"))) \
  void outputhandler_set_output_state(uint8_t output, uint8_t brightness);


// ----------------------------------------------------------------------------
// Maintains the current state of the system
static uint16_t state = 0;

// ------------------------------------------------------------------
// Output brightness
static uint8_t output_brightness[DOMOTICA_OUTPUT_SIZE];

// ----------------------------------------------------------------------------
void outputhandler_set_output_brightness(uint8_t output, uint8_t brightness)
{
  if (output < DOMOTICA_OUTPUT_SIZE && brightness <= DOMOTICA_OUTPUT_MAX_BRIGHTNESS)
  {
    output_brightness[output] = brightness;
  }
}

void domotica_handle_output_change(uint16_t mask_on, uint16_t mask_off)
{
  // first we check the mask for switching off outputs
  // We take the complement of the state, i.e., it is a 1 if the output is
  // switched off, and 0 if it is switched on. Then we compare with mask_off to
  // see whether the mask differs from the state. If so, we check whether we
  // should do something (& mask_off)
  uint16_t delta = ((~state) ^ mask_off) & mask_off;
  uint8_t index;
  for(index = 0 ; index < DOMOTICA_OUTPUT_SIZE ; index++)
  {
    if(delta & (1 << index))
    {
      outputhandler_set_output_state(index, 0);
      state &= ~(1 << index);
    }
  }

  // similarly, for switching on...
  delta = ((state) ^ mask_on) & mask_on;

  for(index = 0 ; index < DOMOTICA_OUTPUT_SIZE ; index++)
  {
    if(delta & (1 << index))
    {
      outputhandler_set_output_state(index, output_brightness[index]);
      state |= (1 << index);
    }
  }
}

void outputhandler_set_output_state_dummy(uint8_t output, uint8_t brightness)
{
  (void) output;
  (void) brightness;
}

uint16_t outputhandler_get_state(void)
{
  return state;
}
