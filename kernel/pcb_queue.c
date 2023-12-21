/**
 * @file pcb.c
 * @author Joe Bayer (joexbayer)
 * @brief Manages PCBs creation, destruction and their status.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <pcb.h>
#include <serial.h>
#include <memory.h>
#include <assert.h>
#include <kutils.h>
#include <util.h>
#include <errors.h>

/* Prototype functions for pcb queue interface */
static error_t __pcb_queue_push(struct pcb_queue* queue, struct pcb* pcb);
static error_t __pcb_queue_add(struct pcb_queue* queue, struct pcb* pcb);
static void __pcb_queue_remove(struct pcb_queue* queue, struct pcb* pcb);
static struct pcb* __pcb_queue_pop(struct pcb_queue* queue);
static struct pcb* __pcb_queue_peek(struct pcb_queue* queue);

/* Setup for default pcb queue operations */
static struct pcb_queue_operations pcb_queue_default_ops = {
	.push = &__pcb_queue_push,
	.add = &__pcb_queue_add,
	.remove = &__pcb_queue_remove,
	.pop = &__pcb_queue_pop,
	.peek = &__pcb_queue_peek
};

/**
 * @brief Creates a new PCB queue.
 *
 * The `pcb_new_queue()` function allocates memory for a new `pcb_queue` structure and initializes its members.
 * The `_list` member is set to `NULL`, and the queue's operations and spinlock are attached and initialized.
 *
 * @return A pointer to the newly created `pcb_queue` structure. NULL on error.
 */ 
struct pcb_queue* pcb_new_queue()
{
	struct pcb_queue* queue = create(struct pcb_queue); 
	if(queue == NULL){
		return NULL;
	}

	queue->_list = NULL;
	queue->ops = &pcb_queue_default_ops;
	queue->spinlock = 0;
	queue->total = 0;

	return queue;
}

/**
 * @brief Pushes a PCB onto the single linked PCB queue.
 *
 * The `__pcb_queue_push()` function adds a PCB to the end of the specified queue. The function takes a pointer to
 * the `pcb_queue` structure and a pointer to the `pcb` structure to be added as arguments. The function uses
 * a spinlock to protect the critical section and adds the PCB to the end of the queue by traversing the current
 * list of PCBs and adding the new PCB to the end.
 *
 * @param queue A pointer to the `pcb_queue` structure to add the `pcb` to.
 * @param pcb A pointer to the `pcb` structure to add to the queue.
 */
static error_t __pcb_queue_push(struct pcb_queue* queue, struct pcb* pcb)
{
	ERR_ON_NULL(pcb);
	if(queue == NULL){
		return -ERROR_PCB_QUEUE_NULL;
	}

	/* This approach is suboptimal for large pcb queues, as you have to iterate over all pcbs */
	SPINLOCK(queue, {

		struct pcb* current = queue->_list;
		if(current == NULL){
			queue->_list = pcb;
			break;
		}
		while (current->next != NULL){
			current = current->next;
		}
		current->next = pcb;
		pcb->next = NULL;
		queue->total++;


	});

	return ERROR_OK;
}

/**
 * @brief Adds a PCB to the single linked PCB queue.
 *
 * The `__pcb_queue_add()` function adds a PCB to the beginning of the specified queue. The function takes a pointer to
 * the `pcb_queue` structure and a pointer to the `pcb` structure to be added as arguments. The function uses
 * a spinlock to protect the critical section and adds the PCB to the beginning of the queue by modifying the pointers
 * of the existing PCBs in the queue to insert the new PCB at the front.
 *
 * @param queue A pointer to the `pcb_queue` structure to add the `pcb` to.
 * @param pcb A pointer to the `pcb` structure to add to the queue.
 */
static error_t __pcb_queue_add(struct pcb_queue* queue, struct pcb* pcb)
{
	ERR_ON_NULL(pcb);
	if(queue == NULL){
		return -ERROR_PCB_QUEUE_NULL;
	}

	SPINLOCK(queue, {

		/* Add the pcb to the front of the queue */
		pcb->next = queue->_list; /* Set the next pointer of the new pcb to the current head of the queue */
		queue->_list = pcb; /* Set the head of the queue to the new pcb */

		queue->total++;

	});

	return ERROR_OK;
}

/**
 * @brief Removes a PCB from the double linked PCB queue.
 *
 * The `__pcb_queue_remove()` function removes a PCB from the specified queue. The function takes a pointer to the
 * `pcb_queue` structure and a pointer to the `pcb` structure to be removed as arguments. The function uses a
 * spinlock to protect the critical section and removes the specified PCB from the queue by modifying the pointers
 * of the previous and next PCBs in the queue to bypass the removed PCB.
 *
 * @param queue A pointer to the `pcb_queue` structure to remove the `pcb` from.
 * @param pcb A pointer to the `pcb` structure to remove from the queue.
 */
static void __pcb_queue_remove(struct pcb_queue* queue, struct pcb* pcb)
{
	if(queue == NULL || pcb == NULL || queue->_list == NULL){
		return;
	}

	SPINLOCK(queue, {

		if(queue->_list == pcb){
			queue->_list = pcb->next;
			break;
		}


		/* Remove pcb from linked list queue */
		struct pcb* current = queue->_list;
		while ( current->next != NULL && current->next != pcb){
			current = current->next;
		}

		if(current->next == NULL){
			break;
		}

		current->next = pcb->next;
	});

}

/**
 * @brief Removes and returns the first PCB in the PCB queue.
 *
 * The `__pcb_queue_pop()` function removes and returns the first PCB in the specified queue. The function takes a pointer
 * to the `pcb_queue` structure as an argument. The function uses a spinlock to protect the critical section and removes
 * the first PCB from the queue by modifying the pointers of the previous and next PCBs in the queue to bypass the
 * removed PCB. The function returns a pointer to the removed PCB, or `NULL` if the queue is empty.
 *
 * @param queue A pointer to the `pcb_queue` structure to remove the first PCB from.
 * @return A pointer to the first PCB in the queue, or `NULL` if the queue is empty.
 */
static struct pcb* __pcb_queue_pop(struct pcb_queue* queue)
{
    if(queue == NULL || queue->_list == NULL){
		return NULL;
	}

	struct pcb* front = NULL;
	SPINLOCK(queue, {

		front = queue->_list;
		queue->_list = front->next;

		front->next = NULL;
		front->prev = NULL;
	});

    return front;
}

/**
 * @brief Returns but not removes the first PCB in the PCB queue.
 *
 * The `__pcb_queue_peek()` function returns the first PCB in the specified queue. The function takes a pointer
 * to the `pcb_queue` structure as an argument. The function uses a spinlock to protect the critical section and
 * returns a pointer to the first PCB in the queue, or `NULL` if the queue is empty.
 *
 * @param queue A pointer to the `pcb_queue` structure to return the first PCB from.
 * @return A pointer to the first PCB in the queue, or `NULL` if the queue is empty.
 */
static struct pcb* __pcb_queue_peek(struct pcb_queue* queue)
{
	if(queue == NULL || queue->_list == NULL){
		return NULL;
	}

	struct pcb* front = NULL;
	SPINLOCK(queue, {
		front = queue->_list;
	});

	return front;
}