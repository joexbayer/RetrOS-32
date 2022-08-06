#include <util.h>

int kernel_size = 0;

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

uint32_t strncmp(const char* str, const char* str2, uint32_t len)
{
	return memcmp((uint8_t*)str, (uint8_t*)str2, len);
}

uint32_t memcmp(const uint8_t* ptr, const uint8_t* ptr2, uint32_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		if(ptr[i] != ptr2[i])
			return 0;
	}

	return 1;
}

/* TODO: Move some functions into own files. */


/*
  Highly optimized memcpy and memset functions.
  from: https://forum.osdev.org/viewtopic.php?t=18119
*/

/**
 * void *dest, const void *src, size_t n
 *
 * @return void*
 */
void* memcpy(void *dest, const void *src, size_t n)
{
	uint32_t num_dwords	= n / 4;
	uint32_t num_bytes	= n % 4;
	uint32_t* dest32	= (uint32_t*) dest;
	uint32_t* src32 	= (uint32_t*) src;
	uint8_t* dest8 		= ((uint8_t*) dest) + num_dwords * 4;
	uint8_t* src8 		= ((uint8_t*) src) + num_dwords * 4;
	uint32_t i;

	for (i=0; i < num_dwords; i++)
	{
		dest32[i] = src32[i];
	}

	for (i=0; i < num_bytes; i++)
	{
		dest8[i] = src8[i];
	}

	return dest;
}
/**
 * void *dest, int val, size_t n
 *
 * @return void*
 */
void* memset(void *dest, int val, size_t n)
{
	uint32_t num_dwords = n / 4;
	uint32_t num_bytes  = n % 4;
	uint32_t* dest32    = (uint32_t*) dest;
	uint8_t* dest8      = ((uint8_t*) dest) + num_dwords * 4;
	uint8_t val8        = (uint8_t) val;
	uint32_t val32      = val|(val << 8) | (val << 16) | (val << 24);
	uint32_t i;

	for (i=0; i < num_dwords; i++)
	{
		dest32[i] = val32;
	}
	for (i=0; i < num_bytes; i++)
	{
		dest8[i] = val8;
	}

	return dest;
}

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
	sign = ((char)s[i] == '-') ? -1 : 1;
	if(s[i] == '-' || s[i] == '+')
		i++;

	for(n = 0; isdigit(s[i]); i++)
		n = 10 * n + (s[i] - '0');
	
	return n*sign;
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
	reverse(s);
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


/* IN and OUT for ports. */ 
uint8_t inportb(uint16_t p)
{
    uint8_t r;
    asm volatile("inb %%dx, %%al":"=a"(r):"d"(p));
    return r;
}

uint16_t inportw(uint16_t p)
{
    uint16_t r;
    asm volatile("inw %%dx, %%ax":"=a"(r):"d"(p));
    return r;
}

uint32_t inportl(uint16_t p)
{
    uint32_t r;
    asm volatile("inl %%dx, %%eax":"=a"(r):"d"(p));
    return r;
}

void outportb(uint16_t portid, uint8_t value)
{
    asm volatile("outb %%al, %%dx":: "d"(portid), "a"(value));
}

void outportw(uint16_t portid, uint16_t value)
{
    asm volatile("outw %%ax, %%dx":: "d"(portid), "a"(value));
}

void outportl(uint16_t portid, uint32_t value)
{
    asm volatile("outl %%eax, %%dx":: "d"(portid), "a"(value));
}

/* https://wiki.osdev.org/Random_Number_Generator */
static unsigned long int next = 1;  // NB: "unsigned long int" is assumed to be 32 bits wide
int rand(void)  // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (unsigned int) (next / 65536) % 32768;
}