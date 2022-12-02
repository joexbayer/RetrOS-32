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
#include <net/netdev.h>
#include <serial.h>
#include <sync.h>

static struct sk_buff sk_buffers[MAX_SKBUFFERS];
static mutex_t skb_mutex;

void init_sk_buffers()
{
	mutex_init(&skb_mutex);
	for (uint16_t i = 0; i < MAX_SKBUFFERS; i++)
	{
		sk_buffers[i].stage = UNUSED;
		sk_buffers[i].netdevice = &current_netdev;
	}
	dbgprintf("[sk_buff] Total size of buffers %d\n", sizeof(struct sk_buff)*MAX_SKBUFFERS);
}

struct sk_buff* next_skb()
{
	acquire(&skb_mutex);
	//CLI();

	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].stage == NEW_SKB){
			release(&skb_mutex);
			//STI();
			return &sk_buffers[i];
		}
	
	release(&skb_mutex);
	//STI();
	return NULL;
}

struct sk_buff* get_skb()
{
	acquire(&skb_mutex);
	//CLI();
	
	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].stage == UNUSED){
			release(&skb_mutex);
			//STI();
			return &sk_buffers[i];
		}

	release(&skb_mutex);
	//STI();
	return NULL;
}

