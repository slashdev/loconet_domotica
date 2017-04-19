/*
 * @file outputhandler.h
 * @brief Loconet Output handler for domotica
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 *
 * Dummy functions that can be implemented to actually control the output
 * changes:
 *  - outputhandler_switch_state_pre_event(uint16_t state)
 *  - outputhandler_switch_state_post_event(uint16_t state)
 *  - outputhandler_set_output_state(uint8_t output, uint8_t brightness)
 *
 * The latter function is called for each output, with brightness varying
 * between 0 and DOMOTICA_OUTPUT_MAX_BRIGHTNESS, where 0 is used to indicate
 * that the output should be switched off.
 *
 * The first two functions are called upon changing the state, i.e., first
 *  the event `output_switch_state_pre_event` is called, then for each output
 *  that requires a change `outputhandler_set_output_state` is called, and
 *  finally `output_switch_state_post_event` is called.
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
