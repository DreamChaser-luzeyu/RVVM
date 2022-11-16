
# RVVM - The RISC-V Virtual Machine
![version](https://img.shields.io/badge/version-0.5.rc-brightgreen)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/LekKit/RVVM.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/LekKit/RVVM/context:cpp)
![RISC-V Logo](https://riscv.org/wp-content/uploads/2018/09/riscv-logo-1.png "The “RISC-V” trade name is a registered trade mark of RISC-V International.")

RISC-V CPU & System software implementation written in С

## What's working
- Passes RISC-V compliance/torture tests for both RV64 & RV32
- OpenSBI, U-Boot, custom firmwares boot and execute properly
- Linux, FreeBSD, OpenBSD & other cool guest OSes fully work
- Tracing JIT, multicore support
- Framebuffer graphics, working mouse & keyboard, interactive shell through UART
- NVMe storage drives
- Networking (WIP)
- Haiku OS guest support (WIP)

## Tell me more...
- Feature-complete RV64IMAFDC instruction set
- Multicore support (SMP), SV32/SV39/SV48/SV57 MMU
- Tracing RVJIT with x86_64, i386, ARM64, ARM, RISC-V backends
  (faster than QEMU, yay!)
- Bootrom, kernel Image loading
- Device Tree auto-generation, passing to firmware/kernel
- RVVM Public API for VM integration
- UART 16550a-compatible text console
- PLIC/ACLINT, Timers, Poweroff/reset
- Generic PCI Express Bus
- NVMe storage, image TRIM support, fast IO
- Graphical framebuffer through X11/WinAPI
- PS2 Altera Controller, PS2 keyboard & mouse
- ATA hard drive (PIO / IDE PCI), deprecated
- OpenCores Ethernet through Linux TAP, WIP usernet

## Building
Currently builds using GNU Make (recommended) or CMake and works on Linux, Windows, MacOS, and many other POSIX systems.
```
git clone https://github.com/LekKit/RVVM
cd RVVM
make
cd release.linux.x86_64
./rvvm_x86_64 -h
```
You can configure the build with USE flags.To cross-compile, pass CC=target-gcc to make. If it fails to detect features, pass ARCH/OS variables explicitly.

Examples:
```
make lib CC=aarch64-linux-android21-clang USE_FB=0
make lib all CC=x86_64-w64-mingw32-gcc USE_NET=1
make CFLAGS=-m32 ARCH=i386 USE_RV64=0 BUILDDIR=build BINARY=build/rvvm
make CC=mipseb-linux-gnu-gcc USE_JIT=0
```
Alternatively, you can use CMake:
```
git clone https://github.com/LekKit/RVVM
cd RVVM
mkdir build
cmake -S. -Bbuild
cmake --build build --target all
cd build
./rvvm -h
```

## Running
```
./rvvm fw_jump.bin -k u-boot_s.bin -i drive.img -m 2G -smp 2 -res 1280x720
```
Argument explanation:
```
[fw_jump.bin]          Initial M-mode firmware, OpenSBI in this case
-k, -kernel u-boot.bin S-mode kernel payload (Linux Image, U-Boot, etc)
-i, -image drive.img   Attach NVMe storage image (Raw format as of now)
-m, -mem 2G            Memory amount (may be suffixed by k/M/G), default 256M
-s, -smp 2             Amount of cores, single-core machine by default
-res 1280x720          Changes framebuffer & VM window resolution
 . . .
-rv32                  Enable 32-bit RISC-V, 64-bit by default
-cmdline, -append ...  Override/append default kernel command line
-nogui, -nojit         Disable GUI (Use only UART), Disable JIT (for debugging)
```
Invoke "./rvvm -h" to see extended help.

## Tested environments
| OS         | JIT                        |   GUI   |
|------------|----------------------------|---------|
| Linux      | Works                      |   X11   |
| Windows    | Works                      |  Win32  |
| MacOS X    | Works, need signing for M1 | XQuartz |
| FreeBSD    | Works                      |   X11   |
| Haiku OS   | Works                      |  *WIP*  |
| Serenity   | Broken due to W^X safety   |    -    |
| Windows CE | Broken                     |  Win32  |
| Emscripten | -                          |    -    |

## Contributions
|                      | Achievments | Working on |
|----------------------|-------------|------------|
| **LekKit**           | RVVM API & codebase infrastructure <br> RV64IMAFDC interpreter, IRQ/MMU/Priv <br> RVJIT Compiler, X86/RISC-V backends <br> NVMe, PCIe rework, ACLINT & other devices <br> X11/Win32 GUI | Networking, GUI |
| **cerg2010cerg2010** | Important fixes, initial RV64 work <br> PLIC, PCI bus, PS2 devices, ATA drive, Ethernet OC <br> ARM/ARM64 RVJIT backends | Testing, Assistance |
| **Mr0maks**          | Initial C/M ext interpreter, VM debugger, CSR work, basic UART <br> ARM32 mul/div JIT intrinsics | - |
| **0xCatPKG**         | Userspace network, new argument parser <br> Extended testing & portability fixes | HD Audio |

*Hoping to see more contributors here*

## TODO
- Improve HID/Framebuffer API, more GUI backends
- Userspace networking, perhaps a better NIC
- Sparse HDD image format, compression/deduplication
- Sound (HD Audio or else)
- USB HID
- Maybe virtio devices
- Other peripherals
- *A lot more...*
- Userspace emulation? KVM hypervisor?
