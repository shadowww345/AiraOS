# AiraOS
This is my first attempt at creating an operating system. It is a minimal OS project and is not intended for actual use.

I would like to thank everyone who has shown interest in the project. Since I need to focus on my own AI framework, and due to the lack of resources for certain functions as well as my limited time, I couldn't find enough free time for this project lately. I will continue at full speed during the summer. Also, I would be happy if you contribute to the development.

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
qemu-system-i386 -cdrom aira.iso \
  -device sb16,audiodev=snd0 \
  -machine pcspk-audiodev=snd0 \
  -audiodev pa,id=snd0,out.mixing-engine=off,timer-period=1000

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
## Aira Proggraming Language Tutorial
```
print("hello world")
```
prints string or variable
```
set varstr = "hello"
set varint = 10
```
variable definition
```
loop 10
  print("hello world")
endloop
```
```
beep(440)
beep(440,10)
```
beeps freq or freq and duration
```
color a
color b
color c
color 7
```
changes color a/b/c/7
```
sleep(50)
```
cpu sleeps miliseconds
```
add 1,1
sub 1,1
mul 1,1
div 10,0
```
mathematichs

## Example
```
set freq = 10
color a
loop 100
  beep(freq,50)
  add freq,10
  print("freq is:")
  print(freq)
  sleep(30)
endloop
```

## If you like the project, give it a ⭐!

#### The project is mainly for learning and experimentation.
