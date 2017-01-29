/**
 * @file loconet_cv.c
 * @brief Process Loconet CV messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#include "loconet_cv.h"

uint16_t loconet_cv_values[LOCONET_CV_MAX_SIZE];
bool loconet_cv_programming;

//-----------------------------------------------------------------------------
static void loconet_cv_response(LOCONET_CV_MSG_Type *msg)
{
  uint8_t resp_data[13];
  resp_data[0] = 15; // Length

  LOCONET_CV_MSG_Type *resp = (LOCONET_CV_MSG_Type*)&resp_data[1];
  resp->source = LOCONET_CV_SRC_MODULE;
  switch (msg->source) {
    case LOCONET_CV_SRC_KPU:
      resp->destination = LOCONET_CV_DST_UB_KPU;
      break;
    default:
      resp->destination = msg->source;
  }
  resp->request_id = LOCONET_CV_REQ_CFGREAD;
  resp->device_class = msg->device_class;
  resp->lncv_number = msg->lncv_number;
  resp->lncv_value = loconet_cv_values[msg->lncv_number];
  resp->flags = 0; // Always 0 for responses

  // Calculate Most Significant Bits
  resp->most_significant_bits = 0;
  for (uint8_t index = 0; index < 7; index++) {
    if (resp_data[6+index] & 0x80) {
      resp->most_significant_bits |= 0x01 << index;
      resp_data[6+index] &= 0x7F;
    }
  }

  // Send message
  loconet_tx_queue_n(0xE5, 1, resp_data, 13);
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_on(LOCONET_CV_MSG_Type *msg)
{
  // lncv_number should be 0, and lncv_value should be 0xFFFF
  // or the address of the device.
  if (msg->lncv_number != 0 || (msg->lncv_value != 0xFFFF && msg->lncv_value != loconet_cv_values[0])) {
    return;
  }

  // Start programming
  loconet_cv_programming = true;
  loconet_cv_response(msg);
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_off(LOCONET_CV_MSG_Type *msg)
{
  (void)msg;
  loconet_cv_programming = false;
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_read(LOCONET_CV_MSG_Type *msg, uint8_t opcode)
{
  if (msg->lncv_number >= LOCONET_CV_MAX_SIZE) {
    loconet_tx_long_ack(opcode, LOCONET_CV_ACK_ERROR_OUTOFRANGE);
    return;
  }

  loconet_cv_response(msg);
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_write(LOCONET_CV_MSG_Type *msg, uint8_t opcode)
{
  // Write is not allowed if we're not in programming mode
  if (!loconet_cv_programming) {
    return;
  }

  // Error if you want to write out of bounds
  if (msg->lncv_number >= LOCONET_CV_MAX_SIZE) {
    loconet_tx_long_ack(opcode, LOCONET_CV_ACK_ERROR_OUTOFRANGE);
    return;
  }
}

//-----------------------------------------------------------------------------
void loconet_cv_process(LOCONET_CV_MSG_Type *msg, uint8_t opcode)
{
  if (msg->device_class != LOCONET_CV_DEVICE_CLASS) {
    return; // We only listen to our own device class
  }
  if (msg->flags == LOCONET_CV_FLG_PROG_ON) {
    loconet_cv_prog_on(msg);
  } else if (msg->flags == LOCONET_CV_FLG_PROG_OFF) {
    loconet_cv_prog_off(msg);
  } else if (msg->request_id == LOCONET_CV_REQ_CFGWRITE) {
    loconet_cv_prog_write(msg, opcode);
  } else {
    loconet_cv_prog_read(msg, opcode);
  }
}

//-----------------------------------------------------------------------------
void loconet_cv_init(void)
{
  // TODO: read CV values from eeprom
  loconet_cv_values[0] = LOCONET_CV_INITIAL_ADDRESS;
  // Disable programming on init
  loconet_cv_programming = false;
}
