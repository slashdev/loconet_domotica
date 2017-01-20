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
// Define LOCONET_RX_RINGBUFFER_Size if it's not defined
#ifndef LOCONET_RX_RINGBUFFER_Size
#define LOCONET_RX_RINGBUFFER_Size 64
#endif

typedef struct {
  uint8_t buffer[LOCONET_RX_RINGBUFFER_Size];
  volatile uint8_t writer;
  volatile uint8_t reader;
} LOCONET_RX_RINGBUFFER_Type;

static LOCONET_RX_RINGBUFFER_Type loconet_rx_ringbuffer = { { 0 }, 0, 0};

//-----------------------------------------------------------------------------
static void loconet_rx_ringbuffer_push(uint8_t byte)
{
  // Get index + 1 of buffer head
  uint8_t index = (loconet_rx_ringbuffer.writer + 1) % LOCONET_RX_RINGBUFFER_Size;

  // If the buffer is full, wait until the reader empties
  // a slot in the buffer to write to.
  while (index == loconet_rx_ringbuffer.reader) {
    continue;
  }

  // Write the byte
  loconet_rx_ringbuffer.buffer[loconet_rx_ringbuffer.writer] = byte;
  loconet_rx_ringbuffer.writer = index;
}

//-----------------------------------------------------------------------------
typedef union {
  struct {
    uint8_t IDLE:1;
    uint8_t TRANSMIT:1;
    uint8_t COLLISION_DETECTED:1;
    uint8_t :5;
  } bit;
  uint8_t reg;
} LOCONET_STATUS_Type;

#define LOCONET_STATUS_IDLE_Pos 0
#define LOCONET_STATUS_IDLE (0x01ul << LOCONET_STATUS_IDLE_Pos)
#define LOCONET_STATUS_TRANSMIT_Pos 1
#define LOCONET_STATUS_TRANSMIT (0x01ul << LOCONET_STATUS_TRANSMIT_Pos)
#define LOCONET_STATUS_COLLISION_DETECT_Pos 2
#define LOCONET_STATUS_COLLISION_DETECT (0x01ul << LOCONET_STATUS_COLLISION_DETECT_Pos)

static LOCONET_STATUS_Type loconet_status = { 0 };

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
      loconet_rx_ringbuffer_push(loconet_sercom->USART.DATA.reg);
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
void ln_handler_dummy_0(void);
void ln_handler_dummy_2(uint8_t, uint8_t);
void ln_handler_dummy_4(uint8_t, uint8_t, uint8_t, uint8_t);
void ln_handler_dummy_n(uint8_t*, uint8_t);

//-----------------------------------------------------------------------------
#define LN_DUMMY_0(name) \
  __attribute__ ((weak, alias ("ln_handler_dummy_0"))) \
  void ln_handler_##name(void)
#define LN_DUMMY_2(name) \
  __attribute__ ((weak, alias ("ln_handler_dummy_2"))) \
  void ln_handler_##name(uint8_t, uint8_t)
#define LN_DUMMY_4(name) \
  __attribute__ ((weak, alias ("ln_handler_dummy_4"))) \
  void ln_handler_##name(uint8_t, uint8_t, uint8_t, uint8_t)
#define LN_DUMMY_N(name) \
  __attribute__ ((weak, alias ("ln_handler_dummy_n"))) \
  void ln_handler_##name(uint8_t*, uint8_t)

//-----------------------------------------------------------------------------
LN_DUMMY_0(busy);
LN_DUMMY_0(gpoff);
LN_DUMMY_0(gpon);
LN_DUMMY_0(idle);

//-----------------------------------------------------------------------------
LN_DUMMY_2(loco_spd);
LN_DUMMY_2(loco_dirf);
LN_DUMMY_2(loco_snd);
LN_DUMMY_2(sq_req);
LN_DUMMY_2(sw_rep);
LN_DUMMY_2(input_rep);
LN_DUMMY_2(long_ack);
LN_DUMMY_2(slot_stat1);
LN_DUMMY_2(consist_func);
LN_DUMMY_2(unlink_slots);
LN_DUMMY_2(link_slots);
LN_DUMMY_2(move_slots);
LN_DUMMY_2(rq_sl_data);
LN_DUMMY_2(sw_state);
LN_DUMMY_2(sw_ack);
LN_DUMMY_2(loco_adr);

//-----------------------------------------------------------------------------
LN_DUMMY_N(wr_sl_data);
LN_DUMMY_N(rd_sl_data);
LN_DUMMY_N(prog_task_start);
LN_DUMMY_N(prog_task_final);
LN_DUMMY_N(fast_clock);

//-----------------------------------------------------------------------------
// Special handlers which cannot be overriden
static void ln_handler_wr_sl_data_(uint8_t*, uint8_t);
static void ln_handler_rd_sl_data_(uint8_t*, uint8_t);

