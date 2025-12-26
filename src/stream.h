#ifndef yaz0_stream_h_
#define yaz0_stream_h_

#include "yaz0.h"

// Reads up to `count` bytes from the `stream`, advances the stream read cursor
// and copies the bytes to `dst`. Returns the amount of bytes read.
size_t sread(yaz0_byte* dst, size_t count, struct yaz0_stream* stream);

#endif // defined yaz0_stream_h_

