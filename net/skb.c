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
#include <sync.h>

static struct sk_buff sk_buffers[MAX_SKBUFFERS];
static mutex_t skb_mutex;

void init_sk_buffers()
{
	mutex_init(&skb_mutex);
	for (uint16_t i = 0; i < MAX_SKBUFFERS; i++)
	{
		sk_buffers[i].stage = UNUSED;
	}
}

struct sk_buff* next_skb()
{
	//acquire(&skb_mutex);
	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].stage == NEW_SKB){
			//release(&skb_mutex);
			return &sk_buffers[i];
		}
	
	//release(&skb_mutex);
	return NULL;
}

struct sk_buff* get_skb()
{
	//acquire(&skb_mutex);
	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].stage == UNUSED){
			//release(&skb_mutex);
			return &sk_buffers[i];
		}

	//release(&skb_mutex);
	return NULL;
}

