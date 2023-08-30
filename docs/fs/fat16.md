## Layout

DISK:
| Boot Sector | FAT Table | | Root Directory | Data Clusters |

The Boot Sector contains the FAT16 bootblock information, giving all needed information to find the FAT table and root directory.
The FAT Table contains a list of all clusters and their status, the size is determined by the number of clusters on the disk.
The Root Directory contains a list of all files and directories in the root directory.

FAT:
| Entry 0 | Entry 1 | Entry 2 | ... | Entry N |

## To read a file:

Start with its First Cluster number from its directory entry.

Read that cluster.

Look up the next cluster in the FAT.

Repeat until you reach a FAT entry of 0xFFFF.

## Writing a File

Find a free cluster (FAT entry of 0x0000).

Update the FAT to indicate it's used.

If more clusters are needed, repeat the above steps and ensure the previous FAT entry points to this new cluster.

Continue until the entire file is written, marking the last FAT entry with 0xFFFF.