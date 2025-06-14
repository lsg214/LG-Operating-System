# Makefile for building and running our OS
#Assembler and emulator
ASM = nasm
EMU = qemu-system-i386

#Build the bootloader
boot.bin: boot.ASM
	$(ASM) -f bin boot.ASM -o boot.bin

#Run in emulator
#make boot.bin/run to run the bootloader message
run: boot.bin
	$(EMU) -drive format=raw,file=boot.bin,if=floppy -boot a

#Clean build files
clean:
	rm -f *.bin

.PHONY : run clean
# This is a phony target, meaning it doesn't correspond to a file
# and should always be executed when called.
# The .PHONY directive tells make that 'run' and 'clean' are not files
# but rather commands to be executed.
# This prevents make from getting confused if a file named 'run' or 'clean' exists.


#Additional Details about thr Makefile:
# The 'clean' target is used to remove any generated files, ensuring a fresh build next time.
# The 'run' target builds the bootloader and runs it in the emulator.
# The 'boot.bin' target compiles the bootloader assembly code into a binary file.
# The 'ASM' variable specifies the assembler to use, and 'EMU' specifies the emulator.
# The 'boot.ASM' file is the source code for the bootloader.
# The '-f bin' option tells nasm to output a flat binary file, and '-o boot.bin' specifies the output file name.
# The 'run' target depends on 'boot.bin', meaning it will only run if 'boot.bin' is successfully built.
# The 'clean' target removes all generated binary files, allowing for a clean slate for the next build.
# The Makefile is designed to automate the build and run process for the OS bootloader,
# making it easier to develop and test the bootloader code without manual intervention.
# The Makefile uses the 'nasm' assembler to compile the assembly code into a binary format
# and the 'qemu-system-i386' emulator to run the resulting binary.
# The 'boot.bin' file is the output of the assembly compilation,
# and it is specified as the input for the emulator.
