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

//-----------------------------------------------------------------------------
static void loconet_cv_prog_on(LOCONET_CV_MSG_Type *msg)
{
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
}
