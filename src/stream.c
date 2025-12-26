#include <string.h>

#include "stream.h"

__attribute__((visibility("hidden")))
size_t sread(yaz0_byte* dst, size_t const count, struct yaz0_stream* stream) {
	size_t bytes_in = count;
	if (bytes_in > stream->avail_in) {
		bytes_in = stream->avail_in;
	}
	memcpy(dst, stream->in, count);
	stream->in += bytes_in;
	stream->avail_in -= bytes_in;
	stream->total_in += bytes_in;
	return bytes_in;
}

