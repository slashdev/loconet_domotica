/**
 * @file loconet.h
 * @brief Loconet base functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * This is a basic implementation of the clock system for Loconet.
 * It reacts on the fast clock messages of loconet to sync the clock.
 * Internally, it updates the clock, until a new fast clock message
 * arrives, after which it resets the clock to the received message,
 * and starts ticking again, using the appropriate clock rate.
 *
 * Additionally, the system can run as a clock master, i.e., it sends
 * the fast clock messages itself. For this, set the appropriate values
 * using:
 *
 *    fast_clock_set_as_master(id1, id2, intermessage_delay)
 *
 * id1 and id2 are used to identify the clock, the intermessage_delay
 * states the seconds between every two clock messages.
 *
 * To return to slave mode, use
 *
 *     fast_clock_set_as_slave();
 *
 * To use the clock system, one should initialize a clock using
 *
 *    LOCONET_BUILD(timer)
 *
 * Where
 * - timer: the TIMER used for fast clock
 *
 * To react on clock changes, you should use the following function
 *
 *     fast_clock_handle_update(FAST_CLOCK_TIME_Type time)
 *
 * It is triggered after every update of the minute counter.
 *
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

// ------------------------------------------------------------------

#ifndef _LOCONET_FAST_CLOCK_H_
#define _LOCONET_FAST_CLOCK_H_

#include <stdbool.h>
#include <stdint.h>
#include "loconet/loconet_tx_messages.h"
#include "utils/logger.h"

typedef struct {
  uint8_t minute;
  uint8_t second;
  uint8_t hour;
  uint8_t day;
} FAST_CLOCK_TIME_Type;


// ------------------------------------------------------------------
// Sets the fast_clock as master. It uses id1 and id2 for
// identifying the master in clock messages.
// The intermessage_delay is the delay in seconds between two
// messages
extern void fast_clock_set_as_master(uint8_t id1, uint8_t id2, uint16_t intermessage_delay);

// ------------------------------------------------------------------
// React on the clock messages as a slave, i.e. it does not send its
// own time messages
extern void fast_clock_set_as_slave(void);

// ------------------------------------------------------------------
// Sets the rate for the clock message. Only useful to use in case
// the system acts as a clock master.
extern void fast_clock_set_rate(uint8_t);

// ------------------------------------------------------------------
// Sets the time
extern void fast_clock_set_time(FAST_CLOCK_TIME_Type time);

// ------------------------------------------------------------------
// Returns the minutes
extern uint8_t fast_clock_get_minutes(void);

// ------------------------------------------------------------------
// Returns the hours
extern uint8_t fast_clock_get_hours(void);

// ------------------------------------------------------------------
// Returns the day.
extern uint8_t fast_clock_get_day(void);

// ------------------------------------------------------------------
// Returns the current time as FAST_CLOCK_TIME_Type
extern FAST_CLOCK_TIME_Type fast_clock_get_time(void);

// ------------------------------------------------------------------
// Returns the time as hour * 100 + minutes
extern uint16_t fast_clock_get_time_as_int(void);

// ------------------------------------------------------------------
// This is the IRQ function that is called after every clock cycle.
extern void fast_clock_tick(void);

// ------------------------------------------------------------------
extern void fast_clock_init(void);
extern void fast_clock_init_timer(Tc*, uint32_t, uint32_t, uint32_t);

#define FAST_CLOCK_BUILD(timer)                                               \
  void fast_clock_init(void)                                                  \
  {                                                                           \
    fast_clock_init_timer(                                                    \
      TC##timer,                                                              \
      PM_APBCMASK_TC##timer,                                                  \
      TC##timer##_GCLK_ID,                                                    \
      TC##timer##_IRQn                                                        \
    );                                                                        \
  }                                                                           \
  /* Handle timer interrupt */                                                \
  void irq_handler_tc##timer(void);                                           \
  void irq_handler_tc##timer(void)                                            \
  {                                                                           \
    /* Reset clock interrupt flag */                                          \
    TC##timer->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1);                        \
    fast_clock_tick();                                                        \
  }                                                                           \

// ------------------------------------------------------------------
// Reacts on the fast clock messages to update the internal clock.
extern void loconet_rx_fast_clock(uint8_t *data, uint8_t length);

#endif // _LOCONET_FAST_CLOCK_H_
