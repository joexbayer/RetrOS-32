/**
 * @file hashmap.c
 * @author Joe Bayer (joexbayer)
 * @brief Simple key / value hashmap.
 * @version 0.1
 * @date 2022-12-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <hashmap.h>
#include <memory.h>
#include <libc.h>

inline int simple_hash(char* key)
{
	int hash = 0;
	for (int i = 0; i < strlen(key); i++) {
		hash = (hash + key[i]) % HASH_SIZE;
	}
	
	return hash;
}


void hashmap_put(hashmap_t* map, char* key, int value)
{

	int h = simple_hash(key);
	struct hash_node* new_node = palloc(sizeof(struct hash_node));
	new_node->key = key;
	new_node->value = value;
	new_node->next = map->buckets[h];
	map->buckets[h] = new_node;
}

int hashmap_get(hashmap_t* map, char* key)
{
	int h = simple_hash(key);

	struct hash_node* current = map->buckets[h];
	while (current != NULL) {
		if (memcmp(current->key, key, strlen(key)) == 0) {
			return current->value;
		}
		current = current->next;
	}

	return -1;
}

int hashmap_add(hashmap_t* map, char* key, int value)
{
	int h = simple_hash(key);

	struct hash_node* current = map->buckets[h];
	while (current != NULL) {
		if (memcmp(current->key, key, strlen(key)) == 0) {
			current->value += value;
			return current->value;
		}
		current = current->next;
	}

	return -1;
}

void hashmap_free(hashmap_t* map)
{
	/* hashmap currently uses non freeable memory. */
}