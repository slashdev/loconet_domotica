/**
 * @file loconet.c
 * @brief Loconet base functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 */
#include "loconet.h"

//-----------------------------------------------------------------------------
// Define LOCONET_RX_RINGBUFFER_Size if it's not defined
#ifndef LOCONET_RX_RINGBUFFER_Size
#define LOCONET_RX_RINGBUFFER_Size 64
#endif

typedef struct {
  uint8_t buffer[LOCONET_RX_RINGBUFFER_Size];
  volatile uint8_t writer;
  volatile uint8_t reader;
} LOCONET_RX_RINGBUFFER_Type;

static LOCONET_RX_RINGBUFFER_Type loconet_rx_ringbuffer = { { 0 }, 0, 0};

//-----------------------------------------------------------------------------
void loconet_rx_ringbuffer_push(uint8_t byte)
{
  // Get index + 1 of buffer head
  uint8_t index = (loconet_rx_ringbuffer.writer + 1) % LOCONET_RX_RINGBUFFER_Size;

  // If the buffer is full, wait until the reader empties
  // a slot in the buffer to write to.
  while (index == loconet_rx_ringbuffer.reader) {
    continue;
  }

  // Write the byte
  loconet_rx_ringbuffer.buffer[loconet_rx_ringbuffer.writer] = byte;
  loconet_rx_ringbuffer.writer = index;
}

//-----------------------------------------------------------------------------
void ln_handler_dummy_0(void);
void ln_handler_dummy_2(uint8_t, uint8_t);
void ln_handler_dummy_4(uint8_t, uint8_t, uint8_t, uint8_t);
void ln_handler_dummy_n(uint8_t*, uint8_t);

//-----------------------------------------------------------------------------
#define LN_DUMMY_0(name) \
  __attribute__ ((weak, alias ("ln_handler_dummy_0"))) \
  void ln_handler_##name(void)
#define LN_DUMMY_2(name) \
  __attribute__ ((weak, alias ("ln_handler_dummy_2"))) \
  void ln_handler_##name(uint8_t, uint8_t)
#define LN_DUMMY_4(name) \
  __attribute__ ((weak, alias ("ln_handler_dummy_4"))) \
  void ln_handler_##name(uint8_t, uint8_t, uint8_t, uint8_t)
#define LN_DUMMY_N(name) \
  __attribute__ ((weak, alias ("ln_handler_dummy_n"))) \
  void ln_handler_##name(uint8_t*, uint8_t)

//-----------------------------------------------------------------------------
LN_DUMMY_0(busy);
LN_DUMMY_0(gpoff);
LN_DUMMY_0(gpon);
LN_DUMMY_0(idle);

//-----------------------------------------------------------------------------
LN_DUMMY_2(loco_spd);
LN_DUMMY_2(loco_dirf);
LN_DUMMY_2(loco_snd);
LN_DUMMY_2(sq_req);
LN_DUMMY_2(sw_rep);
LN_DUMMY_2(input_rep);
LN_DUMMY_2(long_ack);
LN_DUMMY_2(slot_stat1);
LN_DUMMY_2(consist_func);
LN_DUMMY_2(unlink_slots);
LN_DUMMY_2(link_slots);
LN_DUMMY_2(move_slots);
LN_DUMMY_2(rq_sl_data);
LN_DUMMY_2(sw_state);
LN_DUMMY_2(sw_ack);
LN_DUMMY_2(loco_adr);

//-----------------------------------------------------------------------------
LN_DUMMY_N(wr_sl_data);
LN_DUMMY_N(rd_sl_data);
LN_DUMMY_N(prog_task_start);
LN_DUMMY_N(prog_task_final);
LN_DUMMY_N(fast_clock);

//-----------------------------------------------------------------------------
// Special handlers which cannot be overriden
static void ln_handler_wr_sl_data_(uint8_t*, uint8_t);
static void ln_handler_rd_sl_data_(uint8_t*, uint8_t);

