/**
 * @file fat16.c
 * @author Joe Bayer (joexbayer)
 * @brief Main API for the FAT16 Filesystem.
 * @version 0.1
 * @date 2023-05-31
 * @see http://www.tavi.co.uk/phobos/fat.html
 * @see https://wiki.osdev.org/FAT
 * @copyright Copyright (c) 2023
 * 
 */
#include <fs/fat16.h>

void fat16_set_time(uint16_t *time, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    /* Clear the existing time bits */
    *time &= 0xFC00;

    /* Set the hours bits */
    *time |= (hours & 0x1F) << 11;

    /* Set the minutes bits */
    *time |= (minutes & 0x3F) << 5;

    /* Set the seconds bits (converted to two-second periods) */
    uint8_t twoSecondPeriods = seconds / 2;
    *time |= twoSecondPeriods & 0x1F;
}

void fat16_set_date(uint16_t *date, uint16_t year, uint8_t month, uint8_t day)
{
    /* Clear the existing date bits */
    *date &= 0xFE00;

    /* Set the year bits (offset from 1980) */
    *date |= ((year - 1980) & 0x7F) << 9;

    /* Set the month bits */
    *date |= (month & 0x0F) << 5;

    /* Set the day bits */
    *date |= (day & 0x1F);
}

/* Blocks before ROOT directory: (size of FAT)*(number of FATs) + 1 */