/*
 * @file domotica_fastclock.c
 * @brief Loconet Domotica Module - listen to the fast clock
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 */

#include "domotica_fastclock.h"

// ----------------------------------------------------------------------------
typedef struct {
  uint16_t timestamp;
  uint8_t lncv;
} TIMESTAMP_Type;

TIMESTAMP_Type timestamps[60];

// ----------------------------------------------------------------------------
static uint16_t last_timestamp = 2400;

// ----------------------------------------------------------------------------
bool is_enabled = true;

// ----------------------------------------------------------------------------
void domotica_fastclock_enable(bool enabled)
{
  is_enabled = enabled;
}

// ----------------------------------------------------------------------------
void fast_clock_handle_update(FAST_CLOCK_TIME_Type time){
  // If the fast clock is not enabled, do not use it
  if (!is_enabled) {
    return;
  }

  // Calculate the current time stamp
  uint16_t current_time = time.hour * 100 + time.minute;

  for(uint8_t index = 0 ; index < 60 ; index++)
  {
    // If last_timestamp is smaller than the current_time, we just check
    // whether the timestamp is in interval (last_timestamp, current_timestamp]
    // Otherwise, i.e., last_timestamp is bigger than current_time, then we
    // need to check whether the timestamp is in (last_timestamp, 2400) or
    // in [0, current_timestamp]. Hence the huge if condition.
    if (
          (   last_timestamp < current_time
           && timestamps[index].timestamp > last_timestamp
           && timestamps[index].timestamp <= current_time
          )
          ||
          (   last_timestamp > current_time
           && (   (   timestamps[index].timestamp > last_timestamp
                   && timestamps[index].timestamp < 2359
                  )
               || timestamps[index].timestamp <= current_time
              )
          )
       )
    {
      domotica_enqueue_output_change(
        loconet_cv_get(timestamps[index].lncv + 1),
        loconet_cv_get(timestamps[index].lncv + 2)
      );
    }
  }
  // Set the last_timestamp to the current time.
  last_timestamp = current_time;
}
