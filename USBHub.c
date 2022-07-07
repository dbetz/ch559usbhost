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

__code unsigned char hub_GetDescriptor[] =  {USB_REQ_TYP_IN  | USB_REQ_TYP_CLASS | USB_REQ_RECIP_DEVICE, HUB_GET_DESCRIPTOR, 0, USB_DESCR_TYP_HUB, 0, 0, sizeof(USB_HUB_DESCR), 0x00};
__code unsigned char hub_GetHubStatus[] =   {USB_REQ_TYP_IN  | USB_REQ_TYP_CLASS | USB_REQ_RECIP_DEVICE, HUB_GET_STATUS, 0, 0, 0, 0, sizeof(PORT_STATUS), 0x00};
__code unsigned char hub_SetPortFeature[] = {USB_REQ_TYP_OUT | USB_REQ_TYP_CLASS | USB_REQ_RECIP_OTHER,  HUB_SET_FEATURE, 0, 0, 0, 0, 0, 0x00};
__code unsigned char hub_GetPortStatus[] =  {USB_REQ_TYP_IN  | USB_REQ_TYP_CLASS | USB_REQ_RECIP_OTHER,  HUB_GET_STATUS, 0, 0, 0, 0, sizeof(PORT_STATUS), 0x00};

unsigned char hub_initialize(PXUSBdevice usbDevice)
{
	unsigned char s;
    unsigned short len;
    DEBUG_OUT("Found a hub\n");
    PXUSB_HUB_DESCR desc = (PXUSB_HUB_DESCR)receiveDataBuffer;
	fillTxBuffer(hub_GetDescriptor, sizeof(hub_GetDescriptor));
    s = hostCtrlTransfer(receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
    if (s != ERR_SUCCESS)
        return s;
    unsigned char bNbrPorts = desc->bNbrPorts;
    DEBUG_OUT("hub - bNbrPorts %d\n", bNbrPorts);
    hub_getHubStatus(usbDevice);
    for (int i = 1; i <= bNbrPorts; ++i) {
        hub_setPortFeature(usbDevice, i, HUB_PORT_POWER);
        hub_getPortStatus(usbDevice, i);
    }
	return s;                          
}

unsigned char hub_getHubStatus(PXUSBdevice usbDevice)
{
	unsigned char s;
    unsigned short len;
    PXPORT_STATUS desc = (PXPORT_STATUS)receiveDataBuffer;
	selectHubPort(usbDevice->RootHubIndex, usbDevice->DeviceAddress);
	fillTxBuffer(hub_GetHubStatus, sizeof(hub_GetHubStatus));
    s = hostCtrlTransfer(receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
    DEBUG_OUT("getHubStatus 0x%02x\n", s);
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
	selectHubPort(usbDevice->RootHubIndex, usbDevice->DeviceAddress);
	fillTxBuffer(hub_SetPortFeature, sizeof(hub_SetPortFeature));
    pSetupReq->wValueL = feature;
    pSetupReq->wIndexL = port;
    s = hostCtrlTransfer(receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
    DEBUG_OUT("setPortFeature 0x%02x\n", s);
	return s;                          
}

unsigned char hub_getPortStatus(PXUSBdevice usbDevice, unsigned char port)
{
	unsigned char s;
    unsigned short len;
    PUSB_SETUP_REQ pSetupReq = (PUSB_SETUP_REQ)TxBuffer;
    PXPORT_STATUS desc = (PXPORT_STATUS)receiveDataBuffer;
	selectHubPort(usbDevice->RootHubIndex, usbDevice->DeviceAddress);
	fillTxBuffer(hub_GetPortStatus, sizeof(hub_GetPortStatus));
    pSetupReq->wIndexL = port;
    s = hostCtrlTransfer(receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
    DEBUG_OUT("getPortStatus 0x%02x\n", s);
    if (s != ERR_SUCCESS)
        return s;
    DEBUG_OUT("hub - port %d status: status %02x%02x, change %02x%02x\n", port, desc->wPortStatusH, desc->wPortStatusL, desc->wPortChangeH, desc->wPortChangeL);
	return s;                          
}

void hub_inHandler(PXHIDdevice hidDevice, PXUCHAR buf, unsigned char len) __reentrant
{
    PXUSBdevice usbDevice = hidDevice->usbDevice;
    DEBUG_OUT("Got hub port status\n");
}


