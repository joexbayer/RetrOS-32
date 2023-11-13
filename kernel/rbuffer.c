#include <rbuffer.h>
#include <memory.h>

/* Prototypes */
static error_t __ring_buffer_add(struct ring_buffer *buffer, unsigned char *data, int length);
static error_t __ring_buffer_read(struct ring_buffer *buffer, unsigned char *data, int length);

/* Default ring buffer operations */
struct ring_buffer_operations default_ring_buffer_ops = {
    .add = &__ring_buffer_add,
    .read = &__ring_buffer_read  
};

/**
 * @brief Creates a new ring buffer.
 *
 * The `rbuffer_new()` function creates a new ring buffer of the specified size using the kernel allocator `kalloc()`. 
 * The function initializes the start and end indices to 0 and sets the spinlock to 0. It returns a pointer to the 
 * newly created `struct ring_buffer`.
 *
 * @param size The size of the ring buffer to be created.
 * @return A pointer to the newly created `struct ring_buffer`.
 */
struct ring_buffer* rbuffer_new(int size)
{
    struct ring_buffer* rbuf = kalloc(sizeof(struct ring_buffer));
    rbuf->buffer = kalloc(size);
    rbuf->ops = &default_ring_buffer_ops;
    rbuf->size = size;
    rbuf->start = 0;
    rbuf->end = 0;
    rbuf->spinlock = 0;

    return rbuf;
}


/**
 * @brief Frees the memory allocated for a ring buffer.
 *
 * The `rbuffer_free()` function frees the memory allocated for the specified ring buffer and its associated buffer
 * using the kernel allocator `kfree()`. The function takes a pointer to the `struct ring_buffer` to be freed as 
 * the argument and does not return anything.
 *
 * @param rbuf A pointer to the `struct ring_buffer` to be freed.
 */
void rbuffer_free(struct ring_buffer* rbuf)
{
    kfree(rbuf->buffer);
    kfree(rbuf);
}

/**
 * @brief Adds data to a ring buffer.
 *
 * The `ring_buffer_add()` function adds `length` bytes of data from the `data` buffer to the end of the ring buffer
 * specified by the `buffer` parameter. The function uses a spinlock to protect the critical section and adds the data
 * to the end of the buffer by copying it into the buffer starting at the end index. If the buffer becomes full, the
 * oldest data is overwritten.
 *
 * @param buffer A pointer to the `struct ring_buffer` representing the ring buffer to add data to.
 * @param data A pointer to the buffer containing the data to be added to the ring buffer.
 * @param length The number of bytes of data to add to the ring buffer.
 * @return The number of bytes of data added to the buffer.
 */
static error_t __ring_buffer_add(struct ring_buffer *buffer, unsigned char *data, int length)
{
    /* Calculate the number of bytes that can be added to the buffer */
    int available = buffer->size - buffer->end + buffer->start;
    if (available < length) {
        return -ERROR_RBUFFER_FULL;
    }

    SPINLOCK(buffer, {
        /* Copy the data into the buffer */
        if (buffer->end + length <= buffer->size) {
            /* The data doesn't wrap around the buffer */
            memcpy(buffer->buffer + buffer->end, data, length);
            buffer->end += length;
            break;
        } 
        /* The data wraps around the buffer */
        int first_length = buffer->size - buffer->end;
        memcpy(buffer->buffer + buffer->end, data, first_length);
        memcpy(buffer->buffer, data + first_length, length - first_length);
        buffer->end = length - first_length;
        
    });

	return length;
}

/**
 * @brief Reads data from a ring buffer.
 *
 * The `ring_buffer_read()` function reads up to `length` bytes of data from the ring buffer specified by the `buffer`
 * parameter into the `data` buffer. The function uses a spinlock to protect the critical section and reads the data
 * from the buffer starting at the start index. If the buffer becomes empty, the function returns 0.
 *
 * @param buffer A pointer to the `struct ring_buffer` representing the ring buffer to read data from.
 * @param data A pointer to the buffer to store the read data.
 * @param length The maximum number of bytes of data to read from the ring buffer.
 * @return The actual number of bytes of data read from the buffer.
 */
static error_t __ring_buffer_read(struct ring_buffer *buffer, unsigned char *data, int length) {
    /* Check if there is data available in the buffer */
    int available = buffer->end - buffer->start;
    int read_length = 0;
    if (available == 0) {
        return -ERROR_RBUFFER_EMPTY;
    }

    SPINLOCK(buffer, {
        /* Read the data from the buffer into a temporary buffer */
        char* temp_buffer = kalloc(available);
        if (buffer->start + available <= buffer->size) {
            memcpy(temp_buffer, buffer->buffer + buffer->start, available);
        } else {
            int first_length = buffer->size - buffer->start;
            memcpy(temp_buffer, buffer->buffer + buffer->start, first_length);
            memcpy(temp_buffer + first_length, buffer->buffer, available - first_length);
        }

        /* Copy the data to the output buffer */
        read_length = available < length ? available : length;
        memcpy(data, temp_buffer, read_length);

        /* Update the start index of the buffer */
        buffer->start += read_length;
        if (buffer->start == buffer->end) {
            buffer->start = 0;
            buffer->end = 0;
        } else if (buffer->start == buffer->size) {
            buffer->start = 0;
        }

        kfree(temp_buffer);
    });

    return read_length;
}