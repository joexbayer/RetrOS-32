#include <stdio.h>

/* Base64 Encoding/Decoding Table */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Function to find index in Base64 Table */
static int b64_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

/* Base64 Encode Function */
void base64_encode(const unsigned char *input, int input_len, char *output) {
    int i, j;
    for (i = 0, j = 0; i < input_len; i += 3, j += 4) {
        int val = (input[i] << 16) + (i + 1 < input_len ? input[i + 1] << 8 : 0) + (i + 2 < input_len ? input[i + 2] : 0);
        output[j] = b64_table[(val >> 18) & 0x3F];
        output[j + 1] = b64_table[(val >> 12) & 0x3F];
        output[j + 2] = (i + 1 < input_len) ? b64_table[(val >> 6) & 0x3F] : '=';
        output[j + 3] = (i + 2 < input_len) ? b64_table[val & 0x3F] : '=';
    }
    output[j] = '\0'; /* Null-terminate the output string */
}

/* Base64 Decode Function */
void base64_decode(const char *input, int input_len, unsigned char *output) {
    int i, j;
    for (i = 0, j = 0; i < input_len; i += 4, j += 3) {
        int val = (b64_index(input[i]) << 18) + (b64_index(input[i + 1]) << 12) +
                  ((input[i + 2] == '=') ? 0 : (b64_index(input[i + 2]) << 6)) +
                  ((input[i + 3] == '=') ? 0 : b64_index(input[i + 3]));
        output[j] = (val >> 16) & 0xFF;
        if (input[i + 2] != '=') output[j + 1] = (val >> 8) & 0xFF;
        if (input[i + 3] != '=') output[j + 2] = val & 0xFF;
    }
}

/* Example Usage */
int main() {
    const unsigned char data_to_encode[] = "Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! ";
    char encoded_data[100]; /* Make sure this is large enough */
    base64_encode(data_to_encode, sizeof(data_to_encode) - 1, encoded_data);
    printf("Encoded: %s\n", encoded_data);

    unsigned char decoded_data[100]; /* Make sure this is large enough */
    base64_decode(encoded_data, sizeof(encoded_data) - 1, decoded_data);
    printf("Decoded: %s\n", decoded_data);

    return 0;
}
