# Makefile for building the kernel
# This builds a minimal C kernel that can be loaded by a bootloader

# Detect OS
UNAME_S := $(shell uname -s)

# Compiler and tools
ifeq ($(UNAME_S),Darwin)
    # macOS - use i386-elf cross compiler if available, otherwise regular gcc
    CC = i386-elf-gcc
    LD = i386-elf-ld
    OBJCOPY = i386-elf-objcopy
    # If cross compiler not available, fall back to regular tools
    ifeq ($(shell which i386-elf-gcc 2>/dev/null),)
        CC = gcc
        LD = ld
        # Use otool and dd instead of objcopy on macOS
        OBJCOPY = 
        LDFLAGS = -arch i386 -static -e _kernel_main -segaddr __TEXT 0x100000 -pagezero_size 0x0 -macos_version_min 10.6
        MACOS_NATIVE = 1
    else
        LDFLAGS = -m elf_i386 -T linker.ld
        MACOS_NATIVE = 0
    endif
else
    # Linux/other Unix
    CC = gcc
    LD = ld
    OBJCOPY = objcopy
    LDFLAGS = -m elf_i386 -T linker.ld
    MACOS_NATIVE = 0
endif

AS = nasm

# Compiler flags for kernel development
CFLAGS = -m32 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -Wall -Wextra -Werror -c -O2

# Assembly flags
ASFLAGS = -f elf32
BOOTLOADER_ASFLAGS = -f bin

# Source files
KERNEL_SOURCES = kernel.c
KERNEL_OBJECTS = kernel.o
BOOTLOADER_SOURCES = boot.asm
BOOTLOADER_OBJECTS = boot.o

# Output files
KERNEL_BIN = kernel.bin
BOOTLOADER_BIN = bootloader.bin
OS_IMAGE = myos.img

# Default target
all: check-tools $(OS_IMAGE)

# Check for required tools
check-tools:
	@echo "Checking for required tools..."
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found" && exit 1)
	@which $(LD) > /dev/null || (echo "Error: $(LD) not found" && exit 1)
	@which $(AS) > /dev/null || (echo "Error: $(AS) not found" && exit 1)
ifeq ($(MACOS_NATIVE),0)
	@which $(OBJCOPY) > /dev/null || (echo "Error: $(OBJCOPY) not found" && exit 1)
endif
	@echo "All tools found!"

# Build the OS image
$(OS_IMAGE): $(KERNEL_BIN) $(BOOTLOADER_BIN)
	@echo "Creating OS image..."
	# Create a 1.44MB floppy disk image
	dd if=/dev/zero of=$(OS_IMAGE) bs=1024 count=1440 2>/dev/null
	# Copy bootloader to first sector
	dd if=$(BOOTLOADER_BIN) of=$(OS_IMAGE) bs=512 count=1 conv=notrunc 2>/dev/null
	# Copy kernel starting at second sector
	dd if=$(KERNEL_BIN) of=$(OS_IMAGE) bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "OS image created: $(OS_IMAGE)"

# Build the kernel binary
$(KERNEL_BIN): $(KERNEL_OBJECTS)
	@echo "Linking kernel..."
	@echo "Using LD: $(LD)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "Objects: $(KERNEL_OBJECTS)"
	@ls -la $(KERNEL_OBJECTS)
ifeq ($(UNAME_S),Darwin)
    ifeq ($(shell which i386-elf-gcc 2>/dev/null),)
        # macOS without cross-compiler - use native tools
	$(LD) $(LDFLAGS) -o kernel.elf $(KERNEL_OBJECTS)
	@echo "Extracting binary from Mach-O executable..."
	# Extract the TEXT segment from the Mach-O file
	otool -l kernel.elf | grep -A 5 "segname __TEXT" | grep "fileoff" | awk '{print $$2}' > .text_offset
	otool -l kernel.elf | grep -A 5 "segname __TEXT" | grep "filesize" | awk '{print $$2}' > .text_size
	dd if=kernel.elf of=$(KERNEL_BIN) bs=1 skip=$$(cat .text_offset) count=$$(cat .text_size) 2>/dev/null
	rm -f .text_offset .text_size
    else
        # macOS with cross-compiler
	$(LD) $(LDFLAGS) -o kernel.elf $(KERNEL_OBJECTS)
	$(OBJCOPY) -O binary kernel.elf $(KERNEL_BIN)
    endif
else
    # Linux
	$(LD) $(LDFLAGS) -o kernel.elf $(KERNEL_OBJECTS)
	$(OBJCOPY) -O binary kernel.elf $(KERNEL_BIN)
endif

# Compile kernel C source
kernel.o: kernel.c
	@echo "Compiling kernel..."
	$(CC) $(CFLAGS) kernel.c -o kernel.o

# Assemble bootloader
$(BOOTLOADER_BIN): boot.asm
	@echo "Assembling bootloader..."
	$(AS) $(BOOTLOADER_ASFLAGS) boot.asm -o $(BOOTLOADER_BIN)

boot.o: boot.asm
	$(AS) $(ASFLAGS) boot.asm -o boot.o

# Clean build artifacts
clean:
	rm -f *.o *.bin *.elf *.img .text_offset .text_size .boot_offset .boot_size

# Run in QEMU (requires QEMU to be installed)
run: myos.img
	qemu-system-x86_64 -drive file=myos.img,format=raw -m 128M

# Run in QEMU with debugging
debug: $(OS_IMAGE)
	qemu-system-i386 -fda $(OS_IMAGE) -s -S

# Show kernel information
info: $(KERNEL_BIN)
	@echo "Kernel binary information:"
	@ls -la $(KERNEL_BIN)
	@echo "Kernel size: $$(stat -f%z $(KERNEL_BIN)) bytes"
	@echo "Kernel hex dump (first 256 bytes):"
	@hexdump -C $(KERNEL_BIN) | head -16

# Disassemble kernel
disasm: kernel.elf
	objdump -d kernel.elf

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build complete OS image"
	@echo "  clean    - Remove build artifacts"
	@echo "  run      - Run OS in QEMU"
	@echo "  debug    - Run OS in QEMU with debugging"
	@echo "  info     - Show kernel information"
	@echo "  disasm   - Disassemble kernel"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Note: For best results on macOS, install cross-compilation tools:"
	@echo "  brew install i386-elf-gcc i386-elf-binutils"

.PHONY: all clean run debug info disasm help