#pragma once
#include <stdint.h>
/**
 * @brief ABFV0 (Asynchronous Binary Frame version 0)
 *
 * Frame format:
 * [0x00 SYNC_BYTE][1Byte ServiceID][1Byte Payload Size][1Byte MCOBS_CODE][CRC8][MCOBS_ENCODED(Payload)]
 * Total Header: 5 bytes (0x00 + Size + ServiceID + MCOBS_Code + CRC8)
 */
/**
 * @brief ABF (Asynchronous Binary Frame) Protocol Handler
 *
 * Handles encoding and decoding of ABF protocol frames with MCOBS encoding
 * and CRC-8 AUTOSAR error detection.
 */
// ================== CONSTANTS ==================
#define ABF_HEADER_SIZE      5
#define ABF_SYNC_BYTE        0x00
#define ABF_MAX_PAYLOAD_SIZE 255
#ifdef __cplusplus
extern "C" {
#endif
// ================== ENUMS ==================
typedef enum {
  AbfId_RPC       = 120,
  AbfId_DAQStream = 121,
  AbfId_DAQCodec  = 127,
} AbfId;
typedef enum {
  AbfErr_OK                           = 0,
  AbfErr_PAYLOAD_INCOMPLETE           = 1,
  AbfErr_PAYLOAD_LARGER_THAN_EXPECTED = 2,
  AbfErr_HEADER_CRC_MISMATCH          = 3,
  AbfErr_RX_BUFFER_SMALL              = 4,
  AbfErr_UNKNOWN                      = 255
} AbfErr;
typedef enum {
  AbfReadState_HUNT_SYNC    = 0, // Looking for sync byte (0x00)
  AbfReadState_READ_HEADER  = 1, // Got sync, expecting length byte
  AbfReadState_READ_PAYLOAD = 2, // Reading payload data
} AbfReadState;
typedef enum { AbfReadEvent_NONE = 0, AbfReadEvent_FRAME = 1, AbfReadEvent_ERROR = 2 } AbfReadEvent;
typedef struct {
  uint8_t sync;
  uint8_t size;
  uint8_t id;
  uint8_t mcobs_code;
  uint8_t crc;
} AbfHeader_t;
typedef struct {
  uint8_t      header_pos;
  AbfReadState state;
  uint8_t      b_buf_pos;
  uint8_t      next_zero;
} AbfReader_t;
// Api accessible
typedef struct {
  AbfErr   err;
  uint8_t  header[5];
  uint8_t* b_buf;
  uint8_t  b_buf_sz;
  uint8_t  pos;
} AbfRead_t;
uint8_t      crc8Calculate_autosar(const uint8_t* data, uint8_t len);
AbfHeader_t  abf_header_decode(uint8_t* header);
AbfReadEvent abf_read(AbfReader_t* reader, const uint8_t* data, uint16_t length, AbfRead_t* read);
AbfErr       abf_write(uint8_t* header, uint8_t* p_buf, uint8_t p_buf_sz, uint8_t id);
void         abf_init_reader(AbfReader_t* reader);
#ifdef __cplusplus
}
#endif