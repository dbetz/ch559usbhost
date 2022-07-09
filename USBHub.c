#include "CH559.h"
#include "USBHost.h"
#include "util.h"
#include "uart.h"

typedef struct {
    unsigned char wPortStatusL;
    unsigned char wPortStatusH;
    unsigned char wPortChangeL;
    unsigned char wPortChangeH;
} PORT_STATUS, *PPORT_STATUS;

typedef PORT_STATUS __xdata *PXPORT_STATUS;

#define HUB_PORT_STATUS_CONNECTION      0x0001
#define HUB_PORT_STATUS_ENABLE          0x0002
#define HUB_PORT_STATUS_SUSPEND         0x0004
#define HUB_PORT_STATUS_OVER_CURRENT    0x0008
#define HUB_PORT_STATUS_RESET           0x0010
#define HUB_PORT_STATUS_POWER           0x0100
#define HUB_PORT_STATUS_LOW_SPEED       0x0200
#define HUB_PORT_STATUS_HIGH_SPEED      0x0400
#define HUB_PORT_STATUS_TEST            0x0800
#define HUB_PORT_STATUS_INDICATOR       0x1000

#define changed(change, mask)           (((change) & (mask)) == (mask))

__code unsigned char hub_GetDescriptor[] =    {USB_REQ_TYP_IN  | USB_REQ_TYP_CLASS | USB_REQ_RECIP_DEVICE, HUB_GET_DESCRIPTOR, 0, USB_DESCR_TYP_HUB, 0, 0, sizeof(USB_HUB_DESCR), 0};
__code unsigned char hub_GetHubStatus[] =     {USB_REQ_TYP_IN  | USB_REQ_TYP_CLASS | USB_REQ_RECIP_DEVICE, HUB_GET_STATUS, 0, 0, 0, 0, sizeof(PORT_STATUS), 0};
__code unsigned char hub_SetPortFeature[] =   {USB_REQ_TYP_OUT | USB_REQ_TYP_CLASS | USB_REQ_RECIP_OTHER,  HUB_SET_FEATURE, 0, 0, 0, 0, 0, 0};
__code unsigned char hub_ClearPortFeature[] = {USB_REQ_TYP_OUT | USB_REQ_TYP_CLASS | USB_REQ_RECIP_OTHER,  HUB_CLEAR_FEATURE, 0, 0, 0, 0, 0, 0};
__code unsigned char hub_GetPortStatus[] =    {USB_REQ_TYP_IN  | USB_REQ_TYP_CLASS | USB_REQ_RECIP_OTHER,  HUB_GET_STATUS, 0, 0, 0, 0, sizeof(PORT_STATUS), 0};

