/*
 * @file fast_clock.c
 * @brief Loconet clock implementation
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 * @see fast_clock.h
 */

#include "fast_clock.h"

// ----------------------------------------------------------------------------
// Prototypes
void fast_clock_handle_update_(uint8_t day, uint8_t hour, uint8_t minute);

// ----------------------------------------------------------------------------
// this is the function that any system that wants to react on the clock should
// implement.
//
__attribute__ ((weak, alias("fast_clock_handle_update_")))
  void fast_clock_handle_update(uint8_t day, uint8_t hour, uint8_t minute);

// ----------------------------------------------------------------------------
typedef struct {
  bool master;
  uint8_t id1;
  uint8_t id2;
  uint16_t intermessage_delay;
  uint8_t rate;
  uint8_t minute;
  uint8_t second;
  uint8_t hour;
  uint8_t day;
} fast_clock_STATUS_Type;

fast_clock_STATUS_Type fast_clock_status = {0, 0, 0, 0, 1, 0, 0, 0, 0};

uint16_t fast_clock_current_intermessage_delay = 0;

// ----------------------------------------------------------------------------
// ONLY FOR DEBUGGING
static void print_time(uint8_t day, uint8_t hour, uint8_t minute)
{
  switch(day)
  {
    default:
    case 0:
      logger_string("Sunday ");
      break;
    case 1:
      logger_string("Monday ");
      break;
    case 2:
      logger_string("Tuesday ");
      break;
    case 3:
      logger_string("Wednesday ");
      break;
    case 4:
      logger_string("Thursday ");
      break;
    case 5:
      logger_string("Friday ");
      break;
    case 6:
      logger_string("Saturday ");
      break;
  }
  logger_number(hour);
  logger_char(':');
  logger_number(minute);
  logger_newline();
}


// ----------------------------------------------------------------------------
// The clock is defined on div1024. That means that 7812 ticks equals a second
#define FAST_CLOCK_TIMER_DELAY 7812
Tc *fast_clock_timer;

// ----------------------------------------------------------------------------
//
void fast_clock_init_timer(Tc *timer, uint32_t pm_tmr_mask, uint32_t gclock_tmr_id, uint32_t nvic_irqn)
{
  fast_clock_timer = timer;

  // Enable clock for fast clock, without prescaler
  PM->APBCMASK.reg |= pm_tmr_mask;
  GCLK->CLKCTRL.reg =
    GCLK_CLKCTRL_ID(gclock_tmr_id)
    | GCLK_CLKCTRL_CLKEN
    | GCLK_CLKCTRL_GEN(0);

  /* CTRLA register:
   *   PRESCSYNC: 0x02  RESYNC
   *   RUNSTDBY:        Ignored
   *   PRESCALER: 0x07  DIV1024, each tick will be 1/7812 s
   *   WAVEGEN:   0x01  MFRQ, zero counter on match
   *   MODE:      0x00  16 bits timer
   */
  fast_clock_timer->COUNT16.CTRLA.reg =
    TC_CTRLA_PRESCSYNC_RESYNC
    | TC_CTRLA_PRESCALER_DIV1024
    | TC_CTRLA_WAVEGEN_MFRQ
    | TC_CTRLA_MODE_COUNT16;

  fast_clock_timer->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);
  NVIC_EnableIRQ(nvic_irqn);

  fast_clock_timer->COUNT16.COUNT.reg = 0;
  fast_clock_timer->COUNT16.CC[0].reg = FAST_CLOCK_TIMER_DELAY;
  fast_clock_timer->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
}

// ----------------------------------------------------------------------------
// sets the values for being a master: whether it is a
void fast_clock_set_as_master(uint8_t id1, uint8_t id2, uint16_t intermessage_delay)
{
  fast_clock_status.master = true;
  fast_clock_status.id1 = id1;
  fast_clock_status.id2 = id2;
  fast_clock_status.intermessage_delay = intermessage_delay;
}

// ----------------------------------------------------------------------------
// sets the thing as a slave.
void fast_clock_set_as_slave(void)
{
  fast_clock_status.master = false;
}

