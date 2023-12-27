
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
    <img src="https://github.com/joexbayer/RetrOS-32/blob/main/graphics/logo.png?raw=true">
  </p>

  <p align="center">
    Hobby 32bit operatingsystem project focusing on networking on i386 architecture.
    <br />
    <a href="https://github.com/joexbayer/RetrOS-32"><strong>Explore the docs »</strong></a>
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
  <img src="https://github.com/joexbayer/RetrOS-32/blob/main/graphics/screenshot2.png?raw=true">
</p>

| | |
|:-------------------------:|:-------------------------:|
| <img src="https://github.com/joexbayer/RetrOS-32/blob/main/graphics/screenshot1.png?raw=true"> | <img src="https://github.com/joexbayer/RetrOS-32/blob/main/graphics/screenshot3.png?raw=true">
## About The Project

<p align="center">32bit Hobby Operatingsystem with graphics, multitasking and networking!</p>
<p align="center"><i>Started: 12.05.2022</i></p>

<p align="right">(<a href="#readme-top">back to top</a>)</p>



### Built With

This project is built with C & Assembly for the kernel, utilities and build system. C++ for userspace applications and Make for compilation.
Docker used for crossplatform compilation.

![Docker](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)
![](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)


<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

### Prerequisites

Crossplatform: Docker to compile the .iso file and QEMU for emulation.<br>
For native compilation you will need:
 * i386-elf-gcc, i386-elf-ld and i386-elf-g++ for MacOS.
 * build-essential and gcc-multilib for Linux / WSL (ubuntu)


### Installation

_To compile the kernel and its needed programs you simply need to run *make iso* and *make qemu* to open QEMU_

#### MacOS / Linux

##### MacOS can compile the kernel but not the needed tools.
1. Clone the repo
   ```sh
   git clone https://github.com/joexbayer/RetrOS-32.git
   ```
2. Compile the kernel / OS
  
   Docker:
   ```sh
   sudo docker-compose up
   ```
3. Launch QEMU
   ```sh
   make qemu
   ```
#### Windows
  Using Docker (can also compile in WSL)

1. Clone the repo
   ```sh
   git clone https://github.com/joexbayer/RetrOS-32.git
   ```

2. Compile and Launch QEMU
   ```sh
   ./make.cmd run
   ```
<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

Use this space to show useful examples of how a project can be used. Additional screenshots, code examples and demos work well in this space. You may also link to more resources.

_For more examples, please refer to the [Documentation](https://example.com)_

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Hardware
Tested on:
Lenovo x240,
Asus Eee PC series,
Dell Optiplex 780,
Samsung N150 Plus,
Samsung NP-NC10,
IBM Thinkpad a21p

## Project Structure (TBD)
    NETOS - Project
    ├── Dockerfile 
    ├── LICENSE.txt
    ├── Makefile
    ├── apps (userspace applications)
    │   ├── Makefile
    │   ├── ...
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
- [x] Multi-threaded pre-emptive scheduling
- [x] GDT & TSS kernel / userspace separation
- [x] Interrupt handling
- [x] PS/2 Keyboard & Mouse, PIT, VESA (640x480x8), RTC, Serial drivers, E1000
- [x] PCI.
- [x] 8Bit RGB to 8Bit VGA
- [x] Filesystem
- [x] Textmode only
- [x] Networkstack
  - [x] Ethernet, IP, ARP, UDP
  - [x] Socket API
  - [x] Interfaces
    - [x] Loopback
  - [x] DHCP
  - [x] DNS
  - [x] TCP
  - [ ] HTTP
  - [ ] HTML
- [x] ATA IDE Driver
  - [x] Ext2 (like) Filesystem
  - [x] FAT16, support for up to 32mb files.
  - [x] read / write
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
- [x] Terminal, Window Server, Process Informtaion, Finder.
- [x] Library (printf, memcpy, etc)
- [x] IPC
- [x] System calls
- [x] C Compiler / interpreter to bytecode
- [x] Custom VM to run bytecode.
- [x] Editor with Syntax Highlighting
- [x] Calculator
- [x] 3D Game
- [x] Userspace threads (shared virtual memory)
- [x] Remote virtual terminal over TCP
- [x] Tools
  - [x] Build
  - [ ] Sync / Migration Took
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

Joe Bayer - joeba@uio.no

Project Link: [https://github.com/joexbayer/RetrOS-32](https://github.com/joexbayer/RetrOS-32)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

Use this space to list resources you find helpful and would like to give credit to. I've included a few of my favorites to kick things off!

* README Template(https://github.com/othneildrew/Best-README-Template/blob/master/README.md)

<p align="right">(<a href="#readme-top">back to top</a>)</p>
