/*
 * yaz0.h: yaz0 compression library
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of libyaz0.
 *
 * libyaz0 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libyaz0 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libyaz0. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LIBYAZ0_YAZ0_H
#define LIBYAZ0_YAZ0_H

#include <stddef.h>
#include <stdint.h>

#include "yaz0/config.h"

#if defined _WIN32 || defined __CYGWIN__
#  if defined YAZ0_STATIC
#    define YAZ0_API
#  elif defined YAZ0_SHARED
#    define YAZ0_API __declspec(dllexport)
#  else
#    define YAZ0_API __declspec(dllimport)
#  endif
#elif defined __GNUC__ && !defined YAZ0_STATIC
#  define YAZ0_API __attribute__((visibility("default")))
#else
#  define YAZ0_API
#endif

#if defined __cplusplus
extern "C" {

#endif

/*!
 * Function that allocates memory.
 * \see yaz0_stream
 */
typedef void* (* yaz0_alloc_func)(void* opaque, size_t size);

/*!
 * Function that releases memory allocated by \ref yaz0_alloc_func.
 * \see yaz0_stream
 */
typedef void (* yaz0_free_func)(void* opaque, void* ptr);

/*!
 * Return codes returned by the library.
 */
enum yaz0_result {
    //! The operation succeeded successfully.
    YAZ0_OK = 0,

    //! The stream has finished (de)compressing all data.
    YAZ0_STREAM_END = 1,

    //! Unused return code that can be used by applications.
    YAZ0_ERRNO = -1,

    //! The stream is in an invalid state.
    YAZ0_STREAM_ERROR = -2,

    //! The input data is malformed or invalid.
    YAZ0_DATA_ERROR = -3,

    //! An allocation failed.
    YAZ0_MEMORY_ERROR = -4,

    //! The (de)compressor has stalled because no bytes can be read or written.
    YAZ0_BUFFER_ERROR = -5,

    //! The input stream came to an unexpected end.
    YAZ0_TRUNCATED = -6,

    //! The header in the input data is malformed.
    YAZ0_BAD_HEADER = -7,

    //! The stream has ended without errors, but has not written or read the
    //! expected amount of bytes.
    YAZ0_SIZE_MISMATCH = -8,
};

/*!
 * Controls what the stream does when the input runs out.
 */
enum yaz0_flush {
    /*!
     * More input may follow: the call suspends, returns YAZ0_OK, and the
     * caller supplies more and calls again.
     */
    YAZ0_NO_FLUSH = 0,

    /*!
     * Tells the (de)compressor that no further data is coming beyond what is
     * in the buffer now.
     */
    YAZ0_FINISH = 1,
};

/*!
 * Controls how much effort the compressor makes to compress data.
 */
enum yaz0_level {
    //! Picks the library default compression level.
    YAZ0_DEFAULT_COMPRESSION = -1,

    //! Performs no compression at all, encodes literals only. Output will be
    //! 12.5% larger than the input using this.
    YAZ0_NO_COMPRESSION = 0,

    //! The best compression, the slowest speed.
    YAZ0_BEST_COMPRESSION = 9,
};

/*!
 * \brief Options that can be passed to the compressor.
 * \see yaz0_compress_init_with_options
 */
struct yaz0_compress_options {
    //! Compression level.
    int level;

    //! Value for the alignment hint, copied into header.
    uint32_t alignment;

    //! Value for the reserved bytes, copied into the header.
    uint8_t reserved[4];
};

/*!
 * Structure describing a Yaz0 header.
 */
struct yaz0_header {
    //! Always Yaz0, output only.
    char magic[4];

    //! The size in bytes of the data when decompressed.
    uint32_t uncompressed_size;

    //! Alignment hint, ignored by libyaz0.
    uint32_t alignment;

    //! Unused, reserved by Nintendo.
    uint8_t reserved[4];
};

/*!
 * \brief State for a single compression or decompression.
 *
 * The caller must zero-initialize the stream before use:
 *
 *     struct yaz0_stream stream = {0};
 *
 * Set next_in, avail_in, next_out and avail_out around each call. The library
 * advances all four as it consumes and produces, so a call that suspends with
 * YAZ0_OK is resumed by refilling whichever buffer ran out and calling again.
 *
 * \note To route allocation through your own functions, set both alloc and
 *       free, plus opaque if your allocator needs it. Leave all three zero
 *       and the library uses a default.
 * \see yaz0_compress_init, yaz0_decompress_init
 */
struct yaz0_stream {
    //! Next input byte to read.
    uint8_t const* next_in;

    //! Bytes available for reading at next_in.
    size_t avail_in;

    //! Total bytes consumed from the input across every call on this stream.
    size_t total_in;

    //! Next position to write output to.
    uint8_t* next_out;

    //! Space remaining for writing at next_out.
    size_t avail_out;

    //! Total bytes written to the output across every call on this stream.
    //! After YAZ0_STREAM_END this is the exact size of the result.
    size_t total_out;

    //! Passed unchanged as the first argument to alloc and free.
    void* opaque;

    //! Allocation function, or NULL to use a default allocator.
    yaz0_alloc_func alloc;

    //! Deallocation function, or NULL to use a default allocator.
    yaz0_free_func free;

    //! Private state library state.
    void* state;
};

/*!
 * \brief Returns default options for the compressor.
 * \return Default compression options.
 */
YAZ0_API struct yaz0_compress_options
yaz0_default_compress_options(void);

