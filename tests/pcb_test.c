#include <pcb.h>
#include <stdio.h>

FILE* filesystem = NULL;
unsigned int* kernel_page_dir = 0;
int cli_cnt = 0;
int kernel_size = 50000;

int main(int argc, char const *argv[])
{
    // Test pcb_new_queue
    struct pcb_queue* new_queue = pcb_new_queue();
    testprintf(new_queue != NULL, "pcb_new_queue() - Queue Creation");

    // Test __pcb_queue_push and __pcb_queue_pop
    struct pcb test_pcb = {
        .next = NULL,
        .prev = NULL,
    }; // Mock pcb structure
    error_t push_result = new_queue->ops->push(new_queue, &test_pcb);
    testprintf(push_result == ERROR_OK, "__pcb_queue_push() - Push PCB to Queue");
    struct pcb* popped_pcb = new_queue->ops->pop(new_queue);
    testprintf(popped_pcb == &test_pcb, "__pcb_queue_pop() - Pop PCB from Queue");

    // Test __pcb_queue_add and __pcb_queue_peek
    error_t add_result = new_queue->ops->add(new_queue, &test_pcb);
    testprintf(add_result == 0, "__pcb_queue_add() - Add PCB to Queue");
    struct pcb* peeked_pcb = new_queue->ops->peek(new_queue);
    testprintf(peeked_pcb == &test_pcb, "__pcb_queue_peek() - Peek PCB in Queue");

    // Test __pcb_queue_remove
    new_queue->ops->remove(new_queue, &test_pcb);
    peeked_pcb = new_queue->ops->peek(new_queue);
    testprintf(peeked_pcb == NULL, "__pcb_queue_remove() - Remove PCB from Queue");    
    printf("%p\n", peeked_pcb);

    return 0;
}