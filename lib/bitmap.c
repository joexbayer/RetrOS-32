/**
 * @file bitmap.c
 * @author Joe Bayer (joexbayer)
 * @brief Simple bitmap api from stackoverflow.
 * @see https://stackoverflow.com/questions/16947492/looking-for-a-bitmap-implementation-api-in-linux-c
 * @version 0.1
 * @date 2022-06-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <bitmap.h>
#include <memory.h>

void set_bitmap(bitmap_t b, int i)
{
    b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i)
{
    b[i / 8] &= ~(1 << (i & 7));
}

int get_bitmap(bitmap_t b, int i)
{
    return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
}

int get_bitmap_size(int n)
{
    return (n + 7) / 8;
}

bitmap_t create_bitmap(int n)
{
    return (bitmap_t) alloc((n + 7) / 8);
}

void destroy_bitmap(bitmap_t b)
{
    free((void*) b);
}

int get_free_bitmap(bitmap_t b, int n)
{
    for (int i = 0; i < n; i++)
    {
        if(get_bitmap(b, i) == 0)
        {
            set_bitmap(b, i);
            return i;
        }
    }
    
    return -1;
}