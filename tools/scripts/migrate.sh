#!/bin/bash
# migrate.sh

set -e

SRC_IMG=$1
DST_IMG=$2
MNT_SRC="./mnt_src"
MNT_DST="./mnt_dst"

# Validate input arguments
if [[ -z "$SRC_IMG" || -z "$DST_IMG" ]]; then
    echo "Usage: $0 <source_image> <destination_image>"
    exit 1
fi

mkdir -p $MNT_SRC $MNT_DST

# Mount the source and destination images
sudo mount -o shortname=winnt $SRC_IMG $MNT_SRC
sudo mount -o shortname=winnt $DST_IMG $MNT_DST

# Copy contents from source image to destination image
sudo cp -vvv -r $MNT_SRC/* $MNT_DST/

# Unmount both images
sudo umount $MNT_SRC
sudo umount $MNT_DST

echo "Migration completed!"
