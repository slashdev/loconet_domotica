/**
 * @file loconet_tx.h
 * @brief Process sending Loconet messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * This file contains the processing of sending loconet messages.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#ifndef _LOCONET_LOCONET_TX_MESSAGES_H_
#define _LOCONET_LOCONET_TX_MESSAGES_H_

#include <stdint.h>
#include <stdbool.h>
#include "loconet_tx.h"

// 2 bytes messages
extern void loconet_tx_busy(void);  // 0x81
extern void loconet_tx_gpoff(void); // 0x82
extern void loconet_tx_gpon(void);  // 0x83
extern void loconet_tx_idle(void);  // 0x85

// 4 bytes messages
extern void loconet_tx_sq_req(uint16_t address, bool dir, bool state); // 0xB0
extern void loconet_tx_sw_rep(uint16_t address, bool state);           // 0xB1
extern void loconet_tx_input_rep(uint16_t address, bool state);        // 0xB2
extern void loconet_tx_long_ack(uint8_t lopc, uint8_t ack1);           // 0xB4

// n bytes messages
extern void loconet_tx_fast_clock(uint8_t clk_rate, uint8_t frac_minsl, uint8_t frac_minsh, uint8_t minutes, uint8_t hours, uint8_t days, uint8_t id1, uint8_t id2); // 0xEF

#endif // _LOCONET_LOCONET_TX_MESSAGES_H_
