#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MSG_TYPE_CONNECTED          0x01
#define MSG_TYPE_DISCONNECTED       0x02
#define MSG_TYPE_ERROR              0x03
#define MSG_TYPE_DEVICE_POLL        0x04
#define MSG_TYPE_DEVICE_STRING      0x05
#define MSG_TYPE_DEVICE_INFO        0x06
#define MSG_TYPE_HID_INFO           0x07
#define MSG_TYPE_STARTUP            0x08
#define MSG_TYPE_DEBUG              0x09
#define MSG_TYPE_UART_CONNECTED     0x20
#define MSG_TYPE_UART_DISCONNECTED  0x21
#define MSG_TYPE_UART_SET_CONFIG    0x22
#define MSG_TYPE_UART_IN            0x23
#define MSG_TYPE_UART_OUT           0x24

typedef struct {
    uint8_t sop;
    uint8_t len0;
    uint8_t len1;
    uint8_t type;
} MessageHdr;

typedef struct {
    uint8_t port;
} SerialConnect;

typedef struct {
    uint8_t port;
    uint8_t baud0;
    uint8_t baud1;
    uint8_t baud2;
    uint8_t baud3;
    uint8_t dataBits;
    uint8_t parityType;
    uint8_t charFormat;
} SerialSetLineConfig;

typedef struct {
    uint8_t port;
} SerialWriteHdr;

#ifdef __cplusplus
}
#endif
