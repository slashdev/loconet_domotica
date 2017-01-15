/**
 * @file loconet.h
 * @brief Loconet base functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * The loconet receiver can work with any of the SERCOM interfaces.
 * To build `loconet_init` you call `LOCONET_BUILD` with the
 * parameters how you want use the SERCOM interface. The
 * `LOCONET_BUILD` is structured as follows:
 * LOCONET_BUILD(sercom, tx_port, tx_pin, rx_port, rx_pin, rx_pad)
 * - sercom:  the SERCOM interface number you'd like to use (e.g. 3)
 * - tx_port: the PORT of the TX output (e.g. A)
 * - tx_pin:  the PIN of the TX output (e.g. 14)
 * - rx_port: the PORT of the RX input (e.g. A)
 * - rx_pin:  the PIN of the RX input (e.g. 15)
 * - rx_pad:  the PAD to use for RX input on the SERCOM interface
 *     (e.g. 1, see datasheet)
 * - fl_port: the PORT of the FLANK detection (e.g. A)
 * - fl_pin:  the PIN of the FLANK detection (e.g. 13)
 * - fl_int:  the external interrupt associated to fl_pin
 *     (e.g. 1, see datasheet)
 * - fl_tmr:  the TIMER used for Carrier and Break detection
 *
 * Loconet uses CSMA/CD techniques to arbitrage and control network
 * access. The bit times are 60 uSecs or 16.66 KBaud +/- 1.5%. This
 * means that a Baudrate of 16.457 can be used as well. The data is
 * sent asynchonous using 1 start bit, 8 data bits and 1 stop bit.
 * The data is sent LSB first. Bytes can be sent back-to-back.
 *
 * Flank detection is used to start the delays for carrier detect
 * and break detect. The current code assumes the chip to run with
 * F_CPU 8 MHz.
 *
 * Before you can use the logger functions, initialize the logger
 * using `logger_init(baudrate);`.
 *
 * To process incoming messages from loconet, place
 * `process_loconet_rx_ringbuffer` in your main loop, e.g.:
 *
 *   while (1) {
 *     process_loconet_rx_ringbuffer();
 *   }
 *
 * To process the flank detection we need to use the external
 * interrups. But we don't want to claim all external interrupt
 * handling for loconet, so you'll have to catch the interrupt
 * and call our handler. If our handler returns 1, it has handled
 * the interrupt and you could skip the rest of your handling, e.g.:
 *
 *   void irq_handler_eic(void);
 *   void irq_handler_eic(void) {
 *     if (loconet_handle_eic()) {
 *       return;
 *     }
 *   }
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 */

#ifndef _LOCONET_LOCONET_H_
#define _LOCONET_LOCONET_H_

#include <stdint.h>
#include "samd20.h"

//-----------------------------------------------------------------------------
// Give a warning if F_CPU is not 8MHz
#if F_CPU != 8000000
#warn "F_CPU is not 8000000, CD and BREAK timer wont work as expected!"
#endif

//-----------------------------------------------------------------------------
// Initializations
extern void loconet_init(void);
extern void loconet_init_usart(Sercom*, uint32_t, uint32_t, uint8_t, uint32_t);

//-----------------------------------------------------------------------------
// IRQs for flank rise / fall
extern void loconet_irq_flank_rise(void);
extern void loconet_irq_flank_fall(void);
// IRQ for timeout of timer
extern void loconet_irq_timer(void);

extern void loconet_rx_ringbuffer_push(uint8_t byte);
extern void loconet_start_timer_delay(uint16_t delay_us);
extern void loconet_handle_eic(void);

