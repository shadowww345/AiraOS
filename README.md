# AiraOS
This is my first attempt at creating an operating system. It is a minimal OS project and is not intended for actual use.

## Features

Minimal kernel with a terminal prompt (>)

Simple nano-like editor

Has a mini language (I’m still developing it)

Sound Device

Screen color changes

**Notes**

This is just a hobby project / OS experiment.

UEFI-compatible GRUB is needed to boot the ISO:



## Build

**Note**:If you are using Windows, install WSL to build.

**Type in the terminal**:
```
sudo apt update
```

```bash
sudo apt install grub-efi-amd64-bin grub-common
```

```bash
sudo apt install qemu qemu-system-x86
```

```bash
make
```

```bash
qemu-system-i386 -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -cdrom aira.iso
```

## Commands
```aira
nano
```
Opens nano editor

```aira
compile
``` 
It interprets the code written in the nano editor.

```aira
clear
```
Clears all screen
```aira
color a
```
Sets VGA text mode current color to 0x0A (green)
```aira
color b
```
Sets VGA text mode current color to 0x0B (blue)

```aira
color c
```
Sets VGA text mode current color to 0x0C (red)

```aira
color 7
```
Sets VGA text mode current color to 0x07 (gray)
```aira
beep
```
Enables the audio device and produces a 1200 Hz beep.


#### The project is mainly for learning and experimentation.
