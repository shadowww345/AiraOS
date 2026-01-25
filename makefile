AS = nasm
CC = gcc
LD = ld
ASFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-pie -O0 -Ikernel
LDFLAGS = -m elf_i386 -T linker.ld

ISO_DIR = isodir
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub

SRCS = $(wildcard kernel/*.c)
OBJS = $(SRCS:.c=.o)
BOOT_OBJ = boot/bootloader.o

all: aira.iso

kernel.bin: $(BOOT_OBJ) $(OBJS)
	$(LD) $(LDFLAGS) $(BOOT_OBJ) $(OBJS) -o kernel.bin

$(BOOT_OBJ): boot/bootloader.s
	$(AS) $(ASFLAGS) $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

aira.iso: kernel.bin
	mkdir -p $(GRUB_DIR)
	cp kernel.bin $(BOOT_DIR)/kernel.bin
	@echo 'set timeout=5' > $(GRUB_DIR)/grub.cfg
	@echo 'set default=0' >> $(GRUB_DIR)/grub.cfg
	@echo 'insmod vbe' >> $(GRUB_DIR)/grub.cfg
	@echo 'insmod vga' >> $(GRUB_DIR)/grub.cfg
	@echo 'set gfxmode=800x600x32' >> $(GRUB_DIR)/grub.cfg
	@echo 'set gfxpayload=keep' >> $(GRUB_DIR)/grub.cfg
	@echo 'menuentry "Aira" {' >> $(GRUB_DIR)/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> $(GRUB_DIR)/grub.cfg
	@echo '    boot' >> $(GRUB_DIR)/grub.cfg
	@echo '}' >> $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o aira.iso $(ISO_DIR)

clean:
	rm -rf kernel/*.o boot/*.o kernel.bin aira.iso $(ISO_DIR)

run: aira.iso
	qemu-system-i386 -cdrom aira.iso \
  -device sb16,audiodev=snd0 \
  -machine pcspk-audiodev=snd0 \
  -audiodev pa,id=snd0,out.mixing-engine=off,timer-period=1000


.PHONY: all clean run
