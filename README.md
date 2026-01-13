# AiraOS
This is my first attempt at creating an operating system. It is a minimal OS project and is not intended for actual use.

## Features

Minimal kernel with a terminal prompt (>)

Simple nano-like editor

Has a mini language (.aira)

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
qemu-system-i386 -device sb16 -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0 -cdrom aira.iso
```

## Commands
```acmd
nano
```
Opens nano editor

```acmd
compile filename.aira
``` 
It interprets the code written in the nano editor.

```acmd
clear
```
Clears all screen
```acmd
color a
```
Sets VGA text mode current color to 0x0A (green)
```acmd
color b
```
Sets VGA text mode current color to 0x0B (blue)

```acmd
color c
```
Sets VGA text mode current color to 0x0C (red)

```acmd
color 7
```
Sets VGA text mode current color to 0x07 (gray)
```acmd
beep
```
Enables the audio device and produces a 1200 Hz beep.

## If you like the project, give it a ⭐!

### Lua compiling support coming soon...
##### Lua support is in early development

#### The project is mainly for learning and experimentation.
