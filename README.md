# Laomb

An i586 OS built from scratch, targeting a Pentium MMX using the Hyper bootloader and a FAT32 filesystem with MBR.

## Features

- **Architecture**: 32-bit x86 (i586)
- **Bootloader**: [Hyper Bootloader](https://github.com/UltraOS/Hyper)
- **Filesystem**: MBR Disk with a boot FAT32 partition

## Goals and Modules

### 1. Kernel
- [x] **Interrupt Management**
    - [x] IDT and IRQ Handling using Intel 8259
    - [x] Basic ISR Support (Exception Handling)
    - [ ] User Mode Interrupts (Planned)
- [ ] **Memory Management**
    - [x] Bitmap Physical Memory Management
    - [x] Linear Memory Manager using IA-32 Paging
    - [x] Kernel Virtual Memory Allocator (Heap)
    - [ ] On-Demand Paging with On-Disk Paging Out (Planned)
    - [ ] Hardware Memory Passthrough (DMA, Framebuffer, etc)
- [ ] **Scheduler**
    - [x] Basic Round Robin Scheduler `Planned to be replaced with a priority scheduler`
    - [x] Preemptive Scheduler
    - [ ] Blocking and Waiting for Resources (Planned)
    - [ ] Priority-Based Scheduler (Planned)
    - [ ] Multi-threading Support (Planned)
- [ ] **Virtual Filesystem (VFS)**
    - [x] Basic RAM-Filesystem
    - [x] VFS Mounting and Integration
    - [ ] Real Filesystem Integration (FAT32/Ext2)
    - [ ] VFS-Based File Permissions (Planned)
    - [ ] Special Filesystems
        - [ ] Standart Input and Output with Child to Parent Redirection VFS Devices (`/stdin` `/stdout`)
        - [ ] Random Access Memory Passthrough VFS Device (`/mem`)
        - [ ] Scheduler VFS Device (`/sched`)
        - [ ] Kernel Configuration VFS Device (`/krnl`)
        - [ ] VFS Based IPC Devices
- [ ] **Device Management**
    - [ ] PCI Device Detection and Management
    - [ ] Floppy Disk Driver (Planned)
    - [ ] CD-ROM Driver (Planned)
    - [ ] HDD Driver (ATA/DMA Integration)
    - [ ] PS/2 Keyboard and Mouse Drivers
- [ ] **I/O Subsystem**
    - [ ] Character Device Driver Framework
    - [ ] Block Device Driver Framework
    - [ ] I/O Port and MMIO Access Abstractions
- [ ] **Disk Drivers**
    - [ ] ATA PIO (Currently Broken)
    - [x] MBR Parsing
    - [x] Raw FAT32 Driver (Possibly Broken)
    - [ ] Ext2 Driver (Planned)
    - [ ] DMA Handling for ATA (Planned)
- [ ] **Process Management**
    - [x] Process Creation and Destruction
    - [x] Context Switching
    - [ ] Full Process States (Running, Waiting, Blocked)
    - [ ] Signal Handling (Planned)
    - [ ] Process Prioritization (Planned)
- [ ] **Userspace Calls Interface**
    - [ ] Basic System Call Implementation
    - [ ] Entering and Exiting Userspace
    - [ ] Elf32 Parsing
- [ ] **Inter-Process Communication (IPC)**
    - [ ] Shared Memory
    - [ ] Message Queues (Planned, as interrupt blocking is planned to be based on queues)
    - [ ] Semaphores and Mutexes

### 2. Userspace
- [ ] **Userspace Libraries**
    - [ ] mlibc Port with Custom GCC Toolchain
    - [ ] Kernel Specific C Library
- [ ] **Userspace Processes**
    - [ ] Userspace Shell
    - [ ] Loading commands with children tasks
- [ ] **Graphics Composer Service**
    - [ ] Framebuffer Rendering Service with Double Buffering (Planned)
    - [ ] Window Management Service (Planned)

### 3. File Systems
- [x] **RAMFS** (Basic Read/Write Support)
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
