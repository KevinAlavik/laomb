# Laomb

An i586 OS built from scratch, targeting a Pentium MMX using the Hyper bootloader and a FAT32 filesystem with MBR.

## Features

- **Architecture**: 32-bit x86 (i586)
- **Bootloader**: [Hyper Bootloader](https://github.com/UltraOS/Hyper)
- **Filesystem**: MBR Disk with a boot FAT32 partition

## Goals and Modules

### 1. Kernel
- [ ] **Interrupt Management**
    - [X] IDT and IRQ Handling using Intel 8259
    - [X] Basic ISR Support (Exception Handling)
    - [ ] User Mode Interrupts
- [ ] **Memory Management**
    - [X] Bitmap Physical Memory Management
    - [X] Linear Memory Manager using IA-32 Paging
    - [X] Kernel Virtual Memory Allocator (Heap)
    - [ ] On-Demand Paging with On-Disk Paging Out
    - [ ] Hardware Memory Passthrough (DMA, Framebuffer, etc)
- [ ] **Scheduler**
    - [X] Basic Priority-Based Robin Scheduler
    - [X] Cooperative Scheduler
    - [ ] Custom Queues
    - [X] Priority-Based Scheduler
    - [ ] Multi-threading Support
- [ ] **Virtual Filesystem (VFS)**
    - [X] Basic RAM-Filesystem
    - [X] VFS Mounting and Integration
    - [X] Raw Device FS (devfs)
    - [X] Partitions in devfs
    - [ ] Real Filesystem Integration (FAT32/Ext2)
    - [X] VFS-Based File Permissions
    - [ ] Special Filesystems
        - [ ] Standart Input and Output with Child to Parent Redirection VFS Devices (`/stdin` `/stdout`)
        - [ ] Random Access Memory Passthrough VFS Device (`/mem`)
        - [ ] Scheduler VFS Device (`/sched`)
        - [ ] Kernel Configuration VFS Device (`/krnl`)
        - [ ] VFS Based IPC Devices
- [ ] **Device Management**
    - [ ] PCI Device Detection and Management
    - [ ] Floppy Disk Driver
    - [ ] CD-ROM Driver
    - [ ] HDD Driver
        - [X] ATA PIO Driver
        - [ ] ATA DMA Driver
    - [ ] PS/2 Keyboard and Mouse Drivers
- [ ] **I/O Subsystem**
    - [ ] Character Device Driver Framework
    - [ ] Block Device Driver Framework
    - [ ] I/O Port and MMIO Access Abstractions
- [ ] **Disk Drivers**
    - [X] ATA PIO
    - [X] MBR Parsing
    - [ ] Raw FAT32 Driver
    - [ ] Ext2 Driver
    - [ ] DMA Handling for ATA
- [ ] **Process Management**
    - [X] Process Creation and Destruction
    - [X] Context Switching
    - [ ] Signal Handling
    - [ ] Process Prioritization
- [ ] **Userspace Calls Interface**
    - [ ] Basic System Call Implementation
    - [ ] Entering and Exiting Userspace
    - [ ] Elf32 Parsing
- [ ] **Inter-Process Communication (IPC)**
    - [ ] Shared Memory
    - [ ] Message Queues
    - [ ] Semaphores and Mutexes

### 2. Userspace
- [ ] **Userspace Libraries**
    - [ ] mlibc Port with Custom GCC Toolchain
    - [ ] Kernel Specific C Library
- [ ] **Userspace Processes**
    - [ ] Userspace Shell
    - [ ] Loading commands with children tasks
- [ ] **Graphics Composer Service**
    - [ ] Framebuffer Rendering Service with Double Buffering
    - [ ] Window Management Service

### 3. File Systems
- [X] **RAMFS** (Basic Read/Write Support)
- [ ] **FAT32** (Read/Write Support with LFN)
- [ ] **Ext2** (Read, Write Support)
- [ ] **ISO9660** (CD-ROM Filesystem)

### 4. Networking (Optional/Long-term)
- No networking planned for this architecture due to hardware limitations.

### Architecture Specific Choices
- Flat memory model
- IA-32 Paging with 4K pages
- i586, specifically targeting a Pentium MMX
- No ACPI or USB support
- Target Hardware: 3/4 Floppy Drive, ATAPI CD-ROM, LBA HDD (PIO/DMA), Cirrus Logic GD 5602 (VGA)
