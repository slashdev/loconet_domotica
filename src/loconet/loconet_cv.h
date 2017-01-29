/**
 * @file loconet_cv.h
 * @brief Process Loconet CV messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#ifndef _LOCONET_LOCONET_CV_H_
#define _LOCONET_LOCONET_CV_H_

#include <stdint.h>

typedef struct {
  uint8_t source;
  __attribute__((packed)) uint16_t destination;
  uint8_t request_id;
  uint8_t most_significant_bits;
  __attribute__((packed)) uint16_t device_class;
  __attribute__((packed)) uint16_t lncv_number;
  __attribute__((packed)) uint16_t lncv_value;
  uint8_t flags;
} LOCONET_CV_MSG_Type;

#endif // _LOCONET_LOCONET_CV_H_
