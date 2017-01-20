/**
 * @file loconet.c
 * @brief Loconet base functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */
#include "loconet.h"

//-----------------------------------------------------------------------------
// Prototypes
void loconet_tx_stop(void);

//-----------------------------------------------------------------------------
// Peripherals to use for communication
Sercom *loconet_sercom;
Tc *loconet_flank_timer;
PortGroup *loconet_tx_port;
uint32_t loconet_tx_pin;

//-----------------------------------------------------------------------------
// Global variables
LOCONET_CONFIG_Type loconet_config = { 0 };
LOCONET_STATUS_Type loconet_status = { 0 };

//-----------------------------------------------------------------------------
// Loconet message/linked list definition
typedef struct MESSAGE {
  // Control fields
  uint8_t priority;
  struct MESSAGE *next;
  // Message fields
  uint8_t *data;
  uint8_t data_length;
  // Current index we're sending
  uint8_t tx_index;
  uint8_t rx_index;
} LOCONET_MESSAGE_Type;

static LOCONET_MESSAGE_Type *loconet_tx_queue = 0;
static LOCONET_MESSAGE_Type *loconet_tx_current = 0;

//-----------------------------------------------------------------------------
// Initialize USART for loconet
void loconet_init_usart(Sercom *sercom, uint32_t pm_mask, uint32_t gclock_id, uint8_t rx_pad, uint32_t nvic_irqn)
{
  // Save sercom
  loconet_sercom = sercom;

  // Enable clock for peripheral, without prescaler
  PM->APBCMASK.reg |= pm_mask;
  GCLK->CLKCTRL.reg =
    GCLK_CLKCTRL_ID(gclock_id)
    | GCLK_CLKCTRL_CLKEN
    | GCLK_CLKCTRL_GEN(0);

  /* CRTLA register:
   *   DORD:      0x01  LSB first
   *   CPOL:      0x00  Tx: rising, Rx: falling
   *   CMODE:     0x00  Async
   *   FORM:      0x00  USART Frame (without parity)
   *   RXPO:      0x03  Rx on PA15
   *   TXPO:      0x00  Tx on PA14
   *   IBON:      0x00  Ignored
   *   RUNSTDBY:  0x00  Ignored
   *   MODE:      0x01  USART with internal clock
   *   ENABLE:    0x01  Enabled (set at the end of the init)
   */
  loconet_sercom->USART.CTRLA.reg =
    SERCOM_USART_CTRLA_DORD
    | SERCOM_USART_CTRLA_MODE_USART_INT_CLK
    | SERCOM_USART_CTRLA_RXPO(rx_pad)
    | SERCOM_USART_CTRLA_TXPO;

  /* CTRLB register:
   *   RXEN:      0x01  Enable Rx
   *   TXEN:      0x00  Only enable Tx when sending
   *   PMODE:     0x00  Ignored, parity is not used
   *   SFDE:      0x00  Ignored, chip does not go into standby
   *   SBMODE:    0x00  One stop bit
   *   CHSIZE:    0x00  Char size: 8 bits
   */
  loconet_sercom->USART.CTRLB.reg =
    SERCOM_USART_CTRLB_RXEN
    | SERCOM_USART_CTRLB_TXEN
    | SERCOM_USART_CTRLB_CHSIZE(0);

  uint64_t br = (uint64_t)65536 * (F_CPU - 16 * 16666) / F_CPU;
  loconet_sercom->USART.BAUD.reg = (uint16_t)br;

  /* INTERRUPTS register
   *   RXS:       0x00  No interrupt on Rx start
   *   RXC:       0x01  Interrupt on Rx complete
   *   TXC:       0x01  Interrupt on Tx complete
   *   DRE:       0x00  No interrupt on data registry empty
   */
  loconet_sercom->USART.INTENSET.reg =
    SERCOM_USART_INTENSET_RXC
    | SERCOM_USART_INTENSET_TXC;
  NVIC_EnableIRQ(nvic_irqn);

  // Enable USART
  loconet_sercom->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
}

