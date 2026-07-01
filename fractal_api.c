#define _GNU_SOURCE
#include "fractal_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>

void* load_lib_or_die(const char *name)
{
	void *lib = dlopen(name, RTLD_LAZY);
	if (!lib) {
		printf("dlopen error: %s\n", dlerror());
		exit(-1);
	}
	return lib;
}

generator_fun_t load_impl_or_die(void *lib, const char symname[])
{
	generator_fun_t fun = dlsym(lib, symname);
	if (!fun) {
		printf("dlsym error: %s\n", dlerror());
		exit(-1);
	}
	return fun;
}

void close_lib_or_die(void *lib)
{
	if (dlclose(lib) != 0) {
		printf("dlclose error: %s\n", dlerror());
		exit(-1);
	}
}

/* ------------------------------------------------------------------------- */

void *libs[COUNT_IMPL] = {0};

/* Return the directory containing the current executable.
   Resolved once via /proc/self/exe, cached thereafter. */
const char *get_exe_dir(void)
{
	static char dir[PATH_MAX];
	static int done = 0;
	if (!done) {
		char exe_path[PATH_MAX];
		ssize_t len = readlink("/proc/self/exe", exe_path,
				      sizeof(exe_path) - 1);
		if (len == -1) {
			perror("readlink /proc/self/exe");
			exit(-1);
		}
		exe_path[len] = '\0';
		/* dirname may modify its argument and return static storage;
		   copy the result immediately. */
		strncpy(dir, dirname(exe_path), sizeof(dir) - 1);
		dir[sizeof(dir) - 1] = '\0';
		done = 1;
	}
	return dir;
}

generator_fun_t get_impl_or_die(enum implementation impl)
{
	const char *libname;
	switch (impl) {
	case IMPL_C:
		libname = BASENAME "_c.so";
		break;
	case IMPL_ASM:
		libname = BASENAME "_asm.so";
		break;
	default:
		printf("get_impl_or_die: wrong implementation chosen\n");
		exit(-1);
	}
	if (!libs[impl]) {
		/* .so files live next to the executable */
		char full_libname[PATH_MAX];
		snprintf(full_libname, sizeof(full_libname), "%s/%s",
			 get_exe_dir(), libname);
		libs[impl] = load_lib_or_die(full_libname);
	}
	return load_impl_or_die(libs[impl], SYMNAME);
}

void close_libs_or_die(void)
{
	for (int i = 0; i < COUNT_IMPL; ++i) {
		if (libs[i]) {
			close_lib_or_die(libs[i]);
			libs[i] = NULL;
		}
	}
}
