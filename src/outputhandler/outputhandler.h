/*
 * @file outputhandler.h
 * @brief Loconet Output handler for domotica
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 */

#include <stdint.h>

// TODO: Remove after merge!!
#ifndef DOMOTICA_OUTPUT_SIZE
  #define DOMOTICA_OUTPUT_SIZE 16
#endif

// TODO: Remove after merge!!
#ifndef DOMOTICA_OUTPUT_MAX_BRIGHTNESS
  #define DOMOTICA_OUTPUT_MAX_BRIGHTNESS 100
#endif

#ifndef _OUTPUTHANDLER_OUTPUTHANDLER_H_
#define _OUTPUTHANDLER_OUTPUTHANDLER_H_

// ----------------------------------------------------------------------------
// Sets the brighness of #output to #brightness
extern void outputhandler_set_output_brightness(uint8_t output, uint8_t brightness);

//-----------------------------------------------------------------------------
// This function implements the actual output change, determined by the
// domotica controllers.
extern void domotica_handle_output_change(uint16_t mask_on, uint16_t mask_off);

// ----------------------------------------------------------------------------
// Returns the current state of all outputs together
extern uint16_t outputhandler_get_state(void);

#endif