//-----------------------------------------------------------------------------
// Initialize flank detection
void loconet_init_flank_detection(uint8_t fl_int)
{
  // Enable clock for external interrupts, without prescaler
  PM->APBAMASK.reg |= PM_APBAMASK_EIC;
  GCLK->CLKCTRL.reg =
    GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_EIC)
    | GCLK_CLKCTRL_CLKEN
    | GCLK_CLKCTRL_GEN(0);

  // Enable interrupt for external pin
  EIC->INTENSET.reg |= EIC_EVCTRL_EXTINTEO(0x01ul << fl_int);
  EIC->CONFIG[fl_int / 8].reg = EIC_CONFIG_SENSE0_BOTH << 4 * (fl_int % 8);
  NVIC_EnableIRQ(EIC_IRQn);

  // Enable external interrupts
  EIC->CTRL.reg |= EIC_CTRL_ENABLE;
}

//-----------------------------------------------------------------------------
// Initialize flank timer
void loconet_init_flank_timer(Tc *timer, uint32_t pm_tmr_mask, uint32_t gclock_tmr_id, uint32_t nvic_irqn)
{
  // Save timer
  loconet_flank_timer = timer;

  // Enable clock for flank timer, without prescaler
  PM->APBCMASK.reg |= pm_tmr_mask;
  GCLK->CLKCTRL.reg =
    GCLK_CLKCTRL_ID(gclock_tmr_id)
    | GCLK_CLKCTRL_CLKEN
    | GCLK_CLKCTRL_GEN(0);

  /* CTRLA register:
   *   PRESCSYNC: 0x02  RESYNC
   *   RUNSTDBY:        Ignored
   *   PRESCALER: 0x03  DIV8, each tick will be 1us
   *   WAVEGEN:   0x01  MFRQ, zero counter on match
   *   MODE:      0x00  16 bits timer
   */
  loconet_flank_timer->COUNT16.CTRLA.reg =
    TC_CTRLA_PRESCSYNC_RESYNC
    | TC_CTRLA_PRESCALER_DIV8
    | TC_CTRLA_WAVEGEN_MFRQ
    | TC_CTRLA_MODE_COUNT16;

  /* INTERRUPTS:
   *   Interrupt on match
   */
  loconet_flank_timer->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);
  NVIC_EnableIRQ(nvic_irqn);
}

//-----------------------------------------------------------------------------
// Save which pin is connected to TX
void loconet_save_tx_pin(PortGroup *group, uint32_t pin)
{
  loconet_tx_port = group;
  loconet_tx_pin = (0x01ul << pin);
}

//-----------------------------------------------------------------------------
typedef union {
  struct {
    uint8_t CARRIER_DETECT:1;
    uint8_t MASTER_DELAY:1;
    uint8_t LINE_BREAK:1;
    uint8_t PRIORITY_DELAY:1;
    uint8_t :4;
  } bit;
  uint8_t reg;
} LOCONET_TIMER_STATUS_Type;

#define LOCONET_TIMER_STATUS_CARRIER_DETECT_Pos 0
#define LOCONET_TIMER_STATUS_CARRIER_DETECT (0x01ul << LOCONET_TIMER_STATUS_CARRIER_DETECT_Pos)
#define LOCONET_TIMER_STATUS_MASTER_DELAY_Pos 1
#define LOCONET_TIMER_STATUS_MASTER_DELAY (0x01ul << LOCONET_TIMER_STATUS_MASTER_DELAY_Pos)
#define LOCONET_TIMER_STATUS_LINE_BREAK_Pos 2
#define LOCONET_TIMER_STATUS_LINE_BREAK (0x01ul << LOCONET_TIMER_STATUS_LINE_BREAK_Pos)
#define LOCONET_TIMER_STATUS_PRIORITY_DELAY_Pos 3
#define LOCONET_TIMER_STATUS_PRIORITY_DELAY (0x01ul << LOCONET_TIMER_STATUS_PRIORITY_DELAY_Pos)

