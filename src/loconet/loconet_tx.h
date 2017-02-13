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

#ifndef _LOCONET_LOCONET_TX_H_
#define _LOCONET_LOCONET_TX_H_

#include <stdint.h>
#include "loconet.h"

//-----------------------------------------------------------------------------
// Stop sending
extern void loconet_tx_stop(void);

//-----------------------------------------------------------------------------
// Reset indexes of the message and place it at the front of the queue
extern void loconet_tx_reset_current_message_to_queue(void);

//-----------------------------------------------------------------------------
// Give the next byte we expect on the RX line
extern uint8_t loconet_tx_next_rx_byte(void);

//-----------------------------------------------------------------------------
// Give the next byte we want to send
extern uint8_t loconet_tx_next_tx_byte(void);

//-----------------------------------------------------------------------------
// Are we done sending a message
extern uint8_t loconet_tx_finished(void);

//-----------------------------------------------------------------------------
// Process sending of messages
extern void loconet_tx_process(void);

//-----------------------------------------------------------------------------
// Size of queue
extern uint16_t loconet_tx_queue_size(void);

//-----------------------------------------------------------------------------
// Enqueue a message
extern void loconet_tx_queue_2(uint8_t opcode, uint8_t priority);
extern void loconet_tx_queue_4(uint8_t opcode, uint8_t priority, uint8_t  a, uint8_t b);
extern void loconet_tx_queue_6(uint8_t opcode, uint8_t priority, uint8_t  a, uint8_t b, uint8_t c, uint8_t d);
extern void loconet_tx_queue_n(uint8_t opcode, uint8_t priority, uint8_t *d, uint8_t l);

#endif // _LOCONET_LOCONET_TX_H_
