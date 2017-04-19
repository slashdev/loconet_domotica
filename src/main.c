/*
 * Copyright (c) 2017, Alex Taradov <alex@taradov.com>,
 * Ferdi van der Werf <ferdi@slashdev.nl> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// ----------------------------------------------------------------------------
// We need 240 CV values
// #define LOCONET_CV_NUMBERS 240

// All includes
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "samd20.h"
#include "hal_gpio.h"
#include "loconet/loconet.h"
#include "loconet/loconet_cv.h"
#include "utils/eeprom.h"

#include "outputhandler/outputhandler.h"

#include "components/fast_clock.h"

#include "domotica/domotica.h"
#include "domotica/domotica_cv.h"
#include "domotica/domotica_rx.h"

//-----------------------------------------------------------------------------
HAL_GPIO_PIN(LED, A, 12);

// TODO: Change according to actual values. For now I use them in the same
// setting as Ferdi has them.
LOCONET_BUILD(2/*sercom*/, A/*tx_port*/, 14/*tx_pin*/, A/*rx_port*/, 15/*rx_pin*/, 3/*rx_pad*/, A/*fl_port*/, 13/*fl_pin*/, 13/*fl_int*/, 1/*fl_tmr*/);

// Initialize the FAST CLOCK, set it to use Timer 2.
FAST_CLOCK_BUILD(2)

// ----------------------------------------------------------------------------
// Sets the output to brightness level `brightness`
void outputhandler_set_output_state(uint8_t output, uint8_t brightness);
void outputhandler_set_output_state(uint8_t output, uint8_t brightness)
{
  (void) output;
  (void) brightness;
}

// ----------------------------------------------------------------------------
void outputhandler_switch_state_pre_event(uint16_t state);
void outputhandler_switch_state_pre_event(uint16_t state)
{
  (void) state;
}

// ----------------------------------------------------------------------------
void outputhandler_switch_state_post_event(uint16_t state);
void outputhandler_switch_state_post_event(uint16_t state)
{
  (void) state;
}

//----------------------------------------------------------------------------
void irq_handler_eic(void);
void irq_handler_eic(void) {
  if (loconet_handle_eic()) {
    return;
  }
}

//-----------------------------------------------------------------------------
static void sys_init(void)
{
  // Switch to 8MHz clock (disable prescaler)
  SYSCTRL->OSC8M.bit.PRESC = 0;

  // Enable interrupts
  asm volatile ("cpsie i");
}

//-----------------------------------------------------------------------------
static void hard_reset(void)
{
  __DSB();
  asm volatile ("cpsid i");
  WDT->CONFIG.reg = 0;
  WDT->CTRL.reg |= WDT_CTRL_ENABLE;
  while(1);
}

//-----------------------------------------------------------------------------
static void eeprom_init(void)
{
  enum status_code error_code = eeprom_emulator_init();

  // Fusebits for memory are not set, or too low.
  // We need 4+2 pages, so set to
  if (error_code == STATUS_ERR_NO_MEMORY) {
    struct nvm_fusebits fusebits;
    nvm_get_fuses(&fusebits);
    fusebits.eeprom_size = NVM_EEPROM_EMULATOR_SIZE_2048;
    nvm_set_fuses(&fusebits);
    hard_reset();
  } else if (error_code != STATUS_OK) {
    // Erase eeprom, assume unformated or corrupt
    eeprom_emulator_erase_memory();
    hard_reset();
  }
}

//-----------------------------------------------------------------------------
int main(void)
{
  sys_init();
  eeprom_init();
  // Set LED GPIO as output
  HAL_GPIO_LED_out();
  // Turn on the LED
  HAL_GPIO_LED_set();

  // Initialize CVs for loconet
  loconet_cv_init();

  // Set loconet basics
  loconet_config.bit.ADDRESS = loconet_cv_get(0);
  loconet_config.bit.PRIORITY = loconet_cv_get(2);

  // Initialize loconet
  loconet_init();

  // Set up the fast clock
  fast_clock_init();
  fast_clock_set_slave();

  // Initialize domotica
  domotica_init();

  while (1) {
    loconet_loop();
    fast_clock_loop();
    domotica_loop();
  }
  return 0;
}

