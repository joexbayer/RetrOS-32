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
#include <assert.h>

static int __skb_queue_add(struct skb_queue* skb_queue, struct sk_buff* skb);
static struct sk_buff* __skb_queue_remove(struct skb_queue* skb_queue);
struct skb_queue_operations skb_queue_ops = {
	.add = &__skb_queue_add,
	.remove = &__skb_queue_remove
};


void skb_free_queue(struct skb_queue* queue)
{
	assert(queue->size == 0);
	free(queue);
}

struct skb_queue* skb_new_queue()
{
	struct skb_queue* queue = kalloc(sizeof(struct skb_queue));
	assert(queue != NULL);

	queue->ops = &skb_queue_ops;
	queue->_head = NULL;
	queue->_tail = NULL;
	queue->size = 0;
	mutex_init(&queue->lock);
	return queue;
}

/**
 * @brief Adds a new network packet to the end of the packet queue.
 * The function uses a mutex lock to protect the critical section and modifies the queue's _head and _tail pointers to
 * insert the new packet at the end of the linked list. The function increments the size field of the queue
 * and returns 1 upon successful addition of the new packet.
 * @param skb_queue A pointer to the skb_queue structure to add the new packet to.
 * @param skb A pointer to the new network packet to be added to the queue.
 * @return 1 if the packet was successfully added to the queue, or an error code if the operation failed.
 */
static int __skb_queue_add(struct skb_queue* skb_queue, struct sk_buff* skb)
{
	LOCK(skb_queue, {
		if(skb_queue->_tail == NULL){
			skb_queue->_head = skb;
			skb_queue->_tail = skb;
			break;
		}	
		
		skb_queue->_tail->next = skb;
		skb_queue->_tail = skb;
		skb->next = NULL;
	});

	skb_queue->size++;
	return 1;
}

/**
 * @brief Removes and returns the first network packet from the packet queue.
 * The function uses a mutex lock to protect the critical section and modifies the queue's _head pointer
 * to remove the first packet from the linked list. The function returns a pointer to the removed packet, or NULL if
 * the queue is empty. The function also decrements the size field of the queue upon successful removal of a packet.
 * @param skb_queue A pointer to the skb_queue structure to remove the first packet from.
 * @return A pointer to the first network packet in the queue, or NULL if the queue is empty.
 */
static struct sk_buff* __skb_queue_remove(struct skb_queue* skb_queue)
{
	if(skb_queue->_head == NULL) return NULL;
	struct sk_buff* next = NULL;

	LOCK(skb_queue, {
		next = skb_queue->_head;
		skb_queue->_head = next->next;
		next->next = NULL;

		skb_queue->size--;
	});
	
	return next;
}

void skb_free(struct sk_buff* skb)
{
	FREE_SKB(skb);
	free(skb);
}

struct sk_buff* skb_new()
{
	struct sk_buff* new = (struct sk_buff*) kalloc(sizeof(struct sk_buff));
	if(new == NULL) return NULL;

	memset(new, 0, sizeof(struct sk_buff));
	new->netdevice = &current_netdev;
	ALLOCATE_SKB(new);

	return new;
}