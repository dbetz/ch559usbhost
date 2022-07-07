@echo off 
set project_name=CH559USB
Rem This tool flashes the bin file directly to the ch559 chip, you need to install the libusb-win32 driver with the zadig( https://zadig.akeo.ie/ ) tool so the tool can access the usb device
chflasher.exe %project_name%.bin
