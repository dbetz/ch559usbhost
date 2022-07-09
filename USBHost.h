#ifndef __USBHOST_H__
#define __USBHOST_H__

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

typedef const unsigned char __code *PUINT8C;
typedef unsigned char __xdata *PXUCHAR;

#define ROOT_HUB_COUNT  2

#define MAX_EXHUB_PORT_COUNT    4
#define EXHUB_PORT_NONE         0xff
#define MAX_INTERFACE_COUNT     4
#define MAX_ENDPOINT_COUNT      4
#define MAX_EXHUB_LEVEL         1
#define ENDPOINT_OUT            0
#define ENDPOINT_IN             1

#define ERR_SUCCESS         0x00
#define ERR_USB_CONNECT     0x15
#define ERR_USB_DISCON      0x16
#define ERR_USB_BUF_OVER    0x17
#define ERR_USB_DISK_ERR    0x1F
#define ERR_USB_TRANSFER    0x20 
#define ERR_USB_UNSUPPORT   0xFB
#define ERR_USB_UNKNOWN     0xFE

#define ROOT_DEVICE_DISCONNECT  0
#define ROOT_DEVICE_CONNECTED   1
#define ROOT_DEVICE_FAILED      2

/*#define DEV_TYPE_KEYBOARD   ( USB_DEV_CLASS_HID | 0x20 )
#define DEV_TYPE_MOUSE      ( USB_DEV_CLASS_HID | 0x30 )
#define DEV_TYPE_JOYSTICK      ( USB_DEV_CLASS_HID | 0x40 )
#define DEV_TYPE_GAMEPAD      ( USB_DEV_CLASS_HID | 0x50 )*/

#define HID_SEG_KEYBOARD_MODIFIER_INDEX 0
#define HID_SEG_KEYBOARD_VAL_INDEX      1
#define HID_SEG_BUTTON_INDEX            2
#define HID_SEG_X_INDEX                 3
#define HID_SEG_Y_INDEX                 4
#define HID_SEG_WHEEL_INDEX             5
#define HID_SEG_COUNT                   6

typedef struct _EndPoint
{
    unsigned char EndpointAddr;
    unsigned short MaxPacketSize;
    
    unsigned char EndpointDir : 1;
    unsigned char TOG : 1;
} EndPoint, *PEndPoint;

typedef struct _HIDSegmentStructure
{
    unsigned char KeyboardReportId;
    unsigned char MouseReportId;
    
    struct
    {
        unsigned char start;
        unsigned char size;
        unsigned char count;
    } HIDSeg[HID_SEG_COUNT];
} HIDSegmentStructure;

#define HID_KEYBOARD_VAL_LEN           6
#define MAX_HID_KEYBOARD_BIT_VAL_LEN   15

typedef struct _KeyboardParseStruct
{
    unsigned char   KeyboardVal[HID_KEYBOARD_VAL_LEN];
    unsigned char   KeyboardBitVal[MAX_HID_KEYBOARD_BIT_VAL_LEN];
} KeyboardParseStruct;

typedef struct _Interface
{
    unsigned char       InterfaceClass;
    unsigned char       InterfaceProtocol;
    
    unsigned char       EndpointCount;
    EndPoint            endpoint[MAX_ENDPOINT_COUNT];
    
    HIDSegmentStructure HidSegStruct;   

    KeyboardParseStruct KeyboardParseStruct;
} Interface, *PInterface;

typedef Interface __xdata *PXInterface;

typedef struct _ConnectedDevice 
{
    unsigned char connected;
    unsigned char rootHub;
    unsigned char interface;
    unsigned char endPoint;
    unsigned long type;
} ConnectedDevice, *PConnectedDevice;

typedef ConnectedDevice __xdata *PXConnectedDevice;

typedef struct
{
    int             InUse;
    unsigned char   DeviceClass;
    unsigned char   MaxPacketSize0;
    
    unsigned short  VendorID;
    unsigned short  ProductID;
    unsigned short  bcdDevice;

    unsigned char   RootHubIndex;
    unsigned char   DeviceAddress;
    unsigned char   DeviceSpeed;
    unsigned char   InterfaceCount;
    //Interface       interface[MAX_INTERFACE_COUNT];
} USBdevice, *PUSBdevice;

typedef USBdevice __xdata *PXUSBdevice;

typedef struct _HIDdevice HIDdevice, *PHIDdevice;
typedef HIDdevice __xdata *PXHIDdevice;

struct _HIDdevice
{
    unsigned char index;
    PXUSBdevice usbDevice;
    unsigned char interface;
    unsigned char endPoint;
    unsigned long type;
    void (*inHandler)(PXHIDdevice hidDevice, PXUCHAR buf, unsigned char len) __reentrant;
};

void resetRootHub(unsigned char i);
void initUSB_Host();
unsigned char checkRootHubConnections();

void resetHubDevices(unsigned char hubindex);
void pollHIDdevice();

PXUSBdevice addUSBdevice(unsigned char rootHubIndex);
void selectHubPort(unsigned char rootHubIndex, unsigned char addr);
void fillTxBuffer(PUINT8C data, unsigned char len);
unsigned char hostCtrlTransfer(PXUSBdevice usbDevice, unsigned char __xdata *DataBuf, unsigned short *RetLen, unsigned short maxLength);
void DEBUG_OUT_USB_BUFFER(unsigned char __xdata *usbBuffer);
unsigned char initializeConnection(PXUSBdevice usbDevice);

#define RECEIVE_BUFFER_LEN    512
extern __xdata unsigned char receiveDataBuffer[RECEIVE_BUFFER_LEN];
extern __at(0x0100) unsigned char __xdata TxBuffer[MAX_PACKET_SIZE];
extern __at(0x0100) unsigned char __xdata TxBuffer[MAX_PACKET_SIZE];

// USBHid.c
void resetHubDevices(unsigned char hubindex);
void pollHIDdevice();
unsigned char getHIDDeviceReport(PXHIDdevice device);
void hid_inHandler(PXHIDdevice hidDevice, PXUCHAR buf, unsigned char len) __reentrant;

// USBHub.c
unsigned char hub_initialize(PXUSBdevice usbDevice);
unsigned char hub_getHubStatus(PXUSBdevice usbDevice);
unsigned char hub_setPortFeature(PXUSBdevice usbDevice, unsigned char port, unsigned char feature);
unsigned char hub_getPortStatus(PXUSBdevice usbDevice, unsigned char port, unsigned short *pStatus, unsigned short *pChange);
void hub_inHandler(PXHIDdevice hidDevice, PXUCHAR buf, unsigned char len) __reentrant;

// USBFtdi.c
int ftdi_check(PXUSBdevice usbDevice);
void ftdi_initialize(PXUSBdevice usbDevice);
void ftdi_inHandler(PXHIDdevice hidDevice, PXUCHAR buf, unsigned char len) __reentrant;

#endif