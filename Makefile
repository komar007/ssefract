CC=gcc -std=c99 -m32 -O2
LIBS=-lm -ldl
WARN=
SOURCES=gui.c

all: fractal_asm.so fractal_c.so browser render benchmark

dep:
	@echo -en > Makefile.dep
	@for s in $(SOURCES); do \
		avr-gcc -std=c99 -M $$s >> Makefile.dep; \
		echo -e '\t'@echo CC $$s >> Makefile.dep; \
		echo -e '\t'@$(CC) $(WARN) -c $$s >> Makefile.dep; \
	done

-include Makefile.dep

browser: gui.o io.o fractal_api.o threaded_generator.o
	@echo LINK $@
	$(CC) -o $@ $^ $(LIBS) -lSDL

render: render.o io.o fractal_api.o threaded_generator.o
	@echo LINK $0
	$(CC) -o $@ $^ $(LIBS) -lpthread

benchmark: benchmark.o fractal_api.o
	@echo LINK $0
	$(CC) -o $@ $^ $(LIBS)


fractal_asm.so: fractal.asm
	nasm -f elf fractal.asm
	ld -s -shared -soname fractal_asm.so -o fractal_asm.so fractal.o -melf_i386

fractal_c.so: fractal.c
	$(CC) -fPIC -shared fractal.c -m32 -o fractal_c.so

clean:
	rm -fr *.o *.so
