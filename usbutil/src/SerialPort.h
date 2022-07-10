#pragma once

#include <string>
#include <stdint.h>
#include <memory>

class SerialPort
{
public:
    SerialPort(std::string portName);
    ~SerialPort();
    bool Open();
    void Close();
    bool Write(uint8_t const * const buffer, size_t len);
    size_t Read(uint8_t* buffer, size_t len);

private:
	class impl;
	std::unique_ptr<impl> pimpl;
	SerialPort(const SerialPort&) = delete;
	SerialPort& operator=(const SerialPort&) = delete;
};
