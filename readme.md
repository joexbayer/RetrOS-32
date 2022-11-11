
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
  <h3 align="center">NET/OS</h3>

  <p align="center">
    Hobby operatingsystem project focusing on networking.
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
## About The Project

[![Product Name Screen Shot][product-screenshot]](https://example.com)

TODO

<p align="right">(<a href="#readme-top">back to top</a>)</p>



### Built With

This section should list any major frameworks/libraries used to bootstrap your project. Leave any add-ons/plugins for the acknowledgements section. Here are a few examples.

![Docker](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)
![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)


<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

This is an example of how you may give instructions on setting up your project locally.
To get a local copy up and running follow these simple example steps.

### Prerequisites

Only need Docker to compile the .iso file and QEMU for emulation.

### Installation

_To compile the kernel and its needed programs you simply need to run *make* and *make qemu* to open QEMU_

#### MacOS / Linux
1. Clone the repo
   ```sh
   git clone https://github.com/joexbayer/NETOS.git
   ```
2. Compile the kernel / OS
  
   Native:
   ```sh
   make
   ```
   Docker:
   ```sh
   sudo docker-compose up
   ```
3. Launch QEMU
   ```sh
   make QEMU
   ```
#### Windows
  For Windows you have 2 options:
    Build using WSL (need to intall build-essentials packages)
    or Docker

1. Clone the repo
   ```sh
   git clone https://github.com/joexbayer/NETOS.git
   ```
2. Compile the kernel / OS
  
   WSL:
   ```sh
   ./make.cmd compile
   ```
   Docker:
   ```sh
   docker-compose up
   ```
3. Launch QEMU
   ```sh
   make QEMU
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
- [x] Keyboard, PIT, VGA, RTC, Serial drivers.
- [x] E1000 NIC driver.
- [x] Networkstack
  - [x] Ethernet, IP, ARP, UDP
  - [x] Socket API
  - [x] DHCP
  - [x] DNS
  - [ ] TCP
- [x] ATA IDE Driver
  - [x] INODE Filesystem
  - [x] read / write
- [x] Memory
  - [x] Virtual Memory
- [x] Window Manager
  - [x] Windows
  - [ ] Dynamic resize


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
