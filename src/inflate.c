#include <assert.h>

#include "yaz0.h"
#include "yaz0_util.h"

enum inf_status {
	INF_READ_HEADER,
	INF_READ_GROUP,
	INF_INFLATE_GROUP,
	INF_FLUSH,
};

struct inf_state { 
	struct yaz0_stream* stream;
	enum inf_status status;
	
	yaz0_byte read_buf[32];
	size_t read_sz;

	yaz0_byte write_buf[0x2000];
	size_t write_sz;
};

static enum yaz0_result read_header() {
	return YAZ0_OK;
}

static enum yaz0_result read_group() {
	return YAZ0_OK;
}

static enum yaz0_result inflate_group() {
	return YAZ0_OK;
}

static enum yaz0_result flush() {
	return YAZ0_OK;
}

enum yaz0_result yaz0_inflate_init(struct yaz0_stream* stream) {
	assert(stream != NULL);
	if (stream->alloc == NULL) {
		stream->alloc = yaz0_alloc;
		stream->free = yaz0_free;
	}
	struct inf_state* state = YAZ0_ALLOC(sizeof(struct inf_state), stream);
	if (state == NULL) {
		return YAZ0_ERR_OUT_OF_MEMORY;
	}
	state->stream = stream;
	state->status = INF_READ_HEADER;
	state->read_sz = 0;
	state->write_sz = 0;
	return YAZ0_OK;
}

void yaz0_inflate_end(struct yaz0_stream* stream) {
	assert(stream != NULL);
	YAZ0_FREE(stream->internal, stream);
	stream->internal = NULL;
}

enum yaz0_result yaz0_inflate(struct yaz0_stream* stream) {
	struct inf_state* state = stream->internal;
	switch (state->status) {
		case INF_READ_HEADER: return read_header();
		case INF_READ_GROUP: return read_group();
		case INF_INFLATE_GROUP: return inflate_group();
		case INF_FLUSH: return flush();
	}
	return YAZ0_STREAM_END;
}