//-----------------------------------------------------------------------------
void (* const ln_messages_0[32])(void) = {
  ln_handler_dummy_0,     // 0x80
  ln_handler_busy,        // 0x81
  ln_handler_gpoff,       // 0x82
  ln_handler_gpon,        // 0x83
  ln_handler_dummy_0,     // 0x84
  ln_handler_idle,        // 0x85
  ln_handler_dummy_0,     // 0x86
  ln_handler_dummy_0,     // 0x87
  ln_handler_dummy_0,     // 0x88
  ln_handler_dummy_0,     // 0x89
  ln_handler_dummy_0,     // 0x8A
  ln_handler_dummy_0,     // 0x8B
  ln_handler_dummy_0,     // 0x8C
  ln_handler_dummy_0,     // 0x8D
  ln_handler_dummy_0,     // 0x8E
  ln_handler_dummy_0,     // 0x8F
  ln_handler_dummy_0,     // 0x90
  ln_handler_dummy_0,     // 0x91
  ln_handler_dummy_0,     // 0x92
  ln_handler_dummy_0,     // 0x93
  ln_handler_dummy_0,     // 0x94
  ln_handler_dummy_0,     // 0x95
  ln_handler_dummy_0,     // 0x96
  ln_handler_dummy_0,     // 0x97
  ln_handler_dummy_0,     // 0x98
  ln_handler_dummy_0,     // 0x99
  ln_handler_dummy_0,     // 0x9A
  ln_handler_dummy_0,     // 0x9B
  ln_handler_dummy_0,     // 0x9C
  ln_handler_dummy_0,     // 0x9D
  ln_handler_dummy_0,     // 0x9E
  ln_handler_dummy_0,     // 0x9F
};

//-----------------------------------------------------------------------------
void (* const ln_messages_2[32])(uint8_t, uint8_t) = {
  ln_handler_loco_spd,    // 0xA0
  ln_handler_loco_dirf,   // 0xA1
  ln_handler_loco_snd,    // 0xA2
  ln_handler_dummy_2,     // 0xA3
  ln_handler_dummy_2,     // 0xA4
  ln_handler_dummy_2,     // 0xA5
  ln_handler_dummy_2,     // 0xA6
  ln_handler_dummy_2,     // 0xA7
  ln_handler_dummy_2,     // 0xA8
  ln_handler_dummy_2,     // 0xA9
  ln_handler_dummy_2,     // 0xAA
  ln_handler_dummy_2,     // 0xAB
  ln_handler_dummy_2,     // 0xAC
  ln_handler_dummy_2,     // 0xAD
  ln_handler_dummy_2,     // 0xAE
  ln_handler_dummy_2,     // 0xAF
  ln_handler_sq_req,      // 0xB0
  ln_handler_sw_rep,      // 0xB1
  ln_handler_input_rep,   // 0xB2
  ln_handler_dummy_2,     // 0xB3
  ln_handler_long_ack,    // 0xB4
  ln_handler_slot_stat1,  // 0xB5
  ln_handler_consist_func,// 0xB6
  ln_handler_dummy_2,     // 0xB7
  ln_handler_unlink_slots,// 0xB8
  ln_handler_link_slots,  // 0xB9
  ln_handler_move_slots,  // 0xBA
  ln_handler_rq_sl_data,  // 0xBB
  ln_handler_sw_state,    // 0xBC
  ln_handler_sw_ack,      // 0xBD
  ln_handler_dummy_2,     // 0xBE
  ln_handler_loco_adr,    // 0xBF
};