#define LOCONET_DELAY_CARRIER_DETECT 1200 /* 20x bit time (60ux) */
#define LOCONET_DELAY_MASTER_DELAY    360 /*  6x bit time (60us) */
#define LOCONET_DELAY_LINE_BREAK      900 /* 15x bit time (60us) */
#define LOCONET_DELAY_PRIORITY_DELAY   60 /*  1x bit time (60ux) */

static LOCONET_TIMER_STATUS_Type loconet_timer_status = { 0 };

//-----------------------------------------------------------------------------
static void loconet_flank_timer_delay(uint16_t delay_us) {
  // Set timer counter to 0
  loconet_flank_timer->COUNT16.COUNT.reg = 0;
  // Set timer match, 1200us
  loconet_flank_timer->COUNT16.CC[0].reg = delay_us;
  // Enable timer
  loconet_flank_timer->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
}

//-----------------------------------------------------------------------------
void loconet_irq_flank_rise(void) {
  loconet_flank_timer_delay(LOCONET_DELAY_CARRIER_DETECT);
  loconet_timer_status.reg = LOCONET_TIMER_STATUS_CARRIER_DETECT;
  // If flank changes, loconet is not idle anymore
  loconet_status.bit.IDLE = 0;
}

//-----------------------------------------------------------------------------
void loconet_irq_flank_fall(void) {
  loconet_flank_timer_delay(LOCONET_DELAY_LINE_BREAK);
  loconet_timer_status.reg = LOCONET_TIMER_STATUS_LINE_BREAK;
  // If flank changes, loconet is not idle anymore
  loconet_status.bit.IDLE = 0;
}

//-----------------------------------------------------------------------------
void loconet_irq_timer(void) {
  // Carrier detect?
  if (loconet_timer_status.bit.CARRIER_DETECT) {
    if (loconet_config.bit.MASTER) {
      // Master, set as idle directly
      loconet_status.reg |= LOCONET_STATUS_IDLE;
    } else {
      // Start master delay
      loconet_flank_timer_delay(LOCONET_DELAY_MASTER_DELAY);
      loconet_timer_status.reg = LOCONET_TIMER_STATUS_MASTER_DELAY;
    }
  } else if (loconet_timer_status.bit.MASTER_DELAY) {
    if (loconet_config.bit.PRIORITY) {
      // Start priority delay
      loconet_flank_timer_delay(loconet_config.bit.PRIORITY * LOCONET_DELAY_PRIORITY_DELAY);
      loconet_timer_status.reg = LOCONET_TIMER_STATUS_PRIORITY_DELAY;
    } else {
      loconet_status.reg |= LOCONET_STATUS_IDLE;
    }
  } else if (loconet_timer_status.bit.PRIORITY_DELAY) {
    loconet_status.reg |= LOCONET_STATUS_IDLE;
  } else if (loconet_timer_status.bit.LINE_BREAK) {
    // Remove collision detected flag
    loconet_status.bit.COLLISION_DETECTED = 0;
    // Release TX pin
    loconet_tx_port->OUTCLR.reg |= loconet_tx_pin;
    // Enable receiving and sending
    loconet_sercom->USART.CTRLB.reg |= SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
  }
}

static void loconet_irq_collision(void)
{
  // Set collision detected flag
  loconet_status.bit.COLLISION_DETECTED = 1;
  // Stop receiving and sending
  loconet_sercom->USART.CTRLB.bit.RXEN = 0;
  loconet_sercom->USART.CTRLB.bit.TXEN = 0;
  // If we were transmitting, enforce line break
  if (loconet_status.bit.TRANSMIT) {
    // Disable TRANSMIT
    loconet_status.bit.TRANSMIT = 0;
    // Pull Tx pin low
    loconet_tx_port->OUTSET.reg |= loconet_tx_pin;
    // Reset transmit and receive index
    loconet_tx_current->tx_index = 0;
    loconet_tx_current->rx_index = 0;
    // Place message back at front of queue
    loconet_tx_current->next = loconet_tx_current;
    loconet_tx_queue = loconet_tx_current;
    loconet_tx_current = 0;
  }
}

