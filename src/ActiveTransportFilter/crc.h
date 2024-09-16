#include <ntddk.h>

#define CRC16_SEED 0x5bc3

//
// Take the CRC16 of a databuffer
//
UINT16 crc16(const VOID *buf, size_t len);

