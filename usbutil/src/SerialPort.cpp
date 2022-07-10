#include "SerialPort.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

// You may have to do "newgrp dialout" and "usermod -a -G dialout $USER"

class SerialPort::impl
{
public:
    impl(std::string portName)  : portName(portName) {}
    bool Open();
    void Close();
    bool Write(uint8_t const * const buffer, size_t len);
    size_t Read(uint8_t* buffer, size_t len);
private:
    std::string portName;
    int fd;
};

template class std::unique_ptr<SerialPort::impl>;

bool SerialPort::impl::Open()
{
    fd = open(portName.c_str(), O_RDWR | O_NONBLOCK | O_NDELAY);
    if (fd < 0)
    {
        //INFO_MSG(strerror(errno));
        return false;
    }

    fcntl(fd, F_SETFL, 0);

    termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B230400);
    cfsetospeed(&options, B230400);
    options.c_iflag &= ~(BRKINT | INPCK | ISTRIP | IXON | ICRNL);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);
    options.c_oflag &= ~OPOST;
    if(tcsetattr(fd, TCSANOW, &options) < 0)
    {
        //DEBUG_MSG(strerror(errno));
        close(fd);
        return false;
    }
    tcflush(fd, TCIFLUSH);
    return true;
}

void SerialPort::impl::Close()
{
    close(this->fd);
}

#include <iostream>

bool SerialPort::impl::Write(uint8_t const * const buffer, size_t len)
{
    size_t result = write(this->fd, buffer, len);
    if (result != len) std::cout << "write returned " << result << std::endl;
    return result != -1;
    //return write(this->fd, buffer, len) != -1;
}

size_t SerialPort::impl::Read(uint8_t* buffer, size_t len)
{
    return read(this->fd, buffer, len);
}

SerialPort::SerialPort(std::string portName) : pimpl(new impl(portName)) { }
SerialPort::~SerialPort() = default;
bool SerialPort::Open() { return pimpl->Open(); }
void SerialPort::Close() { pimpl->Close(); }
bool SerialPort::Write(uint8_t const * const buffer, size_t len) { return pimpl->Write(buffer, len); }
size_t SerialPort::Read(uint8_t* buffer, size_t len) { return pimpl->Read(buffer, len); }
