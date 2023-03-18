#ifndef C344935F_66B9_4B70_A26F_D6BCDAF73498
#define C344935F_66B9_4B70_A26F_D6BCDAF73498

#include <sync.h>

struct work {
    void (*work_fn)(void*);
    void* arg;
    struct work* next;
};

struct work_queue {
    mutex_t lock;
    struct work* head;
    struct work* tail;

    int size;
};

void work_queue_add(void (*work_fn)(void*), void* arg);
void worker_thread();
void init_worker();

void worker_test(int a);

#endif /* C344935F_66B9_4B70_A26F_D6BCDAF73498 */
