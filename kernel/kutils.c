#include <util.h>
#include <serial.h>

/* Function to perform run-length encoding on binary data */
unsigned char* run_length_encode(const unsigned char* data, int length, unsigned char* out, int* encodedLength)
{
    unsigned char* encodedData = out;
    int index = 0;
    int count = 1;

    for (int i = 1; i < length; i++) {
        if (data[i] == data[i - 1]) {
            count++;
        } else {
            encodedData[index++] = count;
            encodedData[index++] = data[i - 1];
            count = 1;
        }
    }

    // Store the last run
    encodedData[index++] = count;
    encodedData[index++] = data[length - 1];

    *encodedLength = index;
    dbgprintf("Run length encoded data from %d to %d bytes\n", length, *encodedLength);
    return encodedData;
}

/* Function to perform run-length decoding on binary data */
unsigned char* run_length_decode(const unsigned char* encodedData, int encodedLength, unsigned char* out, int* decodedLength)
{
    unsigned char* decodedData = out;
    int index = 0;

    for (int i = 0; i < encodedLength; i += 2) {
        int count = encodedData[i];
        unsigned char bit = encodedData[i + 1];

        for (int j = 0; j < count; j++) {
            decodedData[index++] = bit;
        }
    }

    *decodedLength = index;
    return decodedData;
}