//-----------------------------------------------------------------------------
void (* const ln_messages_4[32])(uint8_t, uint8_t, uint8_t, uint8_t) = {
  ln_handler_dummy_4,     // 0xC0
  ln_handler_dummy_4,     // 0xC1
  ln_handler_dummy_4,     // 0xC2
  ln_handler_dummy_4,     // 0xC3
  ln_handler_dummy_4,     // 0xC4
  ln_handler_dummy_4,     // 0xC5
  ln_handler_dummy_4,     // 0xC6
  ln_handler_dummy_4,     // 0xC7
  ln_handler_dummy_4,     // 0xC8
  ln_handler_dummy_4,     // 0xC9
  ln_handler_dummy_4,     // 0xCA
  ln_handler_dummy_4,     // 0xCB
  ln_handler_dummy_4,     // 0xCC
  ln_handler_dummy_4,     // 0xCD
  ln_handler_dummy_4,     // 0xCE
  ln_handler_dummy_4,     // 0xCF
  ln_handler_dummy_4,     // 0xD0
  ln_handler_dummy_4,     // 0xD1
  ln_handler_dummy_4,     // 0xD2
  ln_handler_dummy_4,     // 0xD3
  ln_handler_dummy_4,     // 0xD4
  ln_handler_dummy_4,     // 0xD5
  ln_handler_dummy_4,     // 0xD6
  ln_handler_dummy_4,     // 0xD7
  ln_handler_dummy_4,     // 0xD8
  ln_handler_dummy_4,     // 0xD9
  ln_handler_dummy_4,     // 0xDA
  ln_handler_dummy_4,     // 0xDB
  ln_handler_dummy_4,     // 0xDC
  ln_handler_dummy_4,     // 0xDD
  ln_handler_dummy_4,     // 0xDE
  ln_handler_dummy_4,     // 0xDF
};

//-----------------------------------------------------------------------------
void (* const ln_messages_n[32])(uint8_t*, uint8_t) = {
  ln_handler_dummy_n,     // 0xE0
  ln_handler_dummy_n,     // 0xE1
  ln_handler_dummy_n,     // 0xE2
  ln_handler_dummy_n,     // 0xE3
  ln_handler_dummy_n,     // 0xE4
  ln_handler_dummy_n,     // 0xE5
  ln_handler_dummy_n,     // 0xE6
  ln_handler_rd_sl_data_, // 0xE7
  ln_handler_dummy_n,     // 0xE8
  ln_handler_dummy_n,     // 0xE9
  ln_handler_dummy_n,     // 0xEA
  ln_handler_dummy_n,     // 0xEB
  ln_handler_dummy_n,     // 0xEC
  ln_handler_dummy_n,     // 0xED
  ln_handler_dummy_n,     // 0xEE
  ln_handler_wr_sl_data_, // 0xEF
  ln_handler_dummy_n,     // 0xF0
  ln_handler_dummy_n,     // 0xF1
  ln_handler_dummy_n,     // 0xF2
  ln_handler_dummy_n,     // 0xF3
  ln_handler_dummy_n,     // 0xF4
  ln_handler_dummy_n,     // 0xF5
  ln_handler_dummy_n,     // 0xF6
  ln_handler_dummy_n,     // 0xF7
  ln_handler_dummy_n,     // 0xF8
  ln_handler_dummy_n,     // 0xF9
  ln_handler_dummy_n,     // 0xFA
  ln_handler_dummy_n,     // 0xFB
  ln_handler_dummy_n,     // 0xFC
  ln_handler_dummy_n,     // 0xFD
  ln_handler_dummy_n,     // 0xFE
  ln_handler_dummy_n,     // 0xFF
};

//-----------------------------------------------------------------------------
// Read slot data (SL_RD_DATA)
// Handle special cases
static void ln_handler_rd_sl_data_(uint8_t *data, uint8_t length) {
  if (data[0] == 0x7C) { // Program task final
    ln_handler_prog_task_final(&data[1], length - 1);
  } else { // Default handler
    ln_handler_rd_sl_data(data, length);
  }
}

//-----------------------------------------------------------------------------
// Write slot data (RW_SL_DATA)
// Handle special cases
static void ln_handler_wr_sl_data_(uint8_t *data, uint8_t length) {
  if (data[0] == 0x7B) { // Fast clock
    ln_handler_fast_clock(&data[1], length - 1);
  } else if (data[0] == 0x7C) { // Program task start
    ln_handler_prog_task_start(&data[1], length - 1);
  } else { // Default handler
    ln_handler_wr_sl_data(data, length);
  }
}

