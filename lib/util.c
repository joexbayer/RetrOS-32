#ifdef __cplusplus
extern "C" {
#endif

#include <util.h>

int cli_cnt = 0;

float cos_60[60] = {6.123233995736766e-17,0.10452846326765346,0.20791169081775945,0.30901699437494745,0.4067366430758002,0.5000000000000001,0.5877852522924731,0.6691306063588582,0.7431448254773942,0.8090169943749475,0.8660254037844387,0.9135454576426009,0.9510565162951535,0.9781476007338057,0.9945218953682733,1.0,0.9945218953682733,0.9781476007338057,0.9510565162951535,0.9135454576426009,0.8660254037844387,0.8090169943749475,0.7431448254773942,0.6691306063588582,0.5877852522924731,0.5000000000000001,0.4067366430758002,0.30901699437494745,0.20791169081775945,0.10452846326765346,6.123233995736766e-17,-0.10452846326765333,-0.20791169081775912,-0.30901699437494734,-0.4067366430758001,-0.4999999999999998,-0.587785252292473,-0.6691306063588582,-0.7431448254773941,-0.8090169943749473,-0.8660254037844387,-0.9135454576426008,-0.9510565162951535,-0.9781476007338057,-0.9945218953682733,-1.0,-0.9945218953682734,-0.9781476007338057,-0.9510565162951535,-0.9135454576426011,-0.8660254037844386,-0.8090169943749475,-0.7431448254773942,-0.6691306063588585,-0.5877852522924732,-0.5000000000000004,-0.4067366430758001,-0.30901699437494756,-0.2079116908177598,-0.10452846326765336,
};

float sin_60[60] = {-1.0,-0.9945218953682733,-0.9781476007338056,-0.9510565162951535,-0.9135454576426009,-0.8660254037844386,-0.8090169943749475,-0.7431448254773941,-0.6691306063588582,-0.5877852522924731,-0.49999999999999994,-0.40673664307580015,-0.3090169943749474,-0.20791169081775931,-0.10452846326765346,0.0,0.10452846326765346,0.20791169081775931,0.3090169943749474,0.40673664307580015,0.49999999999999994,0.5877852522924731,0.6691306063588582,0.7431448254773941,0.8090169943749475,0.8660254037844386,0.9135454576426009,0.9510565162951535,0.9781476007338056,0.9945218953682733,1.0,0.9945218953682734,0.9781476007338057,0.9510565162951536,0.913545457642601,0.8660254037844387,0.8090169943749475,0.7431448254773942,0.6691306063588583,0.5877852522924732,0.49999999999999994,0.40673664307580043,0.3090169943749475,0.20791169081775931,0.10452846326765373,1.2246467991473532e-16,-0.10452846326765305,-0.20791169081775907,-0.30901699437494773,-0.4067366430757998,-0.5000000000000001,-0.587785252292473,-0.6691306063588582,-0.743144825477394,-0.8090169943749473,-0.8660254037844385,-0.913545457642601,-0.9510565162951535,-0.9781476007338056,-0.9945218953682734,
};

float cos_12[] = {
6.123233995736766e-17,0.5000000000000001,0.8660254037844387,1.0,0.8660254037844387,0.5000000000000001,6.123233995736766e-17,-0.4999999999999998,-0.8660254037844387,-1.0,-0.8660254037844386,-0.5000000000000004,
};

float sin_12[] = {
-1.0,-0.8660254037844386,-0.49999999999999994,0.0,0.49999999999999994,0.8660254037844386,1.0,0.8660254037844387,0.49999999999999994,1.2246467991473532e-16,-0.5000000000000001,-0.8660254037844385,
};

int strlen(const char* str) 
{
	int len = 0;
	while (str[len])
		len++;
	return len;
}

inline uint32_t strncmp(const char* str, const char* str2, uint32_t len)
{
	return memcmp((uint8_t*)str, (uint8_t*)str2, len) == 0;
}

inline inline uint32_t memcmp(const void* ptr, const void* ptr2, uint32_t len)
{
	char* ptr_c = (char*) ptr;
	char* ptr2_c = (char*) ptr2;
	for (uint32_t i = 0; i < len; i++)
	{
		if(ptr_c[i] != ptr2_c[i])
			return -1;
	}

	return 0;
}

#define MAX_ARGS 5
int parse_arguments(const char *input_string, char *tokens[]) {
    int num_tokens = 0;  /* Number of arguments */
    int token_start = -1;  /* Index of start of current token */
    int token_end = -1;  /* Index of end of current token */
    int i;
    /* Tokenize input string using space as delimiter */
    for (i = 0; input_string[i] != '\0'; i++) {
        char c = input_string[i];

        if (c == ' ' || c == '\n') {
            /* End of token, add to tokens array */
            if (token_start != -1) {
                token_end = i - 1;
                int token_length = token_end - token_start + 1;

                /* Copy characters from input string to token string */
                for (int j = 0; j < token_length; j++) {
                    tokens[num_tokens][j] = input_string[token_start + j];
                }
                tokens[num_tokens][token_length] = '\0';
                
                num_tokens++;
                token_start = -1;
            }
        } else {
            /* Start of new token */
            if (token_start == -1) {
                token_start = i;
            }
        }

        /* Maximum number of arguments reached, break loop */
        if (num_tokens == MAX_ARGS) {
            break;
        }
    }
    
    if (token_start != -1) {
        token_end = i - 1;
        int token_length = token_end - token_start + 1;
    
        /* Copy characters from input string to token string */
        for (int j = 0; j < token_length; j++) {
            tokens[num_tokens][j] = input_string[token_start + j];
        }
        tokens[num_tokens][token_length] = '\0';
    
        num_tokens++;
    }
    
    
    return num_tokens;
}

char* strchr(const char* str, int ch)
{
    while (*str != '\0') {
        if (*str == ch) {
            return (char*)str;
        }
        ++str;
    }
    return NULL;
}

char* strtok(char* str, const char* delim)
{
    static char* current = NULL;   /* Keeps track of the current position in the string */

    if (str != NULL) {
        current = str;
    }

    if (current == NULL || *current == '\0') {
        return NULL;   /* If there is no more string or the current position is at the end, return NULL */
    }

    char* token = current;   /* Start of the next token */

    /* Find the next occurrence of any delimiter characters */
    while (*current != '\0' && strchr(delim, *current) == NULL) {
        ++current;
    }

    if (*current != '\0') {
        *current = '\0';   /* Null-terminate the token */
        ++current;   /* Move the current position to the next character after the delimiter */
    }

    return token;   /* Return the next token */
}

/**
 * @brief Custom implementation of getopt.
 *
 * @param argc     The number of command-line arguments.
 * @param argv     An array of command-line argument strings.
 * @param optstring A string specifying the valid options and their characteristics.
 *                  Each character represents an option, and a colon (:) following
 *                  an option indicates that it requires an argument.
 * @param optarg   A pointer to store the argument of the option (if any).
 * @return int     The next recognized option character or -1 if there are no more options.
 *                 Returns '?' if an invalid option is encountered.
 *                 Returns ':' if an option requiring an argument is missing the argument.
 *
 * This function parses the command-line options and arguments, based on the provided
 * optstring, and returns the next recognized option character. It also updates the
 * optarg pointer to point to the argument associated with an option that requires one.
 * If all options have been processed, the function returns -1. If an invalid option is
 * encountered, it returns '?'. If an option requiring an argument is missing the argument,
 * it returns ':'.
 */
int getopt(int argc, char* argv[], const char* optstring, char** optarg)
{
    int optind = 1;
    int optpos = 1;

    if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
        return -1;  // No more options or not an option
    }

    char opt = argv[optind][optpos];
    const char* optptr = strchr(optstring, opt);

    if (optptr == NULL) {
        return '?';  // Invalid option
    }

    if (*(optptr + 1) == ':') {
        if (argv[optind][optpos + 1] != '\0') {
            *optarg = argv[optind] + optpos + 1;
            optind++;
            optpos = 1;
        }
        else if (optind + 1 < argc) {
            *optarg = argv[optind + 1];
            optind += 2;
            optpos = 1;
        }
        else {
            return ':';  // Missing option argument
        }
    }
    else {
        if (argv[optind][optpos + 1] != '\0') {
            optpos++;
        }
        else {
            optind++;
            optpos = 1;
        }
    }

    return opt;
}

