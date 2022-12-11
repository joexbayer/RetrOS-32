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

	int h = hash(key);
	struct hash_node* new_node = alloc(sizeof(struct hash_node));
	new_node->key = key;
	new_node->value = value;
	new_node->next = map->buckets[h];
	map->buckets[h] = new_node;
}

int hashmap_get(hashmap_t* map, char* key)
{
	int h = hash(key);

	struct hash_node* current = map->buckets[h];
	while (current != NULL) {
		if (strcmp(current->key, key) == 0) {
			return current->value;
		}
		current = current->next;
	}

	return -1;
}

int hashmap_add(hashmap_t* map, char* key, int value)
{
	int h = hash(key);

	struct hash_node* current = map->buckets[h];
	while (current != NULL) {
		if (strcmp(current->key, key) == 0) {
			current->value += value;
			return current->value;
		}
		current = current->next;
	}

	return -1;
}

void hashmap_free(hashmap_t* map)
{
	for (int i = 0; i < HASH_SIZE; i++) {

		struct hash_node* current = map->buckets[i];
		while (current != NULL) {
			struct hash_node* next = current->next;
			free(current);
			current = next;
		}
	}
}