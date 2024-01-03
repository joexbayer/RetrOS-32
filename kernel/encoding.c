#include <kutils.h>

unsigned char* encode_run_length(const unsigned char* data, int length, unsigned char* out, int* encodedLength)
{
    unsigned char* encodedData = out;
    int index = 0;
    unsigned short count = 1;

    for (int i = 1; i < length; i++) {
        if (data[i] == data[i - 1]) {
            count++;
        } else {
            encodedData[index++] = data[i - 1];
            encodedData[index++] = (count & 0xFF);           /* Lower byte of count */
            encodedData[index++] = ((count >> 8) & 0xFF);    /* Upper byte of count */
            count = 1;
        }
    }

    /* Store the last run */
    encodedData[index++] = data[length - 1];
    encodedData[index++] = (count & 0xFF);
    encodedData[index++] = ((count >> 8) & 0xFF);

    *encodedLength = index;
    return encodedData;
}

unsigned char* decode_run_length(const unsigned char* encodedData, int encodedLength, unsigned char* out, int* decodedLength)
{
    unsigned char* decodedData = out;
    int index = 0;

    for (int i = 0; i < encodedLength; i += 3) {
        unsigned char bit = encodedData[i];
        unsigned short count = encodedData[i + 1] | (encodedData[i + 2] << 8);

        for (int j = 0; j < count; j++) {
            decodedData[index++] = bit;
        }
    }

    *decodedLength = index;
    return decodedData;
}