//-----------------------------------------------------------------------------
void (* const ln_messages_0[32])(void) = {
  ln_handler_dummy_0,     // 0x80
  ln_handler_busy,        // 0x81
  ln_handler_gpoff,       // 0x82
  ln_handler_gpon,        // 0x83
  ln_handler_dummy_0,     // 0x84
  ln_handler_idle,        // 0x85
  ln_handler_dummy_0,     // 0x86
  ln_handler_dummy_0,     // 0x87
  ln_handler_dummy_0,     // 0x88
  ln_handler_dummy_0,     // 0x89
  ln_handler_dummy_0,     // 0x8A
  ln_handler_dummy_0,     // 0x8B
  ln_handler_dummy_0,     // 0x8C
  ln_handler_dummy_0,     // 0x8D
  ln_handler_dummy_0,     // 0x8E
  ln_handler_dummy_0,     // 0x8F
  ln_handler_dummy_0,     // 0x90
  ln_handler_dummy_0,     // 0x91
  ln_handler_dummy_0,     // 0x92
  ln_handler_dummy_0,     // 0x93
  ln_handler_dummy_0,     // 0x94
  ln_handler_dummy_0,     // 0x95
  ln_handler_dummy_0,     // 0x96
  ln_handler_dummy_0,     // 0x97
  ln_handler_dummy_0,     // 0x98
  ln_handler_dummy_0,     // 0x99
  ln_handler_dummy_0,     // 0x9A
  ln_handler_dummy_0,     // 0x9B
  ln_handler_dummy_0,     // 0x9C
  ln_handler_dummy_0,     // 0x9D
  ln_handler_dummy_0,     // 0x9E
  ln_handler_dummy_0,     // 0x9F
};

//-----------------------------------------------------------------------------
void (* const ln_messages_2[32])(uint8_t, uint8_t) = {
  ln_handler_loco_spd,    // 0xA0
  ln_handler_loco_dirf,   // 0xA1
  ln_handler_loco_snd,    // 0xA2
  ln_handler_dummy_2,     // 0xA3
  ln_handler_dummy_2,     // 0xA4
  ln_handler_dummy_2,     // 0xA5
  ln_handler_dummy_2,     // 0xA6
  ln_handler_dummy_2,     // 0xA7
  ln_handler_dummy_2,     // 0xA8
  ln_handler_dummy_2,     // 0xA9
  ln_handler_dummy_2,     // 0xAA
  ln_handler_dummy_2,     // 0xAB
  ln_handler_dummy_2,     // 0xAC
  ln_handler_dummy_2,     // 0xAD
  ln_handler_dummy_2,     // 0xAE
  ln_handler_dummy_2,     // 0xAF
  ln_handler_sq_req,      // 0xB0
  ln_handler_sw_rep,      // 0xB1
  ln_handler_input_rep,   // 0xB2
  ln_handler_dummy_2,     // 0xB3
  ln_handler_long_ack,    // 0xB4
  ln_handler_slot_stat1,  // 0xB5
  ln_handler_consist_func,// 0xB6
  ln_handler_dummy_2,     // 0xB7
  ln_handler_unlink_slots,// 0xB8
  ln_handler_link_slots,  // 0xB9
  ln_handler_move_slots,  // 0xBA
  ln_handler_rq_sl_data,  // 0xBB
  ln_handler_sw_state,    // 0xBC
  ln_handler_sw_ack,      // 0xBD
  ln_handler_dummy_2,     // 0xBE
  ln_handler_loco_adr,    // 0xBF
};

//-----------------------------------------------------------------------------
void (* const ln_messages_4[32])(uint8_t, uint8_t, uint8_t, uint8_t) = {
  ln_handler_dummy_4,     // 0xC0
  ln_handler_dummy_4,     // 0xC1
  ln_handler_dummy_4,     // 0xC2
  ln_handler_dummy_4,     // 0xC3
  ln_handler_dummy_4,     // 0xC4
  ln_handler_dummy_4,     // 0xC5
  ln_handler_dummy_4,     // 0xC6
  ln_handler_dummy_4,     // 0xC7
  ln_handler_dummy_4,     // 0xC8
  ln_handler_dummy_4,     // 0xC9
  ln_handler_dummy_4,     // 0xCA
  ln_handler_dummy_4,     // 0xCB
  ln_handler_dummy_4,     // 0xCC
  ln_handler_dummy_4,     // 0xCD
  ln_handler_dummy_4,     // 0xCE
  ln_handler_dummy_4,     // 0xCF
  ln_handler_dummy_4,     // 0xD0
  ln_handler_dummy_4,     // 0xD1
  ln_handler_dummy_4,     // 0xD2
  ln_handler_dummy_4,     // 0xD3
  ln_handler_dummy_4,     // 0xD4
  ln_handler_dummy_4,     // 0xD5
  ln_handler_dummy_4,     // 0xD6
  ln_handler_dummy_4,     // 0xD7
  ln_handler_dummy_4,     // 0xD8
  ln_handler_dummy_4,     // 0xD9
  ln_handler_dummy_4,     // 0xDA
  ln_handler_dummy_4,     // 0xDB
  ln_handler_dummy_4,     // 0xDC
  ln_handler_dummy_4,     // 0xDD
  ln_handler_dummy_4,     // 0xDE
  ln_handler_dummy_4,     // 0xDF
};