//-----------------------------------------------------------------------------
// Handle sercom (usart) interrupt
void loconet_irq_sercom(void)
{
  // Rx complete
  if (loconet_sercom->USART.INTFLAG.bit.RXC) {
    if (loconet_status.bit.COLLISION_DETECTED) {
      // Ignore byte
      loconet_sercom->USART.DATA.reg;
      // Make sure Framing error status is cleared
      loconet_sercom->USART.STATUS.reg |= SERCOM_USART_STATUS_FERR;
    } else if (loconet_sercom->USART.STATUS.bit.FERR) {
      // Reset flag
      loconet_sercom->USART.STATUS.reg |= SERCOM_USART_STATUS_FERR;
      // Framing error -> Collision detected
      loconet_irq_collision();
    } else if (loconet_status.bit.TRANSMIT) {
      // Read own bytes to see if we have a collision
      uint8_t data = loconet_sercom->USART.DATA.reg;
      if (loconet_tx_current && data != loconet_tx_current->data[loconet_tx_current->rx_index++]) {
        loconet_irq_collision();
      }
    } else {
      // Get data from USART and place it in the ringbuffer
      loconet_rx_buffer_push(loconet_sercom->USART.DATA.reg);
    }
  }

  // Tx complete
  if (loconet_sercom->USART.INTFLAG.bit.TXC) {
    // Clear TXC flag
    loconet_sercom->USART.INTFLAG.reg |= SERCOM_USART_INTFLAG_TXC;
    // Clear transmit state and free memory
    loconet_tx_stop();
  }

  // Data register empty (TX)
  if (loconet_sercom->USART.INTFLAG.bit.DRE) {
    // Is a collision detected? Or is our message gone AWOL (due to a collision)?
    // If so: do not attempt to buffer bytes to send
    if (loconet_status.bit.COLLISION_DETECTED || !loconet_tx_current) {
      // Disable TRANSMIT
      loconet_status.bit.TRANSMIT = 0;
      // Disable Data Register Empty interrupt
      loconet_sercom->USART.INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
    } else if (loconet_status.bit.TRANSMIT) {
      // Do we have a message and do we have another byte to send?
      if (loconet_tx_current && loconet_tx_current->tx_index < loconet_tx_current->data_length) {
        loconet_sercom->USART.DATA.reg = loconet_tx_current->data[loconet_tx_current->tx_index];
        loconet_tx_current->tx_index++;
      } else {
        // Disable TRANSMIT
        loconet_status.bit.TRANSMIT = 0;
        // Disable Data Register Empty interrupt
        loconet_sercom->USART.INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
      }
    }
  }
}

//-----------------------------------------------------------------------------
// Calculate the checksum of a message
uint8_t loconet_calc_checksum(uint8_t *data, uint8_t length)
{
  uint8_t checksum = 0xFF;
  while (length--) {
    // Dereference data pointer, XOR it with checksum and advance pointer by 1
    checksum ^= *data++;
  }
  return checksum;
}

//-----------------------------------------------------------------------------
// Stop transmission and free memory of the message
void loconet_tx_stop(void)
{
  loconet_status.bit.TRANSMIT = 0;
  // We might not have a message due to collision detection
  if (loconet_tx_current) {
    free(loconet_tx_current->data);
    free(loconet_tx_current);
  }
}

//-----------------------------------------------------------------------------
// Enable data register empty interrupt so we can send data
static void loconet_sercom_enable_dre_irq(void)
{
  loconet_sercom->USART.INTENSET.reg = SERCOM_USART_INTENSET_DRE;
}

//-----------------------------------------------------------------------------
static void loconet_tx_process(void)
{
  // Can we start transmission?
  if (!loconet_tx_queue) {
    // No message is in the queue
    return;
  } else if (loconet_status.bit.COLLISION_DETECTED) {
    return;
  } else if (!loconet_status.bit.IDLE) {
    // We're not allowed to transmit, don't try to
    return;
  } else if (loconet_status.bit.TRANSMIT) {
    // Do not start transmission if we're already sending
    return;
  }

  // We have a queue, loconet is idle, so we can start sending
  loconet_status.reg |= LOCONET_STATUS_TRANSMIT;

  // Set which bytes need to be send
  loconet_tx_current = loconet_tx_queue;
  loconet_tx_queue = loconet_tx_current->next;
  loconet_tx_current->next = 0;

  // Start sending
  loconet_sercom_enable_dre_irq();

  return;
}

