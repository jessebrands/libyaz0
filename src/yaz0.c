#include <stdlib.h>

#include "yaz0.h"

void* yaz0_alloc(size_t const sz, void* opaque) {
	return malloc(sz);
}

void yaz0_free(void* ptr, void* opaque) {
	free(ptr);
}


