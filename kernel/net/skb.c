/**
 * @file skb.c
 * @author Joe Bayer (joexbayer)
 * @brief Manages socket buffers.
 * @version 0.1
 * @date 2022-06-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <net/skb.h>

struct sk_buff sk_buffers[MAX_SKBUFFERS];

void init_sk_buffers()
{
	for (uint8_t i = 0; i < MAX_SKBUFFERS; i++)
	{
		sk_buffers[i].stage = UNUSED;
	}
}

struct sk_buff* next_skb()
{
	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].stage == NEW_SKB)
			return &sk_buffers[i];
	
	return NULL;
}

struct sk_buff* get_skb()
{
	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].stage == UNUSED)
			return &sk_buffers[i];

	return NULL;
}