/*!
 * \brief Initializes the stream for compression.
 * \param stream Pointer to the stream object to initialize.
 * \param level Compression level.
 * \param uncompressed_size Size in bytes of the uncompressed data.
 * \return YAZ0_OK on success.
 * \note The caller must call yaz0_compress_end when done.
 * \see yaz0_level
 */
YAZ0_API enum yaz0_result
yaz0_compress_init(struct yaz0_stream* stream, int level,
                   uint32_t uncompressed_size);

/*!
 * \brief Initializes the stream for compression.
 * \param stream Pointer to the stream object to initialize.
 * \param uncompressed_size Size in bytes of the uncompressed data.
 * \param options Options to be passed to the compressor.
 * \return YAZ0_OK on success.
 * \note The caller must call yaz0_compress_end when done.
 * \see yaz0_default_compress_options
 */
YAZ0_API enum yaz0_result
yaz0_compress_init_with_options(struct yaz0_stream* stream, uint32_t uncompressed_size,
                                struct yaz0_compress_options options);

/*!
 * \brief Compresses input data into the output buffer.
 * \param stream Pointer to a compression stream.
 * \param flush Flush mode.
 * \return YAZ0_OK if the operation was successful.
 *         YAZ0_STREAM_END when all data has been compressed.
 * \note The caller must call yaz0_compress_end when compression is done or
 *       the stream has failed.
 * \see yaz0_flush
 */
YAZ0_API enum yaz0_result
yaz0_compress(struct yaz0_stream* stream, enum yaz0_flush flush);

/*!
 * \brief Releases all memory associated with a compression stream.
 * \param stream Pointer to a compression stream.
 */
YAZ0_API void
yaz0_compress_end(struct yaz0_stream* stream);

/*!
 * \brief Resets the state of the compressor to the initial state.
 * \param stream Pointer to the stream object to initialize.
 * \param uncompressed_size Size in bytes of the uncompressed data.
 * \param options Options to be passed to the compressor.
 * \return YAZ0_OK on success.
 * \note The caller must call yaz0_compress_end when done.
 * \see yaz0_default_compress_options
 */
YAZ0_API enum yaz0_result
yaz0_compress_reset(struct yaz0_stream* stream, uint32_t uncompressed_size,
                    struct yaz0_compress_options options);

/*!
 * \brief Returns the worst-case compression scenario size.
 * \param uncompressed_size Uncompressed size in bytes.
 * \return Compressed size in the worst-case scenario.
 * \note This value is 112.5% of the uncompressed size.
 */
YAZ0_API size_t
yaz0_compress_bound(uint32_t uncompressed_size);

/*!
 * \brief Initializes a stream for decompression.
 * \param stream Pointer to the stream object to initialize.
 * \return YAZ0_OK on success.
 * \note Callers must call yaz0_decompress_end when done.
 */
YAZ0_API enum yaz0_result
yaz0_decompress_init(struct yaz0_stream* stream);

/*!
 * \brief Decompresses the input buffer into the output buffer.
 * \param stream Pointer to a decompression stream.
 * \param flush Flush mode.
 * \return YAZ0_OK if the operation was successful.
 * \see yaz0_flush
 */
YAZ0_API enum yaz0_result
yaz0_decompress(struct yaz0_stream* stream, enum yaz0_flush flush);

/*!
 * \brief Releases all memory associated with a decompression stream.
 * \param stream Pointer to a decompression stream.
 */
YAZ0_API void
yaz0_decompress_end(struct yaz0_stream* stream);

/*!
 * \brief Resets the state of the decompressor to the initial state.
 * \param stream Pointer to the stream object to initialize.
 * \return YAZ0_OK on success.
 * \note Callers must call yaz0_decompress_end when done.
 */
YAZ0_API enum yaz0_result
yaz0_decompress_reset(struct yaz0_stream* stream);

/*!
 * \brief Decodes a Yaz0 header from a buffer.
 * \param data Buffer to read.
 * \param size Size of the buffer in bytes.
 * \param header Pointer to a \ref yaz0_header that will receive the data.
 * \return YAZ0_OK if successful.
 */
YAZ0_API enum yaz0_result
yaz0_read_header(uint8_t const* data, size_t size, struct yaz0_header* header);

/*!
 * \brief Encodes a Yaz0 header into a buffer.
 * \param header Pointer to a \ref yaz0_header.
 * \param dst Buffer to write to.
 * \param size Size of the buffer in bytes.
 * \return YAZ0_OK if successful.
 */
YAZ0_API enum yaz0_result
yaz0_write_header(struct yaz0_header const* header, uint8_t* dst, size_t size);

/*!
 * \brief Returns a human-readable name for a result code.
 * \param result Library result code.
 * \return Null-terminated string.
 */
YAZ0_API char const*
yaz0_result_name(enum yaz0_result result);

/*!
 * \brief Returns a human-readable description for a result code.
 * \param result Library result code.
 * \return Null-terminated string.
 */
YAZ0_API char const*
yaz0_result_string(enum yaz0_result result);

/*!
 * \brief Returns the library version as a packed integer.
 * \return Packed integer with the library version.
 * \see YAZ0_MAKE_VERSION
 */
YAZ0_API uint32_t
yaz0_version(void);

/*!
 * \brief Returns the library version as a string.
 * \return Version string.
 * \see YAZ0_VERSION_STRING
 */
YAZ0_API char const*
yaz0_version_string(void);

#if defined __cplusplus
}
#endif

#endif // LIBYAZ0_YAZ0_H
