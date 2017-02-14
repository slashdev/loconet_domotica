/*
 * @file domotica_rx.c
 * @brief Loconet Domotica Module - receiving messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Jan Martijn van der Werf
 */

#include "domotica_rx.h"

// ------------------------------------------------------------------
typedef struct INPUT_ADDRESS {
  uint8_t lncv;
  uint16_t address;
  struct INPUT_ADDRESS *next;
} INPUT_ADDRESS_Type;

static INPUT_ADDRESS_Type *input_addresses = 0;

// ------------------------------------------------------------------
// Adds a sensor device to the list of elements to listen to.
// lncv is used to be able to refer to the corresponding masks
void domotica_rx_set_input_address(uint8_t lncv, uint16_t address)
{

  if (!input_addresses)
  {
    input_addresses = malloc(sizeof(INPUT_ADDRESS_Type));
    input_addresses->lncv = lncv;
    input_addresses->address = address;

    return;
  }

  INPUT_ADDRESS_Type *curr = input_addresses->next;
  INPUT_ADDRESS_Type *prev = input_addresses;

  // Move to the first lncv that is larger than, or equal to the given lncv
  // number
  for(; curr && curr->address < address; prev = curr, curr = curr->next);

  if (curr && curr->address == address) {
    // If curr exists and the address in it equals the address we want
    // to store, then store it!
    curr->lncv = lncv;
  } else {
    // It is larger, thus we need to squeeze inp in!
    INPUT_ADDRESS_Type *inp = malloc(sizeof(INPUT_ADDRESS_Type));
    inp->lncv = lncv;
    inp->address = address;
    inp->next = curr;

    prev->next = inp;
  }
}

// ------------------------------------------------------------------
// Removes the input address that belongs to the given lncv number
void domotica_rx_remove_input_address(uint8_t lncv)
{
  if (!input_addresses)
  {
    return;
  }
  INPUT_ADDRESS_Type *curr = input_addresses->next;
  INPUT_ADDRESS_Type *prev = input_addresses;

  // Move to the first lncv that is larger than, or equal to the given lncv
  // number
  for(; curr && curr->lncv < lncv; prev = curr, curr = curr->next);

  if (curr && curr->lncv == lncv)
  {
    // Set the next of prev to the next of curr, so curr is no longer
    // in the list
    prev->next = curr->next;
    // Then remove it!
    free(curr);
  }
}

// ------------------------------------------------------------------
static uint16_t extract_b2_address(uint8_t byte1, uint8_t byte2)
{
  // The address is encoded in both bytes: the first seven are in in1
  // The latter 4 are in the first 4 bits of the second byte
  uint16_t address = byte1 | ((byte2 & 0x0F)<<7);

  // Multiply by two
  address <<= 1;
  // We always need to add 1 to the address, and 2 if its even
  // If bit 5 is a 1, then the number is even
  address += (byte2 & 0x20) ? 2 : 1;

  return address;
}

// ------------------------------------------------------------------
static bool extract_state(uint8_t byte)
{
  return (byte & 0x10);
}

static uint8_t in_b2_address_list(uint16_t address)
{
  INPUT_ADDRESS_Type *curr = input_addresses;
  for(; curr && curr->address != address; curr = curr->next);

  // If curr is set, then it is the address, since otherwise we would have
  // traversed the whole list, and would arrive at the last address;
  if (curr)
  {
    return curr->lncv;
  }

  return 0;
}

// ------------------------------------------------------------------
void loconet_rx_input_rep(uint8_t in1, uint8_t in2)
{
  uint16_t address = extract_b2_address(in1, in2);
  bool state = extract_state(in2);

  // Check if address is in our array. If so, we need to update the
  // Output array.
  uint8_t lncv = in_b2_address_list(address);
  if (lncv)
  {
    if (state)
    {
      // Add the high mask: on is in lncv+1, off is in lncv+2
      domotica_enqueue_output_change(loconet_cv_get(lncv + 1), loconet_cv_get(lncv + 2));
    }
    else
    {
      // Add the low mask: on is in lncv+3, off is in lncv+4
      domotica_enqueue_output_change(loconet_cv_get(lncv+3), loconet_cv_get(lncv+4));
    }
  }

}
