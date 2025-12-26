
#ifndef yaz0_h_
#define yaz0_h_

#include <stddef.h>
#include <stdint.h>

typedef unsigned char yaz0_byte;
typedef void* (yaz0_alloc_func)(size_t sz, void* opaque);
typedef void (yaz0_free_func)(void* ptr, void* opaque);

enum yaz0_result {
		YAZ0_OK,
		YAZ0_STREAM_END,
		YAZ0_ERR_OUT_OF_MEMORY,
		YAZ0_ERR_INVALID_DATA,
};

struct yaz0_stream {
		yaz0_byte const* in;
		size_t avail_in;
		size_t total_in;
		yaz0_byte* out;
		size_t avail_out;
		size_t total_out;
		yaz0_alloc_func* alloc;
		yaz0_free_func* free;
		void* opaque;
		void* internal;
};

// Initializes `stream` for decompressing yaz0 data.
enum yaz0_result yaz0_inflate_init(struct yaz0_stream* stream);

// Cleans up all resources allocated by `stream`.
void yaz0_inflate_end(struct yaz0_stream* stream);

// Takes as much data from `stream` as possible and then writes as much
// decompressed data as possible back to `stream`.
enum yaz0_result yaz0_inflate(struct yaz0_stream* stream);

// Allocates at least `sz` bytes from the heap using the standard allocator
// from the C standard library.
void* yaz0_alloc(size_t sz, void* opaque);

// Frees the allocated memory located at `ptr` from the heap.
void yaz0_free(void* ptr, void* opaque);

#endif // defined yaz0_h_