// Marco for loconet_init and irq_handler_sercom<nr>
#define LOCONET_BUILD(sercom, tx_port, tx_pin, rx_port, rx_pin, rx_pad, fl_port, fl_pin, fl_int, fl_tmr) \
  HAL_GPIO_PIN(LOCONET_TX, tx_port, tx_pin);                                  \
  HAL_GPIO_PIN(LOCONET_RX, rx_port, rx_pin);                                  \
  HAL_GPIO_PIN(LOCONET_FL, fl_port, fl_pin);                                  \
                                                                              \
  void loconet_init()                                                         \
  {                                                                           \
    /* Set Tx pin as output */                                                \
    HAL_GPIO_LOCONET_TX_out();                                                \
    HAL_GPIO_LOCONET_TX_pmuxen(PORT_PMUX_PMUXE_C_Val);                        \
    HAL_GPIO_LOCONET_TX_clr();                                                \
    /* Set Rx pin as input */                                                 \
    HAL_GPIO_LOCONET_RX_in();                                                 \
    HAL_GPIO_LOCONET_RX_pmuxen(PORT_PMUX_PMUXE_C_Val);                        \
    /* Set Fl pin as input */                                                 \
    HAL_GPIO_LOCONET_FL_in();                                                 \
    HAL_GPIO_LOCONET_FL_pullup();                                             \
    HAL_GPIO_LOCONET_FL_pmuxen(PORT_PMUX_PMUXE_A_Val);                        \
    /* Set loconet_sercom */                                                  \
    loconet_init_usart(                                                       \
      SERCOM##sercom,                                                         \
      PM_APBCMASK_SERCOM##sercom,                                             \
      SERCOM##sercom##_GCLK_ID_CORE,                                          \
      rx_pad,                                                                 \
      SERCOM##sercom##_IRQn                                                   \
    );                                                                        \
                                                                              \
    /* Flank detection */                                                     \
    /* Enable clock for external interrupts, without prescaler */             \
    PM->APBAMASK.reg |= PM_APBAMASK_EIC;                                      \
    GCLK->CLKCTRL.reg =                                                       \
      GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_EIC)                                    \
      | GCLK_CLKCTRL_CLKEN                                                    \
      | GCLK_CLKCTRL_GEN(0);                                                  \
    /* Enable interrupt for external pin */                                   \
    EIC->INTENSET.reg |= EIC_INTENSET_EXTINT##fl_int;                         \
    EIC->CONFIG[fl_int / 8].reg = EIC_CONFIG_SENSE0_BOTH << 4 * (fl_int % 8); \
    NVIC_EnableIRQ(EIC_IRQn);                                                 \
    /* Enable external interrupts */                                          \
    EIC->CTRL.reg |= EIC_CTRL_ENABLE;                                         \
                                                                              \
    /* Enable clock for flank timer, without prescaler */                     \
    PM->APBCMASK.reg |= PM_APBCMASK_TC##fl_tmr;                               \
    GCLK->CLKCTRL.reg =                                                       \
      GCLK_CLKCTRL_ID(TC##fl_tmr##_GCLK_ID)                                   \
      | GCLK_CLKCTRL_CLKEN                                                    \
      | GCLK_CLKCTRL_GEN(0);                                                  \
                                                                              \
    /* CTRLA register:
     *   PRESCSYNC: 0x02  RESYNC
     *   RUNSTDBY:        Ignored
     *   PRESCALER: 0x03  DIV8, each tick will be 1us
     *   WAVEGEN:   0x01  MFRQ, zero counter on match
     *   MODE:      0x00  16 bits timer
     */                                                                       \
    TC##fl_tmr->COUNT16.CTRLA.reg =                                           \
      TC_CTRLA_PRESCSYNC_RESYNC                                               \
      | TC_CTRLA_PRESCALER_DIV8                                               \
      | TC_CTRLA_WAVEGEN_MFRQ                                                 \
      | TC_CTRLA_MODE_COUNT16;                                                \
                                                                              \
    /* INTERRUPTS:
     *   Interrupt on match
     */                                                                       \
    TC##fl_tmr->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);                     \
    NVIC_EnableIRQ(TC##fl_tmr##_IRQn);                                        \
  }                                                                           \
  void loconet_start_timer_delay(uint16_t delay_us) {                         \
    /* Set timer counter to 0 */                                              \
    TC##fl_tmr->COUNT16.COUNT.reg = 0;                                        \
    /* Set timer match, 1200us */                                             \
    TC##fl_tmr->COUNT16.CC[0].reg = delay_us;                                 \
    /* Enable timer */                                                        \
    TC##fl_tmr->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;                         \
  }                                                                           \
  void loconet_handle_eic(void) {                                             \
    /* Return if it's not our external pin to watch */                        \
    if (!EIC->INTFLAG.bit.EXTINT##fl_int) {                                   \
      return;                                                                 \
    }                                                                         \
    /* Determine RISE / FALL */                                               \
    if (HAL_GPIO_LOCONET_FL_read()) {                                         \
      loconet_irq_flank_rise();                                               \
    } else {                                                                  \
      loconet_irq_flank_fall();                                               \
    }                                                                         \
    /* Reset flag */                                                          \
    EIC->INTFLAG.reg |= EIC_INTFLAG_EXTINT##fl_int;                           \
  }                                                                           \
  /* Handle timer interrupt */                                                \
  void irq_handler_tc##fl_tmr(void);                                          \
  void irq_handler_tc##fl_tmr(void) {                                         \
    /* Disable timer */                                                       \
    TC##fl_tmr->COUNT16.CTRLA.bit.ENABLE = 0;                                 \
    /* Reset clock interrupt flag */                                          \
    TC##fl_tmr->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1);                       \
    /* Handle loconet timer */                                                \
    loconet_irq_timer();                                                      \
  }                                                                           \
  /* Handle received bytes */ \
  void irq_handler_sercom##sercom(void); \
  void irq_handler_sercom##sercom(void) \
  { \
    /* Rx complete */ \
    if (SERCOM##sercom->USART.INTFLAG.bit.RXC) { \
      /* Get data from USART and place it in the ringbuffer */ \
      loconet_rx_ringbuffer_push(SERCOM##sercom->USART.DATA.reg); \
    } \
    \
    /* Tx complete */ \
    if (SERCOM##sercom->USART.INTFLAG.bit.TXC) { \
      /* TODO: Handle TX complete */ \
    } \
  } \

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Process the loconet rx ringbuffer.
extern uint8_t process_loconet_rx_ringbuffer(void);

#endif // _LOCONET_LOCONET_H_