//-----------------------------------------------------------------------------
void (* const ln_messages_n[32])(uint8_t*, uint8_t) = {
  ln_handler_dummy_n,     // 0xE0
  ln_handler_dummy_n,     // 0xE1
  ln_handler_dummy_n,     // 0xE2
  ln_handler_dummy_n,     // 0xE3
  ln_handler_dummy_n,     // 0xE4
  ln_handler_dummy_n,     // 0xE5
  ln_handler_dummy_n,     // 0xE6
  ln_handler_rd_sl_data_, // 0xE7
  ln_handler_dummy_n,     // 0xE8
  ln_handler_dummy_n,     // 0xE9
  ln_handler_dummy_n,     // 0xEA
  ln_handler_dummy_n,     // 0xEB
  ln_handler_dummy_n,     // 0xEC
  ln_handler_dummy_n,     // 0xED
  ln_handler_dummy_n,     // 0xEE
  ln_handler_wr_sl_data_, // 0xEF
  ln_handler_dummy_n,     // 0xF0
  ln_handler_dummy_n,     // 0xF1
  ln_handler_dummy_n,     // 0xF2
  ln_handler_dummy_n,     // 0xF3
  ln_handler_dummy_n,     // 0xF4
  ln_handler_dummy_n,     // 0xF5
  ln_handler_dummy_n,     // 0xF6
  ln_handler_dummy_n,     // 0xF7
  ln_handler_dummy_n,     // 0xF8
  ln_handler_dummy_n,     // 0xF9
  ln_handler_dummy_n,     // 0xFA
  ln_handler_dummy_n,     // 0xFB
  ln_handler_dummy_n,     // 0xFC
  ln_handler_dummy_n,     // 0xFD
  ln_handler_dummy_n,     // 0xFE
  ln_handler_dummy_n,     // 0xFF
};

//-----------------------------------------------------------------------------
// Read slot data (SL_RD_DATA)
// Handle special cases
static void ln_handler_rd_sl_data_(uint8_t *data, uint8_t length) {
  if (data[0] == 0x7C) { // Program task final
    ln_handler_prog_task_final(&data[1], length - 1);
  } else { // Default handler
    ln_handler_rd_sl_data(data, length);
  }
}

//-----------------------------------------------------------------------------
// Write slot data (RW_SL_DATA)
// Handle special cases
static void ln_handler_wr_sl_data_(uint8_t *data, uint8_t length) {
  if (data[0] == 0x7B) { // Fast clock
    ln_handler_fast_clock(&data[1], length - 1);
  } else if (data[0] == 0x7C) { // Program task start
    ln_handler_prog_task_start(&data[1], length - 1);
  } else { // Default handler
    ln_handler_wr_sl_data(data, length);
  }
}

//-----------------------------------------------------------------------------
// Calculate the checksum of a message
static uint8_t loconet_calc_checksum(uint8_t *data, uint8_t length)
{
  uint8_t checksum = 0xFF;
  while (length--) {
    // Dereference data pointer, XOR it with checksum and advance pointer by 1
    checksum ^= *data++;
  }
  return checksum;
}

//-----------------------------------------------------------------------------
typedef union {
  struct {
    uint8_t NUMBER:5;
    uint8_t OPCODE:3;
  } bits;
  uint8_t byte;
} LOCONET_OPCODE_BYTE_Type;

#define LOCONET_OPCODE_FLAG_Pos 7
#define LOCONET_OPCODE_FLAG (0x01ul << LOCONET_OPCODE_FLAG_Pos)