unsigned char hub_initialize(PXUSBdevice usbDevice)
{
	unsigned char s;
    unsigned short len;
    DEBUG_OUT("Found a hub\n");
    PXUSB_HUB_DESCR desc = (PXUSB_HUB_DESCR)receiveDataBuffer;
	fillTxBuffer(hub_GetDescriptor, sizeof(hub_GetDescriptor));
    s = hostCtrlTransfer(usbDevice, receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
    if (s != ERR_SUCCESS)
        return s;
    unsigned char bNbrPorts = desc->bNbrPorts;
    DEBUG_OUT("hub - bNbrPorts %d\n", bNbrPorts);
    hub_getHubStatus(usbDevice);
    for (int i = 1; i <= bNbrPorts; ++i) {
        hub_setPortFeature(usbDevice, i, HUB_PORT_POWER);
    }
	return s;                          
}

unsigned char hub_getHubStatus(PXUSBdevice usbDevice)
{
	unsigned char s;
    unsigned short len;
    PXPORT_STATUS desc = (PXPORT_STATUS)receiveDataBuffer;
	fillTxBuffer(hub_GetHubStatus, sizeof(hub_GetHubStatus));
    s = hostCtrlTransfer(usbDevice, receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
    if (s != ERR_SUCCESS)
        return s;
    DEBUG_OUT("hub - hub status: status %02x%02x, change %02x%02x\n", desc->wPortStatusH, desc->wPortStatusL, desc->wPortChangeH, desc->wPortChangeL);
	return s;                          
}

unsigned char hub_setPortFeature(PXUSBdevice usbDevice, unsigned char port, unsigned char feature)
{
	unsigned char s;
    unsigned short len;
    PUSB_SETUP_REQ pSetupReq = (PUSB_SETUP_REQ)TxBuffer;
    PXPORT_STATUS desc = (PXPORT_STATUS)receiveDataBuffer;
	fillTxBuffer(hub_SetPortFeature, sizeof(hub_SetPortFeature));
    pSetupReq->wValueL = feature;
    pSetupReq->wIndexL = port;
    s = hostCtrlTransfer(usbDevice, receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
	return s;                          
}

unsigned char hub_clearPortFeature(PXUSBdevice usbDevice, unsigned char port, unsigned char feature)
{
	unsigned char s;
    unsigned short len;
    PUSB_SETUP_REQ pSetupReq = (PUSB_SETUP_REQ)TxBuffer;
    PXPORT_STATUS desc = (PXPORT_STATUS)receiveDataBuffer;
	fillTxBuffer(hub_ClearPortFeature, sizeof(hub_ClearPortFeature));
    pSetupReq->wValueL = feature;
    pSetupReq->wIndexL = port;
    s = hostCtrlTransfer(usbDevice, receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
	return s;                          
}

unsigned char hub_getPortStatus(PXUSBdevice usbDevice, unsigned char port, unsigned short *pStatus, unsigned short *pChange)
{
	unsigned char s;
    unsigned short len;
    PUSB_SETUP_REQ pSetupReq = (PUSB_SETUP_REQ)TxBuffer;
    PXPORT_STATUS desc = (PXPORT_STATUS)receiveDataBuffer;
	fillTxBuffer(hub_GetPortStatus, sizeof(hub_GetPortStatus));
    pSetupReq->wIndexL = port;
    s = hostCtrlTransfer(usbDevice, receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
    if (s != ERR_SUCCESS)
        return s;
    DEBUG_OUT("hub - port %d status: status %02x%02x, change %02x%02x\n", port, desc->wPortStatusH, desc->wPortStatusL, desc->wPortChangeH, desc->wPortChangeL);
    *pStatus = desc->wPortStatusL | (desc->wPortStatusH << 8);
    *pChange = desc->wPortChangeL | (desc->wPortChangeH << 8);
	return s;                          
}

void hub_inHandler(PXHIDdevice hidDevice, PXUCHAR buf, unsigned char len) __reentrant
{
    PXUSBdevice usbDevice = hidDevice->usbDevice;
    
    DEBUG_OUT("Got hub port status:");
    for (int i = 0; i < len; ++i) {
        DEBUG_OUT(" %02x", buf[i]);
    }
    DEBUG_OUT("\n");
    
    unsigned short mask = buf[0]; // only support up to 15 ports
    if (len > 1) {
        mask |= buf[1] << 8;
    }
    
    for (int port = 1; port <= 15; ++port) {
        unsigned short portMask = 1 << port;
        if ((mask & portMask) != 0) {
            unsigned short status, change;
            unsigned char s;
            s = hub_getPortStatus(usbDevice, port, &status, &change);
            if (s == ERR_SUCCESS) {
                if (changed(change, HUB_PORT_STATUS_CONNECTION)) {
                    hub_clearPortFeature(usbDevice, port, HUB_C_PORT_CONNECTION);
                    if (status == (HUB_PORT_STATUS_CONNECTION | HUB_PORT_STATUS_POWER)) {
                        DEBUG_OUT("Hub port %d connected\n", port);
                        hub_setPortFeature(usbDevice, port, HUB_PORT_RESET);
                    }
                    else if (status == (HUB_PORT_STATUS_POWER)) {
                        DEBUG_OUT("Hub port %d disconnected\n", port);
                        usbDevice->InUse = FALSE;
                    }
                }
                if (changed(change, HUB_PORT_STATUS_RESET)) {
                    hub_clearPortFeature(usbDevice, port, HUB_C_PORT_RESET);
                    if (status == (HUB_PORT_STATUS_CONNECTION | HUB_PORT_STATUS_ENABLE | HUB_PORT_STATUS_POWER)) {
                        DEBUG_OUT("Hub port %d reset complete\n", port);
                        PXUSBdevice newUsbDevice = addUSBdevice(usbDevice->RootHubIndex);
                        initializeConnection(newUsbDevice);
                    }
                }
                if (changed(change, HUB_PORT_ENABLE)) {
                    hub_clearPortFeature(usbDevice, port, HUB_C_PORT_ENABLE);
                }
            }
        }
    }
}


