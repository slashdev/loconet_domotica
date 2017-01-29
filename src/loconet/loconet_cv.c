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
static void loconet_cv_prog_on(LOCONET_CV_MSG_Type *msg)
{
  // lncv_number should be 0, and lncv_value should be 0xFFFF
  // or the address of the device.
  if (msg->lncv_number != 0 || (msg->lncv_value != 0xFFFF && msg->lncv_value != loconet_cv_values[0])) {
    return;
  }

  // Start programming
  loconet_cv_programming = true;
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_off(LOCONET_CV_MSG_Type *msg)
{
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_read(LOCONET_CV_MSG_Type *msg, uint8_t opcode)
{
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_write(LOCONET_CV_MSG_Type *msg, uint8_t opcode)
{
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
