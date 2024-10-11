#ifndef LZ_H
#define LZ_H

#include <libc.h>
#include <stdint.h>

uint32_t lz_compress(uint8_t *input, uint32_t input_size, uint8_t **output, int find_best);
uint32_t lz_decompress(uint8_t *input, uint32_t input_size, uint8_t **output);

#endif // LZ_H