//-----------------------------------------------------------------------------
typedef union {
  struct {
    uint8_t NUMBER:5;
    uint8_t OPCODE:3;
  } bits;
  uint8_t byte;
} LOCONET_OPCODE_BYTE_Type;

#define LOCONET_OPCODE_FLAG_Pos 7
#define LOCONET_OPCODE_FLAG (0x01ul << LOCONET_OPCODE_FLAG_Pos)

//-----------------------------------------------------------------------------
uint8_t process_loconet_rx_ringbuffer(void)
{
  // Get values from ringbuffer
  uint8_t *buffer = loconet_rx_ringbuffer.buffer;
  uint8_t reader = loconet_rx_ringbuffer.reader;
  uint8_t writer = loconet_rx_ringbuffer.writer;

  // Expect an opcode byte from ringbuffer
  LOCONET_OPCODE_BYTE_Type opcode;
  opcode.byte = buffer[reader];

  // If the reader is "ahead" of the writer in the ringbuffer,
  // add the size of the ringbuffer to the writer for its
  // fictive index.
  if (reader > writer) {
    writer += LOCONET_RX_RINGBUFFER_Size;
  }

  if (writer <= reader + 1) {
    // We want at least two bytes before we try to read the message
    return 0;
  }

  // If it's not an OPCODE byte, skip it
  if (!(opcode.byte & LOCONET_OPCODE_FLAG)) {
    loconet_rx_ringbuffer.reader = (reader + 1) % LOCONET_RX_RINGBUFFER_Size;
    return 0;
  }

  // New message
  uint8_t message_size = 0;
  switch (opcode.bits.OPCODE) {
    case 0x04:
      message_size = 2;
      break;
    case 0x05:
      message_size = 4;
      break;
    case 0x06:
      message_size = 6;
      break;
    case 0x07:
      message_size = buffer[(reader + 1) % LOCONET_RX_RINGBUFFER_Size];
      break;
  }

  // Check if we have all the bytes for this message
  if (writer <= reader + message_size) {
    return 0;
  }

  // Get bytes for passing (and build checksum)
  uint8_t data[message_size - 2];
  uint8_t start = reader + 1;
  uint8_t checksum = opcode.byte;

  for (uint8_t index = 0; index < message_size - 2; index++) {
    data[index] = buffer[(start + index) % LOCONET_RX_RINGBUFFER_Size];
    checksum ^= data[index];
  }

  // Verify checksum
  checksum ^= buffer[(start + message_size - 2) % LOCONET_RX_RINGBUFFER_Size];
  if (checksum != 0xFF) {
    // Advance reader
    loconet_rx_ringbuffer.reader = (reader + message_size) % LOCONET_RX_RINGBUFFER_Size;
    return 0;
  }

  // Handle message
  switch(opcode.bits.OPCODE) {
    case 0x04: // Length 0
      (*ln_messages_0[opcode.bits.NUMBER])();
      break;
    case 0x05: // Length 2
      (*ln_messages_2[opcode.bits.NUMBER])(data[0], data[1]);
      break;
    case 0x06: // Length 4
      (*ln_messages_4[opcode.bits.NUMBER])(data[0], data[1], data[2], data[3]);
      break;
    case 0x07: // Variable length
      (*ln_messages_n[opcode.bits.NUMBER])(&data[1], message_size - 3);
      break;
  }

  // Advance reader
  loconet_rx_ringbuffer.reader = (reader + message_size) % LOCONET_RX_RINGBUFFER_Size;

  // Return that we have processed a message
  return 1;
}

//-----------------------------------------------------------------------------
// Dummy handlers
void ln_handler_dummy_0(void)
{
}

void ln_handler_dummy_2(uint8_t a, uint8_t b)
{
  (void)a;
  (void)b;
}

void ln_handler_dummy_4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  (void)a;
  (void)b;
  (void)c;
  (void)d;
}

void ln_handler_dummy_n(uint8_t *d, uint8_t l)
{
  (void)d;
  (void)l;
}
