all:
	nasm -f elf fractal.asm
	ld -s -shared -soname fractal_asm.so -o fractal_asm.so fractal.o -melf_i386
	gcc -shared -fPIC -std=c99 fractal.c -m32 -lm -o fractal_c.so
	gcc -std=c99 benchmark.c -m32 -lm -ldl -o burn_benchmark
	gcc -std=c99 gui/sdl/gui.c -m32 -lm -ldl -o gui_sdl -lSDL