//-----------------------------------------------------------------------------
// Should be included in the main loop to keep loconet going.
void loconet_loop(void)
{
  // If a message is received and handled, keep processing new messages
  while(loconet_rx_process());
  // Send a message if there is one available
  loconet_tx_process();
}

//-----------------------------------------------------------------------------
static void loconet_tx_enqueue(LOCONET_MESSAGE_Type *message)
{
  // If queue is empty, push it
  if (!loconet_tx_queue) {
    loconet_tx_queue = message;
    return;
  }

  // Pointers to previous and current node
  LOCONET_MESSAGE_Type *prev = loconet_tx_queue;
  LOCONET_MESSAGE_Type *curr = loconet_tx_queue->next;

  // Loop through message which are more important (lower priority)
  for (; curr && curr->priority < message->priority + 1; prev = curr, curr = curr->next);

  // Loop through messages which have the same priority.
  // All priority should be lowered by 1, and place the message at the end.
  for (; curr && curr->priority < message->priority + 2; curr->priority--, prev = curr, curr = curr->next);
  prev->next = message;
  message->next = curr;

  // All next messages should have their priorities decreased.
  // This prevents starvation of messages at the end of the queue
  for (; curr; curr->priority--, curr = curr->next);
}

//-----------------------------------------------------------------------------
// Build an empty message with the correct length
static LOCONET_MESSAGE_Type *loconet_build_message(uint8_t length)
{
  // Allocate space for linked list node
  LOCONET_MESSAGE_Type *message = malloc(sizeof(LOCONET_MESSAGE_Type));
  memset(message, 0, sizeof(LOCONET_MESSAGE_Type));
  // Allocate space for message bytes
  message->data = malloc(sizeof(uint8_t) * length);
  message->data_length = length;
  // Return the new message
  return message;
}

//-----------------------------------------------------------------------------
void loconet_tx_queue_0(uint8_t opcode, uint8_t priority)
{
  LOCONET_MESSAGE_Type *message = loconet_build_message(2);
  // Set priority
  message->priority = priority;
  // Fill message
  message->data[0] = opcode;
  message->data[1] = loconet_calc_checksum(message->data, 1);
  // Enqueue message
  loconet_tx_enqueue(message);
}

void loconet_tx_queue_2(uint8_t opcode, uint8_t priority, uint8_t  a, uint8_t b)
{
  LOCONET_MESSAGE_Type *message = loconet_build_message(4);
  // Set priority
  message->priority = priority;
  // Fill message
  message->data[0] = opcode;
  message->data[1] = a;
  message->data[2] = b;
  message->data[3] = loconet_calc_checksum(message->data, 3);
  // Enqueue message
  loconet_tx_enqueue(message);
}

void loconet_tx_queue_4(uint8_t opcode, uint8_t priority, uint8_t  a, uint8_t b, uint8_t c, uint8_t d)
{
  LOCONET_MESSAGE_Type *message = loconet_build_message(6);
  // Set priority
  message->priority = priority;
  // Fill message
  message->data[0] = opcode;
  message->data[1] = a;
  message->data[2] = b;
  message->data[3] = c;
  message->data[4] = d;
  message->data[5] = loconet_calc_checksum(message->data, 5);
  // Enqueue message
  loconet_tx_enqueue(message);
}

void loconet_tx_queue_n(uint8_t opcode, uint8_t priority, uint8_t *data, uint8_t length)
{
  LOCONET_MESSAGE_Type *message = loconet_build_message(length + 2);
  // Set priority
  message->priority = priority;
  // Fill message
  message->data[0] = opcode;
  for(uint8_t idx = 0; idx < length; message->data[idx+1] = data[idx], idx++);
  message->data[length+1] = loconet_calc_checksum(message->data, length + 1);
  // Enqueue message
  loconet_tx_enqueue(message);
}
