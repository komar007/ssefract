all:
	nasm -f elf test.asm
	ld -s -shared -soname test.so -o test.so test.o -melf_i386
	gcc -std=c99 burningship.c -m32 -lm -DC -o burn_c
	gcc -std=c99 burningship.c test.so -m32 -lm -o burn_asm
