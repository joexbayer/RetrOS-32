#include <stdio.h>
#include <stdlib.h>

unsigned char* run_length_encode(const unsigned char* data, int length, unsigned char* out, int* encodedLength)
{
    unsigned char* encodedData = out;
    int index = 0;
    unsigned short count = 1;

    for (int i = 1; i < length; i++) {
        if (data[i] == data[i - 1]) {
            count++;
        } else {
            encodedData[index++] = data[i - 1];
            encodedData[index++] = (count & 0xFF);           // Lower byte of count
            encodedData[index++] = ((count >> 8) & 0xFF);    // Upper byte of count
            count = 1;
        }
    }

    // Store the last run
    encodedData[index++] = data[length - 1];
    encodedData[index++] = (count & 0xFF);
    encodedData[index++] = ((count >> 8) & 0xFF);

    *encodedLength = index;
    return encodedData;
}

unsigned char* run_length_decode(const unsigned char* encodedData, int encodedLength, unsigned char* out, int* decodedLength)
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



unsigned char* store;
unsigned char* background;

int main(int argc, char** argv) {

    /* usage: encode <filename> <output> */
    if (argc < 3) {
        printf("Usage: encode <filename> <output>\n");
        return 1;
    }

    background = malloc(320*240);

    FILE *inputFile = fopen(argv[1], "r"); // Specify the file name and open the file for reading
    if (inputFile != NULL) { // Check if the file is opened successfully
        // Read data from the file
        fread(background, sizeof(unsigned char), 320*240, inputFile);

        // Close the file
        fclose(inputFile);

        printf("Data has been read from the file successfully.\n");
    } else {
        printf("Failed to open the file for reading.\n");
    }

    int len = 0;
    int len2 = 0;

    store = malloc(1000*70);

    unsigned char* encoded = run_length_encode(background, 320*240, store, &len);
    printf("Run length encoded data from %d to %d bytes\n", 320*240, len);
    run_length_decode(encoded, len, background, &len2);
    printf("Run length decoded data from %d to %d bytes\n", len, len2 );


    FILE *outputFile = fopen(argv[2], "w"); // Specify the file name and open the file for writing
    if (outputFile != NULL) { // Check if the file is opened successfully
        // Write data to the file
        fwrite(encoded, sizeof(unsigned char), len, outputFile);

        // Close the file
        fclose(outputFile);

        printf("Data has been written to the file successfully.\n");
    } else {
        printf("Failed to open the file for writing.\n");
    }

    return 0;
}
