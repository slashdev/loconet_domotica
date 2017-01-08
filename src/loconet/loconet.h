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
 *
 * Loconet uses CSMA/CD techniques to arbitrage and control network
 * access. The bit times are 60 uSecs or 16.66 KBaud +/- 1.5%. This
 * means that a Baudrate of 16.457 can be used as well. The data is
 * sent asynchonous using 1 start bit, 8 data bits and 1 stop bit.
 * The data is sent LSB first. Bytes can be sent back-to-back.
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
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 */

#ifndef _LOCONET_LOCONET_H_
#define _LOCONET_LOCONET_H_

#include <stdint.h>
#include "samd20.h"

//-----------------------------------------------------------------------------
extern void loconet_init(void);
extern void loconet_rx_ringbuffer_push(uint8_t byte);

// Marco for loconet_init and irq_handler_sercom<nr>
#define LOCONET_BUILD(sercom, tx_port, tx_pin, rx_port, rx_pin, rx_pad) \
  HAL_GPIO_PIN(LOCONET_TX, tx_port, tx_pin); \
  HAL_GPIO_PIN(LOCONET_RX, rx_port, rx_pin); \
  \
  void loconet_init() \
  { \
    /* Set Tx pin as output */ \
    HAL_GPIO_LOCONET_TX_out(); \
    HAL_GPIO_LOCONET_TX_pmuxen(PORT_PMUX_PMUXE_C_Val); \
    HAL_GPIO_LOCONET_TX_clr(); \
    /* Set Rx pin as input */ \
    HAL_GPIO_LOCONET_RX_in(); \
    HAL_GPIO_LOCONET_RX_pmuxen(PORT_PMUX_PMUXE_C_Val); \
    /* Enable clock for peripheral, without prescaler */ \
    PM->APBCMASK.reg |= PM_APBCMASK_SERCOM##sercom; \
    GCLK->CLKCTRL.reg = \
      GCLK_CLKCTRL_ID(SERCOM##sercom##_GCLK_ID_CORE) | \
      GCLK_CLKCTRL_CLKEN | \
      GCLK_CLKCTRL_GEN(0); \
    \
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
     */ \
    SERCOM##sercom->USART.CTRLA.reg = \
      SERCOM_USART_CTRLA_DORD | \
      SERCOM_USART_CTRLA_MODE_USART_INT_CLK | \
      SERCOM_USART_CTRLA_RXPO(rx_pad) | \
      SERCOM_USART_CTRLA_TXPO; \
    \
    /* CTRLB register:
     *   RXEN:      0x01  Enable Rx
     *   TXEN:      0x00  Only enable Tx when sending
     *   PMODE:     0x00  Ignored, parity is not used
     *   SFDE:      0x00  Ignored, chip does not go into standby
     *   SBMODE:    0x00  One stop bit
     *   CHSIZE:    0x00  Char size: 8 bits
     */ \
    SERCOM##sercom->USART.CTRLB.reg = \
      SERCOM_USART_CTRLB_RXEN | \
      SERCOM_USART_CTRLB_CHSIZE(0); \
    \
    uint64_t br = (uint64_t)65536 * (F_CPU - 16 * 16666) / F_CPU; \
    SERCOM##sercom->USART.BAUD.reg = (uint16_t)br; \
    \
    /* INTERRUPTS register
     *   RXS:       0x00  No interrupt on Rx start
     *   RXC:       0x01  Interrupt on Rx complete
     *   TXC:       0x01  Interrupt on Tx complete
     *   DRE:       0x00  No interrupt on data registry empty
     */ \
    SERCOM##sercom->USART.INTENSET.reg = \
      SERCOM_USART_INTENSET_RXC | \
      SERCOM_USART_INTENSET_TXC; \
    NVIC_EnableIRQ(SERCOM##sercom##_IRQn); \
    \
    SERCOM##sercom->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE; \
  } \
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
// Process the loconet rx ringbuffer.
extern uint8_t process_loconet_rx_ringbuffer(void);

#endif // _LOCONET_LOCONET_H_
