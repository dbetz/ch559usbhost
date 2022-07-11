#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "CH559.h"
#include "util.h"
#include "uart.h"
#include "USBHost.h"
#include "protocol.h"

typedef enum {
    STATE_SOP,
    STATE_LEN1,
    STATE_LEN2,
    STATE_TYPE,
    STATE_PAYLOAD,
    STATE_EOP
} State;

unsigned char __xdata packet[128];

#define PAYLOAD_START   4

int index = 0;
int length;
unsigned char type;
int remaining;
State state = STATE_SOP;

void processUart(){
    while(RI){
        unsigned char in = SBUF;
        RI=0;
        switch (state) {
        case STATE_SOP:
            index = 0;
            if (in == 0xFE) {
                packet[index++] = in;
                state = STATE_LEN1;
            }
            break;
        case STATE_LEN1:
            length = in;
            packet[index++] = in;
            state = STATE_LEN2;
            break;
        case STATE_LEN2:
            length |= in << 8;
            packet[index++] = in;
            state = STATE_TYPE;
            break;
        case STATE_TYPE:
            type = in;
            packet[index++] = in;
            if (length > 0) {
                remaining = length;
                state = STATE_PAYLOAD;
            }
            else {
                state = STATE_EOP;
            }
            break;
        case STATE_PAYLOAD:
            packet[index++] = in;
            if (--remaining <= 0) {
                state = STATE_EOP;
            }
            break;
        case STATE_EOP:
            if (in == '\n') {
                int payloadEnd = PAYLOAD_START + length;
                packet[index++] = in;
                DEBUG_OUT("IN: type %02x, length %d\n", type, length);
                if (length > 0) {
                    for (int i = PAYLOAD_START; i < payloadEnd; ++i)
                        DEBUG_OUT(" %02x", packet[i]);
                    DEBUG_OUT("\n");
                }
                switch (type) {
                case MSG_TYPE_SERIAL_SET_CONFIG:
                    if (length == sizeof(SerialSetLineConfig)) {
                        SerialSetLineConfig __xdata *config = (SerialSetLineConfig __xdata *)&packet[PAYLOAD_START];
                        PXUSBdevice usbDevice = getUSBdevice(config->port);
                        if (usbDevice != NULL) {
                            uint32_t baudRate = (uint32_t)config->baudRate0;
                            baudRate |= (uint32_t)config->baudRate1 << 8;
                            baudRate |= (uint32_t)config->baudRate2 << 16;
                            baudRate |= (uint32_t)config->baudRate3 << 24;
                            ftdi_setBaudRate(usbDevice, baudRate);
                            ftdi_setCharCoding(usbDevice, config->dataBits, config->parityType, config->charFormat);
                        }
                    }
                    break;
                case MSG_TYPE_SERIAL_OUT:
                    break;
                default:
                    break;
                }
            }
            state = STATE_SOP;
            break;
        default:
            state = STATE_SOP;
            break;
        }
    }
}

void sendProtocolMSG(unsigned char msgtype, unsigned short length, unsigned char type, unsigned char device, unsigned char endpoint, unsigned char __xdata *msgbuffer){
    unsigned short i;
    putchar(0xFE);	
	putchar(length);
	putchar((unsigned char)(length>>8));
	putchar(msgtype);
	putchar(type);
	putchar(device);
	putchar(endpoint);
	putchar(0);
	putchar(0);
	putchar(0);
	putchar(0);
	for (i = 0; i < length; i++)
	{
		putchar(msgbuffer[i]);
	}
	putchar('\n');
}

void sendHidPollMSG(unsigned char msgtype, unsigned short length, unsigned char type, unsigned char device, unsigned char endpoint, unsigned char __xdata *msgbuffer,unsigned char idVendorL,unsigned char idVendorH,unsigned char idProductL,unsigned char idProductH){
    unsigned short i;
    putchar(0xFE);	
	putchar(length);
	putchar((unsigned char)(length>>8));
	putchar(msgtype);
	putchar(type);
	putchar(device);
	putchar(endpoint);
	putchar(idVendorL);
	putchar(idVendorH);
	putchar(idProductL);
	putchar(idProductH);
	for (i = 0; i < length; i++)
	{
		putchar(msgbuffer[i]);
	}
	putchar('\n');
}

uint8_t __xdata debugBuf[256];

void debug_out(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int length = vsprintf(debugBuf, fmt, args);
    int start = 0;
    va_end(args);
    while (length > 0) {
        int count = (length > 64 ? 64 : length);
        sendProtocolMSG(MSG_TYPE_DEBUG, count, 0, 0, 0, &debugBuf[start]);
        length -= count;
        start += count;
    }
}
