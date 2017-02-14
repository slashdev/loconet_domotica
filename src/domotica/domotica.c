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
// Internal structure of a queue to maintain all changes that should be handled
typedef struct OUTPUT_CHANGE {
  uint16_t mask_on;
  uint16_t mask_off;
  struct OUTPUT_CHANGE *next;
} OUTPUT_CHANGE_Type;

static OUTPUT_CHANGE_Type *output_change_queue = 0;

// ------------------------------------------------------------------
void domotica_enqueue_output_change(uint16_t mask_on, uint16_t mask_off)
{
  OUTPUT_CHANGE_Type *change = malloc(sizeof(OUTPUT_CHANGE_Type));
  change->mask_on = mask_on;
  change->mask_off = mask_off;

  if (!output_change_queue)
  {
    output_change_queue = change;
  }
  else
  {
    // Add change add the end of the queue
    OUTPUT_CHANGE_Type *curr = output_change_queue;
    for(; curr; curr = curr->next);
    curr->next = change;
  }

}

// ------------------------------------------------------------------
void domotica_loop(void)
{
  if (output_change_queue)
  {
    // Handle the mask change
    domotica_handle_output_change(output_change_queue->mask_on, output_change_queue->mask_off);

    // Pop the first element from the queue.
    OUTPUT_CHANGE_Type *curr = output_change_queue;
    output_change_queue = output_change_queue->next;
    free(curr);
  }
}

// ------------------------------------------------------------------
void domotica_handle_output_change_dummy(uint16_t mask_on, uint16_t mask_off)
{
  (void) mask_on;
  (void) mask_off;
}
