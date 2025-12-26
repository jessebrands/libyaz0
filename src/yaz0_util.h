#ifndef yaz0_util_h_
#define yaz0_util_h_

#define YAZ0_ALLOC(sz, strm) strm->alloc(sz, strm->opaque)
#define YAZ0_FREE(ptr, strm) strm->free(ptr, strm->opaque)

#endif // defined yaz0_util_h_
