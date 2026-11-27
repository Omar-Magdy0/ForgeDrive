#pragma once
#include <stdint.h>
/**
 * @brief ABFV0 (Asynchronous Binary Frame version 0)
 *
 * Frame format:
 * [0x00 SYNC_BYTE][1Byte ServiceID][1Byte Payload Size][1Byte MCOBS_CODE][CRC8][MCOBS_ENCODED(Payload)]
 * Total Hdr: 5 bytes (0x00 + Size + ServiceID + MCOBS_Code + CRC8)
 */
/**
 * @brief ABF (Asynchronous Binary Frame) Protocol Handler
 *
 * Handles encoding and decoding of ABF protocol frames with MCOBS encoding
 * and CRC-8 AUTOSAR error detection.
 */
// ================== CONSTANTS ==================
#define ABF_HDR_SZ 5
#ifdef __cplusplus
extern "C" {
#endif
// ================== ENUMS ==================
typedef enum {
  AbfId_Rpc       = 120,
  AbfId_DaqStream = 121,
  AbfId_DaqCodec  = 127,
} AbfId;
typedef enum { AbfErr_Ok = 0, AbfErr_PayloadIncomplete = 1, AbfErr_PayloadLarger = 2, AbfErr_HdrCrc = 3, AbfErr_RxBufSz = 4, AbfErr_Unkown = 255 } AbfErr;
typedef enum {
  AbfReadState_HuntSync    = 0, // Looking for sync byte (0x00)
  AbfReadState_ReadHdr     = 1, // Got sync, expecting length byte
  AbfReadState_ReadPayload = 2, // Reading payload data
} AbfReadState;
typedef enum { AbfReadEvent_None = 0, AbfReadEvent_Frame = 1, AbfReadEvent_Err = 2 } AbfReadEvent;
typedef struct {
  uint8_t sync;
  uint8_t sz;
  uint8_t id;
  uint8_t mcobsCode;
  uint8_t crc;
} AbfHdr;
typedef struct {
  uint8_t      hdrPos;
  AbfReadState state;
  uint8_t      bBufPos;
  uint8_t      nextZero;
} AbfReader;
// Api accessible
typedef struct {
  AbfErr   err;
  uint8_t  hdr[5];
  uint8_t* bBuf;
  uint8_t  bBufSz;
  uint8_t  pos;
} AbfRead;
uint8_t      Crc8CalculateAutosar(const uint8_t* data, uint8_t len);
AbfHdr       Abf_HdrDecode(uint8_t* hdr);
AbfReadEvent Abf_Read(AbfReader* reader, const uint8_t* data, uint16_t length, AbfRead* read);
AbfErr       Abf_Write(uint8_t* hdr, uint8_t* pBuf, uint8_t pBufSz, uint8_t id);
void         Abf_InitReader(AbfReader* reader);
#ifdef __cplusplus
}
#endif