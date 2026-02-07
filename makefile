AS = nasm
CC = gcc
LD = ld
ASFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-pie -O0 -Ikernel
LDFLAGS = -m elf_i386 -T linker.ld

ISO_DIR = isodir
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub
FLOPPY_IMG = floppy.img

SRCS = $(wildcard kernel/*.c)
OBJS = $(SRCS:.c=.o)
BOOT_OBJ = boot/bootloader.o

all: aira.iso $(FLOPPY_IMG)

$(FLOPPY_IMG):
	dd if=/dev/zero of=$(FLOPPY_IMG) bs=1024 count=30000
	mkfs.fat -F 12 $(FLOPPY_IMG)
	mcopy -i $(FLOPPY_IMG) test.txt ::TEST.TXT
	mcopy -i $(FLOPPY_IMG) startup.wav ::STARTUP.WAV
	dd if=startup.wav of=$(FLOPPY_IMG) bs=512 seek=500 conv=notrunc

kernel.bin: $(BOOT_OBJ) $(OBJS)
	$(LD) $(LDFLAGS) $(BOOT_OBJ) $(OBJS) -o kernel.bin

$(BOOT_OBJ): boot/bootloader.s
	$(AS) $(ASFLAGS) $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

aira.iso: kernel.bin
	mkdir -p $(GRUB_DIR)
	cp kernel.bin $(BOOT_DIR)/kernel.bin
	@echo 'set timeout=0' > $(GRUB_DIR)/grub.cfg
	@echo 'set default=0' >> $(GRUB_DIR)/grub.cfg
	@echo 'insmod vbe' >> $(GRUB_DIR)/grub.cfg
	@echo 'insmod vga' >> $(GRUB_DIR)/grub.cfg
	@echo 'set gfxmode=1024x768x32' >> $(GRUB_DIR)/grub.cfg
	@echo 'set gfxpayload=keep' >> $(GRUB_DIR)/grub.cfg
	@echo 'menuentry "AiraOS" {' >> $(GRUB_DIR)/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> $(GRUB_DIR)/grub.cfg
	@echo '    boot' >> $(GRUB_DIR)/grub.cfg
	@echo '}' >> $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o aira.iso $(ISO_DIR)

clean:
	rm -rf kernel/*.o boot/*.o kernel.bin aira.iso $(ISO_DIR) $(FLOPPY_IMG)

run: aira.iso $(FLOPPY_IMG)
	qemu-system-i386 -boot d \
	-cdrom aira.iso \
	-drive file=$(FLOPPY_IMG),format=raw,if=ide,bus=0,unit=0,media=disk \
	-device sb16,audiodev=snd0 \
	-machine pcspk-audiodev=snd0 \
	-audiodev pa,id=snd0,out.mixing-engine=off,timer-period=1000

.PHONY: all clean run
