#
# Dev container Dockerfile for RetrOS-32
# Based on the project's main Dockerfile with improvements for development
#

FROM --platform=linux/amd64 ubuntu:latest

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install all required packages in one layer with proper error handling
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    gcc-multilib \
    g++-multilib \
    grub2 \
    xorriso \
    xxd \
    sudo \
    kmod \
    mtools \
    dosfstools \
    git \
    vim \
    ca-certificates \
    curl \
    wget && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /RetrOS-32

# Set back to default
ENV DEBIAN_FRONTEND=dialog

# Default command
CMD ["/bin/bash"]
