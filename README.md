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

sudo apt install grub-efi-amd64-bin grub-common

## Build

**Note**:If you are using Windows, install WSL to build.

**Type in the terminal**:

sudo apt update
sudo apt install qemu qemu-system-x86

make

#### The project is mainly for learning and experimentation.
