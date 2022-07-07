@echo off 
set project_name=CH559USB
set xram_size=0x0800
set xram_loc=0x0600
set code_size=0xEFFF
set dfreq_sys=48000000

if not exist "config.h" echo //add your personal defines here > config.h

set CFLAGS=-V -mmcs51 --model-large --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I/ -DFREQ_SYS=%dfreq_sys%

sdcc -c %CFLAGS% main.c
sdcc -c %CFLAGS% util.c
sdcc -c %CFLAGS% USBHost.c
sdcc -c %CFLAGS% USBHid.c
sdcc -c %CFLAGS% USBHub.c
sdcc -c %CFLAGS% USBFtdi.c
sdcc -c %CFLAGS% uart.c

set OBJS=main.rel util.rel USBHost.rel USBHid.rel USBHub.rel USBFtdi.rel uart.rel

sdcc %OBJS% %CFLAGS% -o %project_name%.ihx

packihx %project_name%.ihx > %project_name%.hex

hex2bin -c %project_name%.hex

del %project_name%.lk
del %project_name%.map
del %project_name%.mem
del %project_name%.ihx

del *.asm
del *.lst
del *.rel
del *.rst
del *.sym
