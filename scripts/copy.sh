#!/bin/bash
# copy_to_image.sh

set -e

DATA_DIR=$1
IMG=$2
MNT_DIR="./mnt"

# Validate input arguments
if [[ -z "$DATA_DIR" || -z "$IMG" ]]; then
    echo "Usage: $0 <data_directory> <destination_image>"
    exit 1
fi

mkdir -p $MNT_DIR

# Mount the destination image
sudo mount -o shortname=winnt $IMG $MNT_DIR

# Copy specified data to the image
sudo cp -vvv -r $DATA_DIR/* $MNT_DIR/

# Unmount the image
sudo umount $MNT_DIR

echo "Copy completed!"
