
<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]
-->

<!-- PROJECT LOGO -->
<br />
<div align="center">
  <h1 align="center">RetrOS 32bit</h1>
  

  <p align="center">
    <img src="https://github.com/joexbayer/RetrOS-32/blob/main/graphics/logo.png?raw=true" width="150">
  </p>

  ![Build](https://github.com/joexbayer/RetrOS-32/actions/workflows/pipeline.yml/badge.svg)
  
  <p align="center">
    Hobby 32bit operatingsystem project focusing on networking on i386 architecture.
    <br />
    <a href="https://github.com/joexbayer/RetrOS-32/tree/main/docs"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/joexbayer/RetrOS-32">View Demo</a>
    ·
    <a href="https://github.com/joexbayer/RetrOS-32/issues">Report Bug</a>
    ·
    <a href="https://github.com/joexbayer/RetrOS-32/issues">Request Feature</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
<p align="center">
  <img src="https://github.com/joexbayer/RetrOS-32/blob/development/docs/pictures/hardwaredemo.gif?raw=true">
</p>

| | | |
|:-------------------------:|:-------------------------:|:-------------------------:|
| <img src="https://github.com/joexbayer/RetrOS-32/blob/main/docs/pictures/screenshot1.png?raw=true"> | <img src="https://github.com/joexbayer/RetrOS-32/blob/main/docs/pictures/screenshot2.png?raw=true"> | <img src="https://github.com/joexbayer/RetrOS-32/blob/main/docs/pictures/screenshot3.png?raw=true">
## About The Project

<p align="center">32bit Hobby Operatingsystem with graphics, multitasking and networking!</p>
<p align="center"><i>Started: 12.05.2022</i></p>

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<p align="center">
  <img src="https://github.com/joexbayer/RetrOS-32/blob/main/docs/pictures/textmode.png?raw=true">
</p>

### Login
There are 3 default users: system, admin and guest. The password for admin is 'admin', while guest has no password.
Currently there is no difference between admin and guest.

You can create a user with the 'admin' command:
```sh
admin create <username> <password>
```

### Built With

This project is built with C & Assembly for the kernel, utilities and build system. C++ for userspace applications and Make for compilation.
Docker used for crossplatform compilation.

![Docker](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)
![](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Hardware
Tested on:
Lenovo x240,
Asus Eee PC series,
Dell Optiplex 780,
Samsung N150 Plus,
Samsung NP-NC10,
IBM Thinkpad a21p

| | |
|:-------------------------:|:-------------------------:|
| <img src="https://github.com/joexbayer/RetrOS-32/blob/main/docs/pictures/hardware1.jpg?raw=true" width="250"> | <img src="https://github.com/joexbayer/RetrOS-32/blob/main/docs/pictures/hardware2.jpg?raw=true" width="250">

<!-- GETTING STARTED -->
## Getting Started

### Prerequisites

Crossplatform: Docker to compile the image file and QEMU for emulation.<br>
For native compilation you will need:
 * i386-elf-gcc, i386-elf-ld and i386-elf-g++ for MacOS.
 * build-essential and gcc-multilib for Linux / WSL (ubuntu)
 *   Also need: grub2, xorriso and xxd (for using Grub as bootloader)


### Installation

_To compile the kernel and its needed programs you simply need to run *make img* and *make qemu* to open QEMU_

#### Linux
1. Clone the repo
   ```sh
   git clone https://github.com/joexbayer/RetrOS-32.git
   ```
2. Check that all dependencies are installed (Only for debian based distros)
   ```sh
   ./debian.sh
   ```

3. Initialize Git submodules (C-Compiler)
   ```sh
   git submodule update --init --recursive
   ```

4. Compile and create image
   ```sh
   make compile
   make img
   ```
   
5. Launch QEMU
   ```sh
   make qemu
   ```

6. Use GRUB (Optional)
   ```sh
   make grub
   ```

#### MacOS

Currently MacOS cannot natively compile the build tools as they depend on 32bit x86 code.
Docker is the simplest way if you still wish to compile the operating system.

1. Clone the repository
   ```sh
   git clone https://github.com/joexbayer/RetrOS-32.git
   ```

2. Initialize Git submodules (C-Compiler)
   ```sh
   git submodule update --init --recursive
   ```

3. Build using Docker
   ```sh
   docker-compose up --build
   ```
 
#### Windows
Use Docker (see MacOS section) or WSL (see Linux section).

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->
## Usage

To run RetrOS-32:
  * http://copy.sh/v86/ : Upload .img file as Hard disk image.
  * QEMU: `make qemu` or `qemu-system-i386 <image name>` 
  * Real hardware: Burn .iso file to USB or CD (Use a GRUB iso)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Project Structure (TBD)
    NETOS - Project
    ├── Dockerfile 
    ├── LICENSE.txt
    ├── Makefile
    ├── apps (userspace applications)
    │   ├── Makefile
    │   ├── <app>
    │   ├── libcore.a
    │   ├── libgraphic.a
    │   ├── libnet.a
    │   ├── readme.md
    │   └── utils
    │       └── *.cpp
    ├── bin 
    │   └── *.o
    ├── boot (custom bootloader)
    │   ├── bootloader.s
    │   └── multiboot.ld
    ├── docker-compose.yml
    ├── docs (documentation)
    │   ├── *.md
    ├── drivers (kernel drivers)
    │   └── *.c
    ├── fs (filesystem)
    │   ├── Makefile
    │   ├── bin
    │   └── *.c
    ├── graphics
    │   └── *.c
    ├── include
    │   ├── arch
    │   │   └── *.h
    │   ├── fs
    │   │   └── *.h
    │   ├── gfx
    │   │   └── *.h
    │   ├── lib
    │   │   └── *.h
    │   ├── net
    │   │   └── *.h
    │   └── *.h
    ├── kernel (main kernel source files)
    │   ├── arch
    │   │   └── *.c
    │   ├── kthreads
    │   │   └── *.c
    │   └── *.c
    ├── legacy (old code)
    ├── lib (libraries)
    │   └── *.c
    ├── net (networking code)
    │   └── *.c
    ├── readme.md
    ├── rootfs (root filesystem for the OS)
    ├── tests (testing code)
    │   ├── Makefile
    │   ├── bin
    │   ├── *_test.c
    │   └── readme.md
    └── tools (build tools)
        ├── scripts
        └── bin
            └── *.c

<!-- ROADMAP -->
## Roadmap

- [x] Custom Bootloader & GRUB compatible
- [x] Stage 2 bootloader 
- [x] Wallpapers
- [x] Mountable image
- [x] Users 
- [x] Multi-threaded pre-emptive scheduling
- [x] GDT & TSS kernel / userspace separation
- [x] Interrupt handling
- [x] PS/2 Keyboard & Mouse, PIT, VESA (640x480x8), RTC, Serial drivers, E1000
- [x] PCI.
- [x] 8Bit RGB to 8Bit VGA
- [x] Filesystem
- [x] Textmode VGA only
- [x] Networkstack
  - [x] Ethernet, IP, ARP, UDP
  - [x] Socket API
  - [x] Interfaces
    - [x] Loopback
  - [x] DHCP
  - [x] DNS
  - [x] TCP
  - [x] Netcat style commands
  - [ ] Webserver
  - [ ] FTP
  - [ ] IRC
  - [ ] SSH
  - [ ] Telnet
  - [ ] HTTP
  - [ ] HTML
- [x] ATA IDE Driver
  - [x] Ext2 (like) Filesystem
  - [x] FAT16, support for up to 32mb files.
  - [x] read / write
  - [ ] Atapi
- [x] Memory
  - [x] 32bit Virtual Memory
  - [x] kalloc / kfree (kernel)
  - [x] malloc / free (userspace) 
- [x] Graphics
  - [x] Window Manager
  - [x] GFXLib
  - [x] Double framebuffer rendering
  - [x] Mouse events
  - [x] Dynamic resize
  - [x] Fullscreen
  - [x] Widgets Library
  - [ ] Custom HTML to Widgets
- [x] Terminal, Window Server, Process Informtaion, Finder.
- [x] Library (printf, memcpy, etc)
- [x] IPC
- [x] System calls
- [x] C Compiler / interpreter to bytecode
- [x] Custom VM to run bytecode.
- [x] Editor with Syntax Highlighting
- [x] Calculator
- [x] 3D Game
- [x] Snake game (Textmode only)
- [x] Userspace threads (shared virtual memory)
- [x] Remote virtual terminal over TCP
- [x] LZ compression library
- [x] Tools
  - [x] Build
  - [x] Sync / Migration Took
  - [x] mkfs
  - [x] Testing
  - [ ] Create & Encode icons and wallpapers

See the [open issues](https://github.com/joexbayer/RetrOS-32/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Joe Bayer

Project Link: [https://github.com/joexbayer/RetrOS-32](https://github.com/joexbayer/RetrOS-32)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

Use this space to list resources you find helpful and would like to give credit to. I've included a few of my favorites to kick things off!

* README Template(https://github.com/othneildrew/Best-README-Template/blob/master/README.md)

<p align="right">(<a href="#readme-top">back to top</a>)</p>
