#include <stdint.h>
#include <fs/fat16.h>
#include <kutils.h>
#include <diskdev.h>
#include <serial.h>

/**
 * Reads data from a specific cluster at a specific offset.
 * 
 * @param cluster The cluster from which data is to be read.
 * @param data The buffer to hold the data read.
 * @param data_length Maximum length of data to read from the cluster.
 * @param offset Offset within the cluster to start reading.
 * @return 0 on success, or a negative value on error.
 */
int fat16_read_data_from_cluster(uint32_t cluster, void *data, int data_length, int offset)
{
    if (offset + data_length > 512) {  /* Assuming 512-byte clusters */
        dbgprintf("Offset and buffer size exceed the cluster size\n");
        return -1;  /* Error: Offset and buffer size exceed the cluster size */
    }

    /* Calculate the block number based on the cluster. */
    uint16_t block_num = get_data_start_block() + cluster;
    return read_block_offset((byte_t *)data, data_length, offset, block_num);
}

int fat16_read_data(int first_cluster, uint32_t start_offset, void* _buffer, int buffer_length, int max_length)
{
    byte_t* buffer = (byte_t*) _buffer;
    if (start_offset > max_length) {
        dbgprintf("Start offset beyond the file size\n");
        return -1;  /* Offset exceeds file size */
    }

    int bytes_left_to_read = buffer_length;
    if ((start_offset + buffer_length) > max_length) {
        bytes_left_to_read = max_length - start_offset;  /* Adjust the buffer length if it exceeds the remaining file size. */
    }
    int total_bytes_to_read = bytes_left_to_read;

    uint32_t current_cluster = first_cluster;
    int offset_within_cluster = start_offset % 512;  /* Calculate offset within the starting cluster */
    int clusters_skipped = start_offset / 512;  /* Calculate the number of clusters to skip */

    /* Skip clusters to reach the start_offset */
    while (clusters_skipped > 0 && current_cluster != 0xFFFF) {
        dbgprintf("Skipping cluster 0x%x\n", current_cluster);
        current_cluster = fat16_get_fat_entry(current_cluster);
        clusters_skipped--;
    }

    byte_t *buf_pos = buffer;
    while (bytes_left_to_read > 0 && current_cluster != 0xFFFF) {
        int bytes_to_read = (bytes_left_to_read > (512 - offset_within_cluster)) ? (512 - offset_within_cluster) : bytes_left_to_read;
        fat16_read_data_from_cluster(current_cluster, buf_pos, bytes_to_read, offset_within_cluster);

        dbgprintf("Read %d bytes from cluster 0x%x\n", bytes_to_read, current_cluster); 

        buf_pos += bytes_to_read;
        bytes_left_to_read -= bytes_to_read;

        /* Fetch next cluster from FAT table and reset offset_within_cluster for subsequent clusters. */
        current_cluster = fat16_get_fat_entry(current_cluster);
        offset_within_cluster = 0;  /* After the first cluster, we read from the start of subsequent clusters. */
    }

    if (bytes_left_to_read > 0) {
        dbgprintf("Unexpected end of data\n");
        return -3;  /* Unexpected end of data */
    }

    return total_bytes_to_read;
}