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

static struct sk_buff sk_buffers[MAX_SKBUFFERS]; /* depricated */

static struct skb_queue {
	mutex_t mutex;
	struct sk_buff* head;
	struct sk_buff* tail;
	int size; /***/
} skb_rx_queue, skb_tx_queue;

static mutex_t skb_mutex;

static int __skb_queue_push(struct skb_queue* skb_queue, struct sk_buff* skb)
{
	acquire(&skb_queue->mutex);

	if(skb_queue->tail == NULL){
		skb_queue->head = skb;
		skb_queue->tail = skb;
		goto skb_queue_push_done;
	}	
	
	skb_queue->tail->next = skb;
	skb_queue->tail = skb;
	skb->next = NULL;

skb_queue_push_done:
	skb_queue->size++;
	release(&skb_queue->mutex);

	return 1;
}

static struct sk_buff* __skb_queue_pop(struct skb_queue* skb_queue)
{
	if(skb_queue->head == NULL) return NULL;

	acquire(&skb_queue->mutex);
	
	struct sk_buff* next = skb_queue->head;
	skb_queue->head = next->next;
	next->next = NULL;

	skb_queue->size--;;

	release(&skb_queue->mutex);

	return next;
}

int skb_transmit(struct sk_buff* skb)
{
	return __skb_queue_push(&skb_tx_queue, skb);
}

struct sk_buff* skb_next()
{
	return __skb_queue_pop(&skb_rx_queue);
}

struct sk_buff* skb_new()
{
	struct sk_buff* new = (struct sk_buff*) alloc(sizeof(struct sk_buff));
	if(new == NULL) return NULL;

	memset(new, 0, sizeof(struct sk_buff));
	new->stage = UNUSED;
	new->netdevice = &current_netdev;

	return new;
}

int skb_free(struct sk_buff* skb)
{
	free(skb->data);
	free(skb);

	return 1;
}

void init_sk_buffers()
{
	mutex_init(&skb_mutex);
	mutex_init(&skb_rx_queue.mutex);
	mutex_init(&skb_tx_queue.mutex);
	skb_rx_queue.size = 0;
	skb_tx_queue.size = 0;

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

	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].stage == NEW_SKB){
			release(&skb_mutex);
			//STI();
			return &sk_buffers[i];
		}
	
	release(&skb_mutex);

	return NULL;
}

struct sk_buff* get_skb()
{
	acquire(&skb_mutex);
	
	int16_t i;
	for (i = 0; i < MAX_SKBUFFERS; i++)
		if(sk_buffers[i].stage == UNUSED){
			release(&skb_mutex);
			return &sk_buffers[i];
		}

	release(&skb_mutex);
	return NULL;
}

