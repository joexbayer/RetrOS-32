/* SimpleJson.hpp */
#ifndef SIMPLE_JSON_H
#define SIMPLE_JSON_H

#include <stdint.h> 

const size_t MAX_JSON_ENTRIES = 256;
const size_t MAX_STRING_LENGTH = 256;

/* SimpleJson class */
class SimpleJson {
public:
    SimpleJson()  {
        for (size_t i = 0; i < MAX_JSON_ENTRIES; ++i) {
            entries[i].is_occupied = false;
        }
    }

    const char* operator[](const char* key) const {
        int index = findIndex(key);
        if (index >= 0) {
            return entries[index].value;
        }
        return nullptr; 
    }

    void set(const char* key, const char* value) {
        int index = findIndex(key);
        if (index < 0) {
            /* Find an empty slot to store the new entry */
            for (int i = 0; i < MAX_JSON_ENTRIES; ++i) {
                if (!entries[i].is_occupied) {
                    entries[i].setKey(key);
                    entries[i].setValue(value);
                    entries[i].is_occupied = true;
                    return;
                }
            }
        } else {
            /* Update existing entry */
            entries[index].setValue(value);
        }
    }

private:
    struct JsonEntry {
        char key[MAX_STRING_LENGTH];
        char value[MAX_STRING_LENGTH];
        bool is_occupied;

        JsonEntry() : is_occupied(false) {
            key[0] = '\0';
            value[0] = '\0';
        }

        void setKey(const char* k) {
            strncpy(key, k, MAX_STRING_LENGTH);
            key[MAX_STRING_LENGTH - 1] = '\0'; /* Ensure null-termination */
        }

        void setValue(const char* v) {
            strncpy(value, v, MAX_STRING_LENGTH);
            value[MAX_STRING_LENGTH - 1] = '\0'; /* Ensure null-termination */
        }
    };

    JsonEntry entries[MAX_JSON_ENTRIES];

    int findIndex(const char* key) const {
        for (int i = 0; i < MAX_JSON_ENTRIES; ++i) {
            if (entries[i].is_occupied && strcmp(entries[i].key, key) == 0) {
                return i;
            }
        }
        return -1;
    }
    static void strncpy(char* dest, const char* src, size_t n) {
        for (size_t i = 0; i < n && src[i] != '\0'; ++i) {
            dest[i] = src[i];
        }
        if (n > 0) {
            dest[n - 1] = '\0';
        }
    }

    static void strcmp(const char* a, const char* b) {
        while (*a && *b && *a == *b) {
            ++a;
            ++b;
        }
        return *a - *b;
    }

};

#endif /* SIMPLE_JSON_H */
