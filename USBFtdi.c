#include "CH559.h"
#include "USBHost.h"
#include "util.h"
#include "uart.h"
#include "protocol.h"

#define FT232_VENDOR_ID       0x0403
#define FT232_PRODUCT_ID      0x6001

#define FTDI_CMD_RESET        0x00
#define FTDI_CMD_SET_FLOW     0x01
#define FTDI_CMD_SET_MHS      0x02 // Modem handshaking
#define FTDI_CMD_SET_BAUDRATE 0x03
#define FTDI_CMD_SET_LINE_CTL 0x04
#define FTDI_CMD_GET_MDMSTS   0x05 // Modem status

__code unsigned char  ftdi_ResetRequest[] = {USB_REQ_TYP_OUT | USB_REQ_TYP_VENDOR | USB_REQ_RECIP_INTERF, FTDI_CMD_RESET, 0, /*interface*/1 , 0, 0, 0, 0};
__code unsigned char  ftdi_SetBaudRateRequest[] = {USB_REQ_TYP_OUT | USB_REQ_TYP_VENDOR | USB_REQ_RECIP_INTERF, FTDI_CMD_SET_BAUDRATE, 0, /*interface*/0 , 0, 0, 0, 0};
__code unsigned char  ftdi_SetCharCodingRequest[] = {USB_REQ_TYP_OUT | USB_REQ_TYP_VENDOR | USB_REQ_RECIP_INTERF, FTDI_CMD_SET_LINE_CTL, 0, /*interface*/0 , 0, 0, 0, 0};

static uint32_t calculate_baudrate(uint32_t baudrate, uint16_t *wValue, uint16_t *wIndex)
{
    #define FTDI_BASE_CLK (3000000)

    uint32_t baudrate_real;
    if (baudrate > 2000000) {
        // set to 3000000
        *wValue = 0;
        *wIndex = 0;
        baudrate_real = 3000000;
    } else if (baudrate >= 1000000) {
        // set to 1000000
        *wValue = 1;
        *wIndex = 0;
        baudrate_real = 1000000;
    } else {
        const float ftdi_fractal[] = {0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1};
        const uint8_t ftdi_fractal_bits[] = {0, 0x03, 0x02, 0x04, 0x01, 0x05, 0x06, 0x07};
        uint16_t divider_n = FTDI_BASE_CLK / baudrate; // integer value
        int ftdi_fractal_idx = 0;
        float divider = FTDI_BASE_CLK / (float)baudrate; // float value
        float divider_fractal = divider - (float)divider_n;

        // Find closest bigger FT23x fractal divider
        for (ftdi_fractal_idx = 0; ftdi_fractal[ftdi_fractal_idx] <= divider_fractal; ftdi_fractal_idx++) {};

        // Calculate baudrate errors for two closest fractal divisors
        int diff1 = baudrate - (int)(FTDI_BASE_CLK / (divider_n + ftdi_fractal[ftdi_fractal_idx]));     // Greater than required baudrate
        int diff2 = (int)(FTDI_BASE_CLK / (divider_n + ftdi_fractal[ftdi_fractal_idx - 1])) - baudrate; // Lesser than required baudrate

        // Chose divider and fractal divider with smallest error
        if (diff2 < diff1) {
            ftdi_fractal_idx--;
        } else {
            if (ftdi_fractal_idx == 8) {
                ftdi_fractal_idx = 0;
                divider_n++;
            }
        }

        baudrate_real = FTDI_BASE_CLK / (float)((float)divider_n + ftdi_fractal[ftdi_fractal_idx]);
        *wValue = ((0x3FFFF) & divider_n) | (ftdi_fractal_bits[ftdi_fractal_idx] << 14);
        *wIndex = ftdi_fractal_bits[ftdi_fractal_idx] >> 2;
    }
    DEBUG_OUT("FT23x - wValue: 0x%04X wIndex: 0x%04X\n", *wValue, *wIndex);
    DEBUG_OUT("FT23x - Baudrate required: %ld, set: %ld\n", baudrate, baudrate_real);

    return baudrate_real;
}

static unsigned char ftdi_reset(PXUSBdevice usbDevice)
{
	unsigned char s;
    unsigned short len;
	fillTxBuffer(ftdi_ResetRequest, sizeof(ftdi_ResetRequest));
    s = hostCtrlTransfer(usbDevice, receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);             
	return s;                          
}

static unsigned char ftdi_setBaudRate(PXUSBdevice usbDevice, uint32_t baudRate)
{
	PXUSB_SETUP_REQ pSetupReq = ((PXUSB_SETUP_REQ)TxBuffer);
    uint16_t wIndex, wValue;
    calculate_baudrate(baudRate, &wValue, &wIndex);
	unsigned char s;
    unsigned short len;
	fillTxBuffer(ftdi_SetBaudRateRequest, sizeof(ftdi_SetBaudRateRequest));
    pSetupReq->wValueL = wValue;
    pSetupReq->wValueH = wValue >> 8;
    pSetupReq->wIndexL = wIndex;
    pSetupReq->wIndexH = wIndex >> 8;
    s = hostCtrlTransfer(usbDevice, receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);             
	return s;                          
}

static unsigned char ftdi_setCharCoding(PXUSBdevice usbDevice, uint8_t dataBits, uint8_t parityType, uint8_t charFormat)
{
	PXUSB_SETUP_REQ pSetupReq = ((PXUSB_SETUP_REQ)TxBuffer);
    uint16_t wValue = dataBits | (parityType << 8) | (charFormat << 11);
	unsigned char s;
    unsigned short len;
	fillTxBuffer(ftdi_SetCharCodingRequest, sizeof(ftdi_SetCharCodingRequest));
    pSetupReq->wValueL = wValue;
    pSetupReq->wValueH = wValue >> 8;
    s = hostCtrlTransfer(usbDevice, receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);             
	return s;                          
}

int ftdi_check(PXUSBdevice usbDevice)
{
    return usbDevice->VendorID == FT232_VENDOR_ID && usbDevice->ProductID == FT232_PRODUCT_ID;
}

void ftdi_initialize(PXUSBdevice usbDevice)
{
	unsigned char s;
    s = ftdi_reset(usbDevice);
    if (s == ERR_SUCCESS) {
        DEBUG_OUT("Got FTDI reset response\n");
        DEBUG_OUT_USB_BUFFER(receiveDataBuffer);
    }
    s = ftdi_setBaudRate(usbDevice, 230400);
    if (s == ERR_SUCCESS) {
        DEBUG_OUT("Got FTDI set baud rate response\n");
        DEBUG_OUT_USB_BUFFER(receiveDataBuffer);
    }
    s = ftdi_setCharCoding(usbDevice, 8, 0, 0);
    if (s == ERR_SUCCESS) {
        DEBUG_OUT("Got FTDI set char coding response\n");
        DEBUG_OUT_USB_BUFFER(receiveDataBuffer);
    }
}

void ftdi_inHandler(PXHIDdevice hidDevice, PXUCHAR buf, unsigned char len) __reentrant
{
    PXUSBdevice usbDevice = hidDevice->usbDevice;
#if 0
    if (len >= 2) {
        DEBUG_OUT("Got FTDI input: modem 0x%02x, line 0x%02x\n", buf[0], buf[1]);
        for (int i = 2; i < len; ++i) {
            DEBUG_OUT(" %02x", buf[i]);
        }
        DEBUG_OUT("\n");
    }
#endif
    if (len > 2) {
        sendProtocolMSG(MSG_TYPE_UART_IN, len, 0, 0, 0, buf);
    }
}