/* TODO: Move some functions into own files. */


/*
  Highly optimized memcpy and memset functions.
  from: https://forum.osdev.org/viewtopic.php?t=18119
*/

/**
 * void *dest, const void *src, int n
 *
 * @return void*
 */
void* memcpy(void *dest, const void *src, int n)
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
 * void *dest, int val, int n
 *
 * @return void*
 */
inline void* memset(void *dest, int val, int n)
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

inline int isdigit(char c)
{
    if ((c>='0') && (c<='9')) return 1;
    return 0;
}

/*
 * Functions from Kerninghan/Ritchie - The C Programming Language
 */

inline void reverse(char s[])
{
	int c, i, j;

	for (i = 0, j = strlen(s)-1; i < j; i++, j--)
	{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

inline int atoi(char s[])
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

inline void itoa(int n, char s[])
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

inline void itohex(uint32_t n, char s[])
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

/* https://wiki.osdev.org/Random_Number_Generator */
static unsigned long int next = 1;  /* NB: "unsigned long int" is assumed to be 32 bits wide */
int rand(void)  /* RAND _MAX assumed to be 32767*/
{
    next = next * 1103515245 + 12345;
    return (unsigned int) (next / 65536) % 32768;
}

int kernel_size = 0;

unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#ifdef __cplusplus
}
#endif
