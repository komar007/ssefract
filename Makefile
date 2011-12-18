all:
	nasm -f elf fractal.asm
	ld -s -shared -soname fractal.so -o fractal.so fractal.o -melf_i386
	gcc -std=c99 burningship.c -m32 -lm -ldl -DLANG_C -o burn_c
	gcc -std=c99 burningship.c -m32 -lm -ldl -o burn_asm
