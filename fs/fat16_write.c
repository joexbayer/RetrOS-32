#include <stdint.h>
#include <fs/fat16.h>
#include <kutils.h>
#include <diskdev.h>
#include <serial.h>

uint32_t fat16_find_cluster_by_offset(struct fat16_directory_entry* entry, int offset, int* cluster_offset)
{
    uint32_t current_cluster = entry->first_cluster;
    int traversed_bytes = 0; 

    while (current_cluster < 0xFFF8 && current_cluster != 0xFFFF && traversed_bytes + 512 <= offset){
        traversed_bytes += 512;
        current_cluster = fat16_get_fat_entry(current_cluster);
    }

    *cluster_offset = offset - traversed_bytes;
    return current_cluster;
}

int fat16_write_data_to_cluster_with_offset(uint32_t cluster, int offset, byte_t* data, int data_length)
{
    int block_num = get_root_directory_start_block() + cluster; /* Assuming you have a function for this. */
    write_block_offset(data, data_length, offset, block_num);

    return 0;
}


int fat16_write_data_to_cluster(uint32_t cluster, const void *data, int data_length)
{
    if (data_length > 512) {  /* Assuming 512-byte clusters */
        return -1;  /* Error: Data too large for one cluster */
    }

    /* Calculate the block number based on the cluster. */
    uint16_t block_num = get_root_directory_start_block() + cluster;

    return write_block((byte_t *)data, block_num);
}

int fat16_write(struct fat16_directory_entry* entry, int offset, void* data, int data_length)
{
    int remaining_data_length = data_length;
    int data_offset = 0;

    /* Determine the cluster and inner cluster offset where we should start writing. */
    int cluster_offset;
    uint32_t current_cluster = fat16_find_cluster_by_offset(entry, offset, &cluster_offset);
    int next_cluster = current_cluster;

    while (remaining_data_length > 0){
        /* If there is no allocated cluster or we've reached the end of the cluster chain, allocate a new one. */
        if (!next_cluster || next_cluster == 0xFFFF || next_cluster >= 0xFFF8){
            uint32_t free_cluster = fat16_get_free_cluster();
            if (!free_cluster) return -1;  /* Error: No free clusters */

            if (current_cluster){
                fat16_set_fat_entry(current_cluster, free_cluster);
            } else {
                entry->first_cluster = free_cluster;  /* Update the entry's starting cluster */
            }
            current_cluster = free_cluster;
        }

        /* Determine how much to write in this iteration. */
        int write_size = 512 - cluster_offset;  /* Account for the offset inside the cluster. */
        if (write_size > remaining_data_length) write_size = remaining_data_length;

        /* Write data to the current cluster at the given offset. */
        int write_result = fat16_write_data_to_cluster_with_offset(current_cluster, cluster_offset, ((byte_t*)data) + data_offset, write_size);
        if (write_result < 0){
            dbgprintf("Error writing data to cluster\n");
            return -2;   /* Error: Couldn't write data */
        }

        //dbgprintf("Wrote %d bytes to cluster %d\n", write_size, current_cluster);

        /* Update our counters and pointers. */
        remaining_data_length -= write_size;
        data_offset += write_size;
        cluster_offset = 0;  /* Reset cluster offset for subsequent clusters. */

        /* Move to the next cluster in the chain. */
        next_cluster = fat16_get_fat_entry(current_cluster);
    }

    /* Mark the last cluster in the chain as end-of-file if necessary. */
    if (fat16_get_fat_entry(current_cluster) != 0xFFFF){
        fat16_set_fat_entry(current_cluster, 0xFFFF);
    }

    return 0;  /* Success */
}