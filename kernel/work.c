#include <work.h>
#include <util.h>
#include <scheduler.h>
#include <serial.h>
#include <assert.h>
#include <memory.h>

static struct work_queue queue = {
    .head = NULL,
    .tail = NULL,
    .size = 0
};

void work_queue_add(void (*work_fn)(void*), void* arg)
{
    struct work* work = (struct work*)kalloc(sizeof(struct work));
    work->work_fn = work_fn;
    work->arg = arg;
    work->next = NULL;

    dbgprintf("Adding work 0x%x\n", work_fn);
    LOCK((&queue), {
        if (queue.head == NULL) {
            queue.head = work;
            queue.tail = work;
        } else {
            queue.head->next = work;
            queue.tail = work;
        }
    });
}

void worker_test(int a)
{
    dbgprintf("Worker test (0x%x): 0x%x\n", &worker_test, a);
}

void init_worker()
{
    mutex_init(&queue.lock);
}

void worker_thread()
{

    while (1) {

        while (queue.head == NULL) {
            /* Should block */
            kernel_yield();
        }

        struct work* work;
        LOCK((&queue), {
            work = queue.head;
            queue.head = queue.head->next;

            if (queue.head == NULL) {
                queue.tail = NULL;
            }
        });

        dbgprintf("Running work... 0x%x\n", work->work_fn);
        work->work_fn(work->arg);
        kfree(work);
    }
}