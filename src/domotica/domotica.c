/*
 * @file domotica.c
 * @brief Loconet Domotica Module - receiving messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 */

#include "domotica.h"

// ------------------------------------------------------------------
// Prototypes
void domotica_handle_output_change_dummy(uint16_t mask_on, uint16_t mask_off);

// ------------------------------------------------------------------
// This prototype is used to abstract from the actual output change,
// so that the same domotica can be used for different types of output channels
__attribute__ ((weak, alias ("domotica_handle_output_change_dummy"))) \
  void domotica_handle_output_change(uint16_t mask_on, uint16_t mask_off);

// ------------------------------------------------------------------
// Output brightness
static uint8_t domotica_output_brightness[DOMOTICA_OUTPUT_SIZE];

// ------------------------------------------------------------------
// Internal structure of a queue to maintain all changes that should be handled
typedef struct {
  uint16_t mask_on;
  uint16_t mask_off;
} DOMOTICA_OUTPUT_CHANGE_Type;

typedef struct {
  DOMOTICA_OUTPUT_CHANGE_Type buffer[DOMOTICA_CHANGE_BUFFER_Size];
  volatile uint8_t writer;
  volatile uint8_t reader;
} DOMOTICA_OUTPUT_CHANGE_RINGBUFFER_Type;

static DOMOTICA_OUTPUT_CHANGE_RINGBUFFER_Type buffer = { { { 0, 0} }, 0, 0};

// ------------------------------------------------------------------
void domotica_enqueue_output_change(uint16_t mask_on, uint16_t mask_off)
{
  uint8_t index = (buffer.writer + 1) % DOMOTICA_CHANGE_BUFFER_Size;

  // If the buffer is full, wait until the reader empties
  // a slot in the buffer to write to.
  while (index == buffer.reader) {
    continue;
  }

  // Write the byte
  buffer.buffer[buffer.writer].mask_on = mask_on;
  buffer.buffer[buffer.writer].mask_off = mask_off;

  // Update the writer
  buffer.writer = index;
}

// ------------------------------------------------------------------
void domotica_loop(void)
{
  uint8_t reader = buffer.reader;
  uint8_t writer = buffer.writer;

  if (writer == reader) {
    return;
  }
  // The indexes are not equal, so the writer is ahead!
  // As we do not need to know anything about sizes, we can just read the
  // current message and advance...
  domotica_handle_output_change(buffer.buffer[reader].mask_on, buffer.buffer[reader].mask_off);
  buffer.reader = (reader + 1) % DOMOTICA_CHANGE_BUFFER_Size;
}

// ------------------------------------------------------------------
void domotica_init(void)
{
  domotica_rx_init();
  domotica_cv_init();
}

// ------------------------------------------------------------------
void domotica_handle_output_change_dummy(uint16_t mask_on, uint16_t mask_off)
{
  (void) mask_on;
  (void) mask_off;
}

void domotica_set_output_brightness(uint8_t output, uint8_t value)
{
  outputhandler_set_output_brightness(output, value);
}