//----------------------------------------------------------------------------
static void fast_clock_set(uint8_t day, uint8_t hour, uint8_t minute)
{
  fast_clock_status.day = day % 7;
  fast_clock_status.hour = hour % 24;
  fast_clock_status.minute = minute % 60;

  fast_clock_status.second = 0;

  logger_string("RECEIVED: ");
  fast_clock_handle_update(fast_clock_status.day, fast_clock_status.hour, fast_clock_status.minute);
}


//----------------------------------------------------------------------------
// this function sets the clock rate.
void fast_clock_set_rate(uint8_t rate)
{
  fast_clock_status.rate = rate;
}

//----------------------------------------------------------------------------
// this function processes the clock information sent on the loconet
void loconet_rx_fast_clock(uint8_t *data, uint8_t length)
{
  if (length < 8)
  {
    return;
  }

  if (!(data[7] == 1)) {
    // the message is not a correct clock tick!
    return;
  }

  //1st byte of data is the clock rate
  fast_clock_set_rate(data[0]);

  uint8_t minute = data[3] - (128-60);
  uint8_t hour = data[5] >= (128-24) ? data[5] - (128-24) : data[5] % 24;
  uint8_t day = data[6] % 7;

  fast_clock_set(day, hour, minute);
}

//-----------------------------------------------------------------------------
// sends the message in the appropriate format
static void fast_clock_send_message(void)
{
  loconet_tx_fast_clock(
    fast_clock_status.rate,
    0x0,
    0x0,
    fast_clock_status.minute,
    fast_clock_status.hour,
    fast_clock_status.day,
    fast_clock_status.id1,
    fast_clock_status.id2
  );
}


// ----------------------------------------------------------------------------
// this function should be called every second. It updates the clock with the
// set rate.
void fast_clock_tick(void)
{
  // keeps track whether we need an update. We only send an update
  // if the minute counter has been increased.
  bool notify = false;

  // update the seconds passed with the clock rate
  fast_clock_status.second += fast_clock_status.rate;

  // as the rate can be higher than 60, we need to adjust
  // the minutes according to the number of seconds added
  // suppose the max rate is used, i.e., 127, then we have
  // to add 2 minutes per tick. Hence, we need a while instead
  // of an if.

  while (fast_clock_status.second > 60)
  {
    fast_clock_status.minute++;
    fast_clock_status.second -= 60;
    notify = true;
  }

  if (fast_clock_status.minute > 59)
  {
    fast_clock_status.minute = 0;
    fast_clock_status.hour++;
  }
  if (fast_clock_status.hour > 23)
  {
    fast_clock_status.hour = 0;

    fast_clock_status.day++;
    fast_clock_status.day %= 7;
  }

  if (notify)
  {
    fast_clock_handle_update(fast_clock_status.day, fast_clock_status.hour, fast_clock_status.minute);
  }

  // Do we need to send a message as master?
  if (fast_clock_status.master && fast_clock_current_intermessage_delay++ > fast_clock_status.intermessage_delay)
  {
    // Send the message!
    fast_clock_send_message();
    // Reset the intermessage delay
    fast_clock_current_intermessage_delay = 0;
  }
}

// ------------------------------------------------------------------
uint8_t fast_clock_get_minutes(void)
{
  return fast_clock_status.minute;
}

// ------------------------------------------------------------------
uint8_t fast_clock_get_hours(void)
{
  return fast_clock_status.hour;
}

// ------------------------------------------------------------------
uint8_t fast_clock_get_day(void)
{
  return fast_clock_status.day;
}

// ------------------------------------------------------------------
uint16_t fast_clock_get_time(void)
{
  return fast_clock_status.hour * 100 + fast_clock_status.minute;
}



// ----------------------------------------------------------------------------
// Dummy implementation of the clock update. This one needs to be
// implemented by the real program, to react on clock changes.
void fast_clock_handle_update_(uint8_t day, uint8_t hour, uint8_t minute)
{
  (void) day;
  (void) hour;
  (void) minute;

  print_time(day, hour, minute);
}

