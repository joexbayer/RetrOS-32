#ifndef STDLIB_HELPER_H
#define STDLIB_HELPER_H

#include <util.h>
#include <stdint.h>
#include "cppUtils.hpp"
#include <lib/syscall.h>

class stdlib {
public:
    // Function to find the minimum of two integers
    static int min(int a, int b) {
        return (a < b) ? a : b;
    }

    // Function to find the maximum of two integers
    static int max(int a, int b) {
        return (a > b) ? a : b;
    }

    // Function to calculate the sum of elements in an array
    static int sum(const int* arr, size_t size) {
        int result = 0;
        for (size_t i = 0; i < size; i++) {
            result += arr[i];
        }
        return result;
    }

    // Function to copy memory from source to destination
    static void* memcpy(void* dest, const void* src, size_t numBytes) {
        char* destChar = static_cast<char*>(dest);
        const char* srcChar = static_cast<const char*>(src);
        for (size_t i = 0; i < numBytes; i++) {
            destChar[i] = srcChar[i];
        }
        return dest;
    }

    // Function to move memory from source to destination (handles overlapping memory regions)
    static void* memmove(void* dest, const void* src, size_t numBytes) {
        char* destChar = static_cast<char*>(dest);
        const char* srcChar = static_cast<const char*>(src);

        // Check if the memory regions overlap and determine the copy direction
        if (destChar < srcChar) {
            for (size_t i = 0; i < numBytes; i++) {
                destChar[i] = srcChar[i];
            }
        } else if (destChar > srcChar) {
            for (size_t i = numBytes; i > 0; i--) {
                destChar[i - 1] = srcChar[i - 1];
            }
        }

        return dest;
    }

    // Function to set memory to a specific value
    static void* memset(void* dest, int value, size_t numBytes) {
        char* destChar = static_cast<char*>(dest);
        for (size_t i = 0; i < numBytes; i++) {
            destChar[i] = static_cast<char>(value);
        }
        return dest;
    }
};

#endif  // STDLIB_HELPER_H