//-----------------------------------------------------------------------------
static uint8_t loconet_rx_process(void)
{
  // Get values from ringbuffer
  uint8_t *buffer = loconet_rx_ringbuffer.buffer;
  uint8_t reader = loconet_rx_ringbuffer.reader;
  uint8_t writer = loconet_rx_ringbuffer.writer;

  // Expect an opcode byte from ringbuffer
  LOCONET_OPCODE_BYTE_Type opcode;
  opcode.byte = buffer[reader];

  // If the reader is "ahead" of the writer in the ringbuffer,
  // add the size of the ringbuffer to the writer for its
  // fictive index.
  if (reader > writer) {
    writer += LOCONET_RX_RINGBUFFER_Size;
  }

  if (writer <= reader + 1) {
    // We want at least two bytes before we try to read the message
    return 0;
  }

  // If it's not an OPCODE byte, skip it
  if (!(opcode.byte & LOCONET_OPCODE_FLAG)) {
    loconet_rx_ringbuffer.reader = (reader + 1) % LOCONET_RX_RINGBUFFER_Size;
    return 0;
  }

  // New message
  uint8_t message_size = 0;
  switch (opcode.bits.OPCODE) {
    case 0x04:
      message_size = 2;
      break;
    case 0x05:
      message_size = 4;
      break;
    case 0x06:
      message_size = 6;
      break;
    case 0x07:
      message_size = buffer[(reader + 1) % LOCONET_RX_RINGBUFFER_Size];
      break;
  }

  // Check if the buffer contains no new opcodes (could happen due to colissions)
  uint8_t index_of_writer_or_eom = writer < (reader + message_size) ? writer : reader + message_size;
  for (uint8_t index = reader + 1; index < index_of_writer_or_eom; index++) {
    if (buffer[index % LOCONET_RX_RINGBUFFER_Size] & LOCONET_OPCODE_FLAG) {
      loconet_rx_ringbuffer.reader = index % LOCONET_RX_RINGBUFFER_Size;
      return 1; // Read the new message right away
    }
  }

  // Check if we have all the bytes for this message
  if (writer < reader + message_size) {
    return 0;
  }

  // Get bytes for passing (and build checksum)
  uint8_t data[message_size - 2];
  uint8_t start = reader + 1;

  for (uint8_t index = 0; index < message_size - 2; index++) {
    data[index] = buffer[(start + index) % LOCONET_RX_RINGBUFFER_Size];
  }

  // Verify checksum (skip message if failed)
  if (loconet_calc_checksum(data, message_size)) {
    loconet_rx_ringbuffer.reader = (reader + message_size) % LOCONET_RX_RINGBUFFER_Size;
    return 0;
  }

  // Handle message
  switch(opcode.bits.OPCODE) {
    case 0x04: // Length 0
      (*ln_messages_0[opcode.bits.NUMBER])();
      break;
    case 0x05: // Length 2
      (*ln_messages_2[opcode.bits.NUMBER])(data[0], data[1]);
      break;
    case 0x06: // Length 4
      (*ln_messages_4[opcode.bits.NUMBER])(data[0], data[1], data[2], data[3]);
      break;
    case 0x07: // Variable length
      (*ln_messages_n[opcode.bits.NUMBER])(&data[1], message_size - 3);
      break;
  }

  // Advance reader
  loconet_rx_ringbuffer.reader = (reader + message_size) % LOCONET_RX_RINGBUFFER_Size;

  // Return that we have processed a message
  return 1;
}

//-----------------------------------------------------------------------------
// Dummy handlers
void ln_handler_dummy_0(void)
{
}

void ln_handler_dummy_2(uint8_t a, uint8_t b)
{
  (void)a;
  (void)b;
}

void ln_handler_dummy_4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  (void)a;
  (void)b;
  (void)c;
  (void)d;
}

void ln_handler_dummy_n(uint8_t *d, uint8_t l)
{
  (void)d;
  (void)l;
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
// Start transmitting bytes
static void loconet_tx_start(void)
{
  // Set data in register
  loconet_sercom->USART.DATA.reg = loconet_tx_current->data[0];
  // Enable Data Register Empty interrupt
  loconet_sercom->USART.INTENSET.reg = SERCOM_USART_INTENSET_DRE;
  // Increment of tx index
  loconet_tx_current->tx_index++;
}

//-----------------------------------------------------------------------------
static uint8_t loconet_tx_process(void)
{
  // Can we start transmission?
  if (!loconet_tx_queue) {
    // No message is in the queue
    return 0;
  } else if (loconet_status.bit.COLLISION_DETECTED) {
    return 0;
  } else if (!loconet_status.bit.IDLE) {
    // We're not allowed to transmit, don't try to
    return 0;
  } else if (loconet_status.bit.TRANSMIT) {
    // Do not start transmission if we're already sending
    return 0;
  }

  // We have a queue, loconet is idle, so we can start sending
  loconet_status.reg |= LOCONET_STATUS_TRANSMIT;

  // Set which bytes need to be send
  loconet_tx_current = loconet_tx_queue;
  loconet_tx_queue = loconet_tx_current->next;
  loconet_tx_current->next = 0;

  // Start sending
  loconet_tx_start();

  return 0;
}

//-----------------------------------------------------------------------------
// Should be included in the main loop to keep loconet going.
void loconet_loop(void)
{
  // If a message is received and handled, keep processing new messages
  while(loconet_rx_process());
  // If a message is sent, keep sending messages
  while(loconet_tx_process());
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
