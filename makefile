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
HDD32_IMG = hdd32.img

SRCS = $(wildcard kernel/*.c)
OBJS = $(SRCS:.c=.o)
BOOT_OBJ = boot/bootloader.o
ISR_OBJ= kernel/isr_stubs.o
TSK_OBJ= kernel/task_switch.o

all: aira.iso $(FLOPPY_IMG) $(HDD32_IMG)
#ffmpeg -i input.mp3 -acodec pcm_u8 -ac 1 -ar 44100 -f wav output.wav
$(FLOPPY_IMG): test.txt startup.wav
	rm -f $(FLOPPY_IMG)
	dd if=/dev/zero of=$(FLOPPY_IMG) bs=1024 count=30000
	mkfs.fat -F 12 $(FLOPPY_IMG)
	mcopy -i $(FLOPPY_IMG) test.txt ::TEST.TXT
	mcopy -i $(FLOPPY_IMG) startup.wav ::STARTUP.WAV


$(HDD32_IMG):
	rm -f $(HDD32_IMG)
	dd if=/dev/zero of=$(HDD32_IMG) bs=1M count=1024
	mkfs.fat -F 32 $(HDD32_IMG)
	mcopy -i $(HDD32_IMG) test.txt ::test.txt
	mcopy -i $(HDD32_IMG) hi.bin ::hi.bin
	mcopy -i $(HDD32_IMG) kv2.wav ::kv2.wav
	mcopy -i $(HDD32_IMG) snowy.wav ::snowy.wav
	mcopy -i $(HDD32_IMG) snowy.bmp ::snowy.bmp
	mcopy -i $(HDD32_IMG) yumul.wav ::yumul.wav
	mcopy -i $(HDD32_IMG) gencosman.wav ::gencosman.wav
	mcopy -i $(HDD32_IMG) startup.wav ::startup.wav
	mmd -i $(HDD32_IMG) ::/testdir

kernel.bin: $(BOOT_OBJ) $(OBJS) $(ISR_OBJ) $(TSK_OBJ)
	$(LD) $(LDFLAGS) $(BOOT_OBJ) $(OBJS) $(ISR_OBJ) $(TSK_OBJ) -o kernel.bin

$(BOOT_OBJ): boot/bootloader.s
	$(AS) $(ASFLAGS) $< -o $@

$(ISR_OBJ): kernel/isr_stubs.s
	$(AS) $(ASFLAGS) $< -o $@

$(TSK_OBJ): kernel/task_switch.s
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
	rm -rf kernel/*.o boot/*.o kernel.bin aira.iso $(ISO_DIR) $(FLOPPY_IMG) $(HDD32_IMG)

run: aira.iso $(HDD32_IMG)
	qemu-system-i386 -boot d \
	-cdrom aira.iso \
	-drive file=hdd32.img,format=raw,if=ide,bus=0,unit=0,media=disk \
	-device sb16,audiodev=snd0,irq=5,dma=1 \
	-machine pcspk-audiodev=snd0 \
	-audiodev pa,id=snd0,out.mixing-engine=off,timer-period=1000

.PHONY: all clean run
