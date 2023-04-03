#ifndef ADE2F814_93C0_48D5_8ADD_9DBB9B975A18
#define ADE2F814_93C0_48D5_8ADD_9DBB9B975A18

#include <sync.h>
struct ring_buffer;
struct ring_buffer_operations {
    /* Reads data from a ring buffer. */
    int (*read)(struct ring_buffer* rbuf, unsigned char* data, int len);
    /* Adds data to a ring buffer. */
    int (*add)(struct ring_buffer* rbuf, unsigned char* data, int len);
};

struct ring_buffer {
    struct ring_buffer_operations* ops;
    spinlock_t spinlock;
	char *buffer;    /* Pointer to the buffer data */
	int size;        /* Size of the buffer */
	int start;       /* Index of the first element in the buffer */
	int end;         /* Index of the next available element in the buffer */
};

struct ring_buffer* rbuffer_new(int size);
void rbuffer_free(struct ring_buffer* rbuf);

#endif /* ADE2F814_93C0_48D5_8ADD_9DBB9B975A18 */
