
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
  <h1 align="center">NET/OS</h1>

  <p align="center">
    Hobby 32bit operatingsystem project focusing on networking on i386 architecture.
    <br />
    <a href="https://github.com/joexbayer/NETOS"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/joexbayer/NETOS">View Demo</a>
    ·
    <a href="https://github.com/joexbayer/NETOS/issues">Report Bug</a>
    ·
    <a href="https://github.com/joexbayer/NETOS/issues">Request Feature</a>
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
<img src="https://github.com/joexbayer/NETOS/blob/main/graphics/screenshot.png?raw=true">)
</p>

## About The Project

32bit Hobby Operatingsystem with graphics, multitasking and networking!

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
   git clone https://github.com/joexbayer/NETOS.git
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
   git clone https://github.com/joexbayer/NETOS.git
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



<!-- ROADMAP -->
## Roadmap

- [x] Bootloader
- [x] Multi-threaded pre-emptive scheduling
- [x] GDT & TSS kernel / userspace separation
- [x] Interrupt handling
- [x] PS/2 Keyboard & Mouse, PIT, VESA (640x480x8), RTC, Serial drivers, E1000
- [x] PCI.
- [x] Networkstack
  - [x] Ethernet, IP, ARP, UDP
  - [x] Socket API
  - [x] DHCP
  - [x] DNS
  - [ ] TCP
  - [ ] HTTP
- [x] ATA IDE Driver
  - [x] Ext2 (like) Filesystem
  - [x] read / write
- [x] Memory
  - [x] 32bit Virtual Memory
  - [x] kalloc / kfree (kernel)
  - [x] malloc / free (process) 
- [x] Graphics
  - [x] Window Manager
  - [x] GFXLib
  - [x] Double framebuffer rendering
  - [x] Mouse events
  - [ ] Dynamic resize
- [x] Terminal, Window Server, Process Informtaion.
- [x] Library (printf, memcpy, etc)
- [ ] IPC
- [x] System calls
- [x] Tools
  - [x] Build
  - [x] mkfs
  - [x] Testing
### General Improvements

1. Less hardcoded arrays, more linked lists.
2. More tables like sys_call_table.
3. Complete rework of pcb and scheduling.
4. Virtual Memory rework.
5. Complete rework of socket.c
6. SKB rework.
7. sync.c rework (depends on pcb)

See the [open issues](https://github.com/joexbayer/NETOS/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Joe Bayer - joeba@uio.com

Project Link: [https://github.com/joexbayer/NETOS](https://github.com/joexbayer/NETOS)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

Use this space to list resources you find helpful and would like to give credit to. I've included a few of my favorites to kick things off!

* README Template](https://github.com/othneildrew/Best-README-Template/blob/master/README.md)

<p align="right">(<a href="#readme-top">back to top</a>)</p>
