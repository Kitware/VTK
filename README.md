ZFP
===
[![Travis CI Build Status](https://travis-ci.org/LLNL/zfp.svg?branch=develop)](https://travis-ci.org/LLNL/zfp)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/LLNL/zfp?branch=develop&svg=true)](https://ci.appveyor.com/project/salasoom/zfp)
[![Documentation Status](https://readthedocs.org/projects/zfp/badge/?version=release0.5.5)](https://zfp.readthedocs.io/en/release0.5.5/?badge=release0.5.5)
[![Codecov](https://codecov.io/gh/LLNL/zfp/branch/develop/graph/badge.svg)](https://codecov.io/gh/LLNL/zfp)

INTRODUCTION
------------

zfp is an open source C/C++ library for compressed numerical arrays that
support high throughput read and write random access.  zfp also supports
streaming compression of integer and floating-point data, e.g., for
applications that read and write large data sets to and from disk.
zfp is primarily written in C and C++ but also includes Python and
Fortran bindings.

zfp was developed at Lawrence Livermore National Laboratory and is loosely
based on the algorithm described in the following paper:

    Peter Lindstrom
    "Fixed-Rate Compressed Floating-Point Arrays"
    IEEE Transactions on Visualization and Computer Graphics
    20(12):2674-2683, December 2014
    doi:10.1109/TVCG.2014.2346458

zfp was originally designed for floating-point arrays only, but has been
extended to also support integer data and could for instance be used to
compress images and quantized volumetric data.  To achieve high compression
ratios, zfp generally uses lossy but optionally error-bounded compression.
Bit-for-bit lossless compression is also possible through one of zfp's
compression modes.

zfp works best for 2D and 3D arrays that exhibit spatial correlation, such as
continuous fields from physics simulations, images, regularly sampled terrain
surfaces, etc.  Although zfp also provides a 1D array class that can be used
for 1D signals such as audio, or even unstructured floating-point streams,
the compression scheme has not been well optimized for this use case, and
rate and quality may not be competitive with floating-point compressors
designed specifically for 1D streams.  zfp also supports compression of
4D arrays.

zfp is freely available as open source under a BSD license, as outlined in
the file 'LICENSE'.  For more information on zfp and comparisons with other
compressors, please see the
[zfp website](https://computation.llnl.gov/projects/floating-point-compression).
For bug reports, please consult the
[GitHub issue tracker](https://github.com/LLNL/zfp/issues).
For questions, comments, and requests, please contact
[Peter Lindstrom](mailto:pl@llnl.gov).


DOCUMENTATION
-------------

Full
[documentation](http://zfp.readthedocs.io/en/release0.5.5/)
is available online via Read the Docs.  A
[PDF](http://readthedocs.org/projects/zfp/downloads/pdf/release0.5.5/)
version is also available.


INSTALLATION
------------

zfp consists of three distinct parts: a compression library written in C;
a set of C++ header files with C wrappers that implement compressed arrays;
and a set of C and C++ examples.  The main compression codec is written in
C and should conform to both the ISO C89 and C99 standards.  The C++ array
classes are implemented entirely in header files and can be included as is,
but since they call the compression library, applications must link with
libzfp.

On Linux, macOS, and MinGW, zfp is easiest compiled using gcc and gmake.
CMake support is also available, e.g., for Windows builds.  See below for
instructions on GNU and CMake builds.

zfp has successfully been built and tested using these compilers:

    gcc versions 4.4.7, 4.9.4, 5.5.0, 6.1.0, 6.4.0, 7.1.0, 7.3.0, 8.1.0
    icc versions 15.0.6, 16.0.4, 17.0.2, 18.0.2, 19.0.0
    clang versions 3.9.1, 4.0.0, 5.0.0, 6.0.0 
    MinGW version 5.3.0
    Visual Studio versions 14 (2015), 15 (2017)

zfp conforms to various language standards, including C89, C99, C11,
C++98, C++11, and C++14.

NOTE: zfp requires 64-bit compiler and operating system support.

## GNU builds 

To build zfp using gcc, type

    make

from this directory.  This builds libzfp as a static library as well as
utilities and example programs.  See documentation for complete build
instructions.

## CMake builds

To build zfp using CMake on Linux or macOS, start a Unix shell and type

    mkdir build
    cd build
    cmake ..
    make

To also build the examples, replace the cmake line with

    cmake -DBUILD_EXAMPLES=ON ..

To build zfp using Visual Studio on Windows, start a DOS shell, cd to the
top-level zfp directory, and type

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

This builds zfp in release mode.  Replace 'Release' with 'Debug' to build
zfp in debug mode.  See the instructions for Linux on how to change the
cmake line to also build the example programs.

## Testing

To test that zfp is working properly, type

    make test

or using CMake

    ctest

If the compilation or regression tests fail, it is possible that some of the
macros in the file 'Config' have to be adjusted.  Also, the tests may fail
due to minute differences in the computed floating-point fields being
compressed, which will be indicated by checksum errors.  If most tests
succeed and the failures result in byte sizes and error values reasonably
close to the expected values, then it is likely that the compressor is
working correctly.
