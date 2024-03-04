/* HashMap.hpp */
#ifndef __HASHMAP_H
#define __HASHMAP_H

#include <stdint.h>

const size_t HASH_MAP_SIZE = 256; /* Fixed size of the hashmap */

/* Entry structure for hashmap */
struct Entry {
    char* key;
    char* value;
    bool is_occupied; /* Indicates if the entry is occupied */

    Entry() : key(nullptr), value(nullptr), is_occupied(false) {}
};

/* HashMap class */
class HashMap {
public:
    HashMap()  {
        for (size_t i = 0; i < HASH_MAP_SIZE; ++i) {
            entries[i].is_occupied = false;
        }
    }

    void put(const char* key, const char* value){
        size_t index = hash(key);
        entries[index].key = const_cast<char*>(key);
        entries[index].value = const_cast<char*>(value);
        entries[index].is_occupied = true;
    }

    const char* get(const char* key) {
        size_t index = hash(key);
        if (entries[index].is_occupied && entries[index].key == key) {
            return entries[index].value;
        }
        return nullptr; /* Key not found */
    }

    void remove(const char* key) {
        size_t index = hash(key);
        if (entries[index].is_occupied && entries[index].key == key) {
            entries[index].is_occupied = false;
        }
    }

private:
    /* Simple hash function */
    size_t hash(const char* key) {
        size_t hash_value = 0;
        while (*key) {
            hash_value = (hash_value * 131 + *key++) % HASH_MAP_SIZE;
        }
        return hash_value;
    }

    /* Array of entries */
    Entry entries[HASH_MAP_SIZE];
};

#endif  /* __HASHMAP_H */
