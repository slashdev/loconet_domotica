/*
 * @file domotica_rx.h
 * @brief Loconet Domotica Module - receiving messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 */

// ------------------------------------------------------------------

#ifndef DOMOTICA_RX_INPUT_ADDRESS_SIZE
  #define DOMOTICA_RX_INPUT_ADDRESS_SIZE 6
#endif

#ifndef _DOMOTICA_RX_H_
#define _DOMOTICA_RX_H_

#include <stdbool.h>
#include <stdint.h>
#include "domotica.h"
#include "loconet/loconet.h"
#include "loconet/loconet_cv.h"
#include "loconet/loconet_rx.h"

// ------------------------------------------------------------------
// Initialize the RX library
extern void domotica_rx_init(void);

// ------------------------------------------------------------------
// Add a B2 address to listen to.
extern void domotica_rx_set_input_address(uint8_t lncv, uint16_t address);

// ------------------------------------------------------------------
// Remove a B2 address to listen to.
void domotica_rx_remove_input_address(uint8_t lncv);

// ------------------------------------------------------------------
// Listen to sensor messages sent by other devices
extern void loconet_rx_input_rep(uint8_t, uint8_t);

// ------------------------------------------------------------------
// Listen to switch requests. We switch if the address matches
// loconet_config.bit.ADDRESS + index, with 0 <= index < 16. Depending on the
// ON bit of the B0 message, we decide whether the output should be switched
// on or off
extern void loconet_rx_sw_req(uint8_t sw1, uint8_t sw2);

#endif // _DOMOTICA_RX_H_
