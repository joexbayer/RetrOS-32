#ifndef C344935F_66B9_4B70_A26F_D6BCDAF73498
#define C344935F_66B9_4B70_A26F_D6BCDAF73498

#include <sync.h>

enum work_states {
    WORK_WAITING,
    WORK_STARTED,
    WORK_FINISHED
};

struct work {
    int (*work_fn)(void*);
    void (*callback)(int);
    struct work* next;
    char state;
    int in_use;
    void* arg;
};

struct work_queue {
    struct work* head;
    struct work* tail;

    int size;
};

int work_queue_add(int (*fn)(void*), void* arg, void(*callback)(int));
void worker_thread();
void init_worker();
#endif /* C344935F_66B9_4B70_A26F_D6BCDAF73498 */
