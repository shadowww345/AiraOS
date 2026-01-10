
AS = nasm
CC = gcc
LD = ld
ASFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-pie -O0
LDFLAGS = -m elf_i386 -T linker.ld


ISO_DIR = isodir
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub


all: aira.iso

kernel.bin: boot.o kernel.o
	$(LD) $(LDFLAGS) boot.o kernel.o -o kernel.bin

boot.o: boot/bootloader.s
	$(AS) $(ASFLAGS) boot/bootloader.s -o boot.o

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o

aira.iso: kernel.bin
	mkdir -p $(GRUB_DIR)
	cp kernel.bin $(BOOT_DIR)/kernel.bin
	@echo 'set timeout=5' > $(GRUB_DIR)/grub.cfg
	@echo 'set default=0' >> $(GRUB_DIR)/grub.cfg
	@echo 'menuentry "Aira" {' >> $(GRUB_DIR)/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> $(GRUB_DIR)/grub.cfg
	@echo '    boot' >> $(GRUB_DIR)/grub.cfg
	@echo '}' >> $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o aira.iso $(ISO_DIR)


clean:
	rm -rf *.o kernel.bin aira.iso $(ISO_DIR)

run: aira.iso
	qemu-system-i386 -cdrom aira.iso
