#include <thread>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <signal.h>

#include "SerialPort.h"

using namespace std;

std::unique_ptr<SerialPort> serialport = nullptr;
std::unique_ptr<std::thread> receiveThread = nullptr;
volatile bool exitFlag = false;

enum State {
    STATE_SOP,
    STATE_LEN1,
    STATE_LEN2,
    STATE_HDR,
    STATE_PAYLOAD,
    STATE_EOP
};

static const int MSGTYPE_OFFSET = 3;
static const int HDR_START = 3;   // SOP, LEN1, and LEN2
static const int PAYLOAD_START = 11; // SOP, LEN1, LEN2, and 8 bytes of header

void SerialReadThread()
{
    int index = 0;
    while (!exitFlag) {
        uint8_t packet[1024];
        uint8_t rxBuffer[1024];
        int length;
        int remaining;
        State state = STATE_SOP;
        size_t readSize = serialport->Read(rxBuffer, sizeof(rxBuffer));
        for (int i = 0; i < readSize; ++i) {
            uint8_t in = rxBuffer[i];
            //printf("%02x ", in);
            switch (state) {
            case STATE_SOP:
                index = 0;
                if (in == 0xFE) {
                    packet[index++] = rxBuffer[i];
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
                    if (packet[MSGTYPE_OFFSET] == 9) {
                        for (int i = PAYLOAD_START; i < payloadEnd; ++i)
                            putchar(packet[i]);
                    }
                    else {
                        printf("msgtype %02x, type %02x, length %d\n", packet[MSGTYPE_OFFSET], packet[4], length);
                        for (int i = HDR_START; i < PAYLOAD_START; ++i)
                            printf(" %02x", packet[i]);
                        putchar('\n');
                        if (length > 0) {
                            for (int i = PAYLOAD_START; i < payloadEnd; ++i)
                                printf(" %02x", packet[i]);
                            putchar('\n');
                        }
                        // handle complete packet
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
}

bool Start()
{
    auto serialportName = "/dev/ttyUSB0";

    cout << "Using " << serialportName << endl;
    serialport = make_unique<SerialPort>(serialportName);
    if (!serialport->Open()) {
        cout << "Failed to connect to RDM" << endl;
        serialport = nullptr;
        return false;
    }

    exitFlag = false;
    receiveThread = make_unique<thread>(&SerialReadThread);
    
    return true;
}

void Stop()
{
    if (receiveThread != nullptr) {
        cout << "Stopping the serial receive thread" << endl;
        exitFlag = 1;
        receiveThread->join();
        receiveThread = nullptr;
    }
    if (serialport != nullptr) {
        cout << "Closing the serial port" << endl;
        serialport->Close();
        serialport = nullptr;
    }
}

void signalHandler(int signum)
{
    switch (signum) {
    case SIGINT:
        cout << "Caught Ctrl-C" << endl;
        Stop();
        cout << "Exiting. " << endl;
        exit(EXIT_SUCCESS);
        break;
    case SIGTSTP:
        cout << "Caught SIGTSTP" << endl;
        Stop();
        cout << "Exiting. " << endl;
        exit(EXIT_SUCCESS);
        break;
    }
}

int main(int argc, const char *argv[])
{
    Start();
    while (!exitFlag);
    return 0;
}