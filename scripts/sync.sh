#!/bin/bash

# Check for the correct number of arguments
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <img_file>"
    exit 1
fi

# Get the image file from the command-line argument
img_file="$1"

# Create the mount directory if it doesn't exist
mkdir -p mnt

# Mount the specified image file with shortname=winnt option
sudo mount -o shortname=winnt "$img_file" ./mnt

# Copy files from rootfs to the mounted directory using rsync
# sudo rsync -av rootfs/* ./mnt/
sudo cp -vvv -r rootfs/* ./mnt/

# Unmount the mounted directory
sudo umount ./mnt

echo "RetrOS image operations completed successfully."
