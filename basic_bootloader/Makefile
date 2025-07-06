all: bootloader.bin

bootloader.bin: bootloader.asm
        nasm -f bin bootloader.asm -o bootloader.bin

run: bootloader.bin
        qemu-system-i386 -fda bootloader.bin

clean:
        rm -rf bootloader.bi
