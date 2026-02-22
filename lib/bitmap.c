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
    if (b == NULL || i < 0) {
        return;
    }

    b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i)
{
    if (b == NULL || i < 0) {
        return;
    }

    b[i / 8] &= ~(1 << (i & 7));
}

int get_bitmap(bitmap_t b, int i)
{
    if (b == NULL || i < 0) {
        return -1;
    }

    return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
}

int get_bitmap_size(int n)
{
    if (n <= 0) {
        return 0;
    }

    return (n + 7) / 8;
}

bitmap_t create_bitmap(int n)
{
    int bitmap_size = get_bitmap_size(n);
    if (bitmap_size <= 0) {
        return NULL;
    }

    bitmap_t map = (bitmap_t) kalloc(bitmap_size);
    if (map == NULL) {
        return NULL;
    }

    memset(map, 0, bitmap_size);
    return map;
}

void destroy_bitmap(bitmap_t b)
{
    kfree((void*) b);
}

inline int __continous_helper(bitmap_t b, int start, int size, int n)
{
    if (b == NULL || start < 0 || size <= 0 || n <= 0) {
        return -1;
    }
    if (start + size > n) {
        return -1;
    }

    for (int j = 0; j < size; j++)
    {
        if(get_bitmap(b, start+j) != 0){
            return -1;
        }
    }
    return 0;
}

int bitmap_unset_continous(bitmap_t b, int start, int size)
{
    if (b == NULL || start < 0 || size <= 0) {
        return -1;
    }

    for (int i = start; i < (start+size); i++)
    {
        unset_bitmap(b, i);
    }
    return 0;
}

int bitmap_get_continous(bitmap_t b, int n, int size)
{
    if (b == NULL || n <= 0 || size <= 0 || size > n) {
        return -1;
    }

    for (int i = 0; i <= (n - size); i++)
    {
        if(get_bitmap(b, i) == 0)
        {
            
            int ret = __continous_helper(b, i, size, n);

            if(ret < 0)
                continue;

            for (int j = 0; j < size; j++)
            {
                set_bitmap(b, i+j);
            }
            return i;
        }
    }

    return -1;
}

int get_free_bitmap(bitmap_t b, int n)
{
    if (b == NULL || n <= 0) {
        return -1;
    }

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
