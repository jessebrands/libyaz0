# libyaz0

A library for (de)compressing data using Nintendo's Yaz0 algorithm.

## Origin

The *libyaz0* project is a subproject of the *Zelda64* project, a library and
tool for manipulating Nintendo 64 Zelda ROMs. The library was originally
conceived for use in the Ocarina of Time Randomizer to provide a much faster
implementation of the Yaz0 algorithm.

## Performance

Inflate performance should be roughly on par with other decompressors.

Deflate performance is very, very fast on hardware that has support for SSE4.2
or AVX2 support, which is virtually all consumer x86 CPUs currently being used
today. Support for AVX512 and ARM Neon SIMD instruction sets is currently on
the roadmap.

Deflate performance without hardware vectorization is unfortunately slow.

## License

*libyaz0*'s code and documentation are covered by the GPL version 3 or
(at your option) any later version. See the `LICENSE` file for the full text
of this license.

## Building

*libyaz0* was written in standards compliant C11 with no dependencies other
than the C standard library. To build the library, the following has to be
installed:

- CMake 3.20 or higher.
- A standards compliant C11 compiler.

The library has been built and tested with the following compilers:

- Microsoft Visual C/C++ Compiler 19.38.33145
- GNU GCC 13.1.0

The following options can be passed to CMake to control the build process:

### `YAZ0_BUILD_TESTS`

If enabled, CMake will automatically download GoogleTest and build tests. This
option requires a C++17 compiler be installed. Defaults to `ON`.

### `YAZ0_BIG_ENDIAN`

If enabled, Yaz0 will be built in big endian mode. Defaults to `OFF`.
