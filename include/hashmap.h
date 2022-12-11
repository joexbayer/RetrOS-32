#ifndef HASHMAP_H
#define HASHMAP_H

#define HASH_SIZE 256

struct hash_node {
	char* key;
	int value;
	struct hash_node* next;
};

typedef struct hashmap {
	struct hash_node* buckets[HASH_SIZE];
} hashmap_t;

void hashmap_put(hashmap_t* map, char* key, int value);
int hashmap_get(hashmap_t* map, char* key);
void hashmap_free(hashmap_t* map);
int hashmap_add(hashmap_t* map, char* key, int value);

#endif /* HASHMAP_H */
