#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "CH559.h"
#include "util.h"
#include "uart.h"

typedef enum {
    STATE_SOP,
    STATE_LEN1,
    STATE_LEN2,
    STATE_HDR,
    STATE_PAYLOAD,
    STATE_EOP
} State;

unsigned char __xdata packet[128];

#define MSGTYPE_OFFSET  3
#define HDR_START       3
#define PAYLOAD_START   11

int index = 0;
int length;
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
            state = STATE_HDR;
            break;
        case STATE_HDR:
            packet[index++] = in;
            if (index == PAYLOAD_START) {
                if (length > 0) {
                    remaining = length;
                    state = STATE_PAYLOAD;
                }
                else {
                    state = STATE_EOP;
                }
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
                DEBUG_OUT("IN: msgtype %02x, type %02x, length %d\n", packet[MSGTYPE_OFFSET], packet[4], length);
                for (int i = HDR_START; i < PAYLOAD_START; ++i)
                    DEBUG_OUT(" %02x", packet[i]);
                DEBUG_OUT("\n");
                if (length > 0) {
                    for (int i = PAYLOAD_START; i < payloadEnd; ++i)
                        DEBUG_OUT(" %02x", packet[i]);
                    DEBUG_OUT("\n");
                }
                // handle complete packet
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
