#!/bin/bash

# Function to check if a package is installed
check_and_install() {
    PACKAGE=$1
    if dpkg -s $PACKAGE &> /dev/null; then
        echo "$PACKAGE is already installed."
    else
        echo "$PACKAGE is not installed. Installing..."
        sudo apt-get install -y $PACKAGE
    fi
}

# Update package list
echo "Updating package list..."
sudo apt-get update

# Check and install each package
check_and_install "build-essential"
check_and_install "gcc-multilib"
check_and_install "grub2"
check_and_install "xorriso"
check_and_install "xxd"

echo "All dependencies are installed."

