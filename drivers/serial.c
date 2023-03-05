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
    while ((inportb(PORT + 5) & 0x20) == 0){};

   	outportb(PORT, a);
}

void serial_write(char* str)
{
	for (int i = 0; i < strlen(str); i++)
		serial_put(str[i]);
}

#define _KDEBUG

/**
 * Writes the given string with formats to screen on give location.
 * @param int x coordinate
 * @param int y coordinate
 * @param char* format string
 * @param ... variable parameters
 * @return number of bytes written
 */
int32_t serial_printf(char* fmt, ...)
{
	int written = 0;
	#ifdef _KDEBUG
	CLI();
	va_list args;

	char str[MAX_FMT_STR_SIZE];
	int num = 0;

	va_start(args, fmt);

	while (*fmt != '\0') {
		switch (*fmt)
		{
			case '%':
				memset(str, 0, MAX_FMT_STR_SIZE);
				switch (*(fmt+1))
				{
					case 'd':
						num = va_arg(args, int);
						itoa(num, str);
						serial_write(str);
						break;
					case 'i': ;
						num = va_arg(args, int);
						unsigned char bytes[4];
						bytes[0] = (num >> 24) & 0xFF;
						bytes[1] = (num >> 16) & 0xFF;
						bytes[2] = (num >> 8) & 0xFF;
						bytes[3] = num & 0xFF;
						serial_printf("%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						serial_write(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						serial_write(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						serial_put(char_arg);
						break;
					
					default:
						break;
				}
				fmt++;
				break;
			default:  
				serial_put(*fmt);
			}
        fmt++;
    }
	STI();
	#endif
	return written;
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
		return;
	}

    outportb(PORT + 4, 0x0F);

	serial_printf("[%s] Serial debugging activated %d!\n", "Serial", 20);
}
