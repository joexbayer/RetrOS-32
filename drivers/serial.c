/**
 * @file serial.c
 * @author Joe Bayer (joexbayer)
 * @brief Serial COM1 implementation for QEMU terminal output.
 * @version 0.1
 * @date 2022-08-07
 * @see https://wiki.osdev.org/Serial_Ports
 * @copyright Copyright (c) 2022
 * 
 */

#include <serial.h>
#include <io.h>
#include <util.h>
#include <stdarg.h>

#define PORT 0x3f8          // COM1
#define MAX_FMT_STR_SIZE 50

void serial_put(char a)
{
    while ((inportb(PORT + 5) & 0x20) == 0);

   	outportb(PORT, a);
}

void dbg(char* str)
{
	for (size_t i = 0; i < strlen(str); i++)
		serial_put(str[i]);
	serial_put('\n');
}

void init_serial()
{
    outportb(PORT + 1, 0x00);    // Disable all interrupts
	outportb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outportb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outportb(PORT + 1, 0x00);    //                  (hi byte)
	outportb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
	outportb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outportb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
	outportb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
	outportb(PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

	// Check if serial is faulty (i.e: not same byte as sent)
	if(inportb(PORT + 0) != 0xAE) {
		twriteln("[warning] Serial ports could not be configured.");
	}

    outportb(PORT + 4, 0x0F);
}
