#ifndef FCB4F955_2425_46A5_9A2F_B11453605E94
#define FCB4F955_2425_46A5_9A2F_B11453605E94

//#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

#define POOLSIZE 256*2024
#define LEX_MAX_SYMBOLS 1000

#endif /* FCB4F955_2425_46A5_9A2F_B11453605E94 */
