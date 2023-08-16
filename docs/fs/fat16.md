## Layout

DISK:
| Boot Sector | FAT1 | FAT2 | Root Directory | Data Clusters |

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