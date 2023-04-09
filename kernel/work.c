#include <work.h>
#include <util.h>
#include <scheduler.h>
#include <serial.h>
#include <assert.h>
#include <memory.h>

/* Limits amount of works in queue and eliminates the need for kalloc */
#define WORK_POOL_SIZE 25
static struct work __work_pool[WORK_POOL_SIZE];
static int works_in_queue = 0;

static struct work* get_new_work() {
    struct work* new = NULL;

    CRITICAL_SECTION({
        for (int i = 0; i < WORK_POOL_SIZE; i++){
            if(__work_pool[i].in_use == 0){
                __work_pool[i].in_use = 1;
                new = &__work_pool[i];
                break;
            }
        }
    });
    
    return new;
}

static struct work_queue queue = {
    .head = NULL,
    .tail = NULL,
    .size = 0
};

/**
 * @brief Adds new work which will call function work_fn and call the call back with the
 * return value of work_fn
 * 
 * @param work_fn function the worker will call
 * @param arg argument to work_fn
 * @param callback with return value of work_fn
 */
void work_queue_add(int (*work_fn)(void*), void* arg, void(*callback)(int))
{
    struct work* work = get_new_work();
    if(work == NULL){
        dbgprintf("Out of works\n");
        return;
    }

    work->work_fn = work_fn;
    work->arg = arg;
    work->state = WORK_WAITING;
    work->callback = callback;
    work->next = NULL;

    dbgprintf("Adding work 0x%x\n", work);

    CRITICAL_SECTION({
        if (queue.head == NULL) {
            queue.head = work;
            queue.tail = work;
        } else {
            queue.tail->next = work;
            queue.tail = work;
        }
        works_in_queue++;
    });
}

void init_worker()
{
    for (int i = 0; i < WORK_POOL_SIZE; i++){
        __work_pool[i].in_use = 0;
    }
}

static int workers = 0;

void worker_thread()
{  
    int my_id = workers++;
    dbgprintf("[%d] Starting worker thread...\n", my_id);
    while (1) {

        CLI();
        if(queue.head == NULL) {
            /* Should block */
            STI();
            kernel_yield();
            continue;
        }

        ASSERT_CRITICAL();
        struct work* work = queue.head;
        queue.head = queue.head->next;
        works_in_queue--;

        if (queue.head == NULL) {
            queue.tail = NULL;
        }
        
        STI();

        work->state = WORK_STARTED;
        dbgprintf("[%d] Running work... 0x%x (arg: %d)\n", my_id, work->work_fn, work->arg);
        int ret = work->work_fn(work->arg);
        if(work->callback != NULL){
            work->callback(ret);
        }
        work->in_use = 0;
        work->next = NULL;
    }
}