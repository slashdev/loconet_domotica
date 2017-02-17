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
typedef struct {
  uint8_t lncv;
  uint16_t address;
} INPUT_ADDRESS_Type;

static INPUT_ADDRESS_Type b2_addresses[DOMOTICA_RX_INPUT_ADDRESS_SIZE];

void domotica_rx_init(void)
{
  b2_addresses[0].address = 0;
}

// ------------------------------------------------------------------
// Adds a sensor device to the list of elements to listen to.
// lncv is used to be able to refer to the corresponding masks
void domotica_rx_set_input_address(uint8_t lncv, uint16_t address)
{
  uint8_t index = 0;
  for(; index < DOMOTICA_RX_INPUT_ADDRESS_SIZE && b2_addresses[index].address > 0 ; index++ );

  if (index < DOMOTICA_RX_INPUT_ADDRESS_SIZE)
  {
    b2_addresses[index].address = address;
    b2_addresses[index].lncv = lncv;
  }
}

// ------------------------------------------------------------------
// Removes the input address that belongs to the given lncv number
void domotica_rx_remove_input_address(uint8_t lncv)
{
  uint8_t index = 0;
  for(; index < DOMOTICA_RX_INPUT_ADDRESS_SIZE && b2_addresses[index].lncv != lncv ; index++);
  if (index < DOMOTICA_RX_INPUT_ADDRESS_SIZE)
  {
    b2_addresses[index].address = 0;
    b2_addresses[index].lncv = 0;
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
  // We always need to add 1 to the address, and 2 if its odd
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
  for(uint8_t index = 0 ; index < DOMOTICA_RX_INPUT_ADDRESS_SIZE ; index++)
  {
    if (b2_addresses[index].address == address)
    {
      return b2_addresses[index].lncv;
    }
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
