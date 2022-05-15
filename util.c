#include "util.h"

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

/*
	TODO:
	memcpy
	memset
	
*/

int isdigit(char c)
{
    if ((c>='0') && (c<='9')) return 1;
    return 0;
}

/*
 * Functions from Kerninghan/Ritchie - The C Programming Language
 */

void reverse(char s[])
{
	int c, i, j;

	for (i = 0, j = strlen(s)-1; i < j; i++, j--)
	{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

int atoi(char s[])
{
	int i, n, sign;
	for(i = 0; s[i] == ' '; i++) /* Skip white spaces */
		; 
	sign = (s[i] == "-") ? -1 : 1;
	if(s[i] == '-' || s[i] == '+')
		i++;

	for(n = 0; isdigit(s[i]); i++)
		n = 10 * n + (s[i] - '0');
	
	return n;
}

void itoa(int n, char s[])
{
	int i, sign;

	if ((sign = n) < 0)
		n = -n;
	i = 0;
	do {
		s[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);

	if(sign < 0)
		s[i++] = '-';
	
	s[i] = '\0';
	reverse(i);
}

void itohex(uint32_t n, char s[])
{
  uint32_t i, d;

  i = 0;
  do {
    d = n % 16;
    if (d < 10)
      s[i++] = d + '0';
    else
      s[i++] = d - 10 + 'a';
  } while ((n /= 16) > 0);
  s[i++] = 0;
  reverse(s);
}

uint32_t ntohl(uint32_t data)
{
  return (((data & 0x000000ff) << 24) |
          ((data & 0x0000ff00) << 8) |
          ((data & 0x00ff0000) >> 8) |
          ((data & 0xff000000) >> 24));
}

uint32_t htonl(uint32_t data)
{
  return ntohl(data);
}

uint16_t ntohs(uint16_t data)
{
  return (((data & 0x00ff) << 8) |
           (data & 0xff00) >> 8);
}

uint16_t htons(uint16_t data)
{
  return ntohs(data);
}


/* IN and OUT for ports. */ 
uint8_t inb(uint16_t p)
{
    uint8_t r;
    asm volatile("inb %%dx, %%al":"=a"(r):"d"(p));
    return r;
}

uint16_t inw(uint16_t p)
{
    uint16_t r;
    asm volatile("inw %%dx, %%ax":"=a"(r):"d"(p));
    return r;
}

uint32_t inl(uint16_t p)
{
    uint32_t r;
    asm volatile("inl %%dx, %%eax":"=a"(r):"d"(p));
    return r;
}

void outb(uint16_t portid, uint8_t value)
{
    asm volatile("outb %%al, %%dx":: "d"(portid), "a"(value));
}

void outw(uint16_t portid, uint16_t value)
{
    asm volatile("outw %%ax, %%dx":: "d"(portid), "a"(value));
}

void outl(uint16_t portid, uint32_t value)
{
    asm volatile("outl %%eax, %%dx":: "d"(portid), "a"(value));
}