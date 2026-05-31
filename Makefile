all: kernel8.img 

kernel8.img: kernel8.elf
	aarch64-none-elf-objcopy -O binary kernel8.elf kernel8.img

kernel8.elf: start.o main.o mbox.o el_switch.o
	aarch64-none-elf-ld -T link.ld -nostdlib -o kernel8.elf start.o main.o mbox.o el_switch.o

start.o: start.S
	aarch64-none-elf-gcc -c start.S -o start.o

main.o: main.c 
	aarch64-none-elf-gcc -c main.c -o main.o -ffreestanding -nostdlib -nostartfiles -mcmodel=small

mbox.o: mbox.c
	aarch64-none-elf-gcc -c mbox.c -o mbox.o -ffreestanding -nostdlib -nostartfiles -mcmodel=small

el_switch.o: el_switch.S
	aarch64-none-elf-gcc -c el_switch.S -o el_switch.o

clean:
	rm -f *.o *.elf *.img