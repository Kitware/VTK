ZFP
===

INTRODUCTION
------------

zfp is an open source C/C++ library for compressed numerical arrays that
support high throughput read and write random access.  zfp also supports
streaming compression of integer and floating-point data, e.g., for
applications that read and write large data sets to and from disk.

zfp was developed at Lawrence Livermore National Laboratory and is loosely
based on the algorithm described in the following paper:

    Peter Lindstrom
    "Fixed-Rate Compressed Floating-Point Arrays"
    IEEE Transactions on Visualization and Computer Graphics
    20(12):2674-2683, December 2014
    doi:10.1109/TVCG.2014.2346458

zfp was originally designed for floating-point arrays only, but has been
extended to also support integer data, and could for instance be used to
compress images and quantized volumetric data.  To achieve high compression
ratios, zfp uses lossy but optionally error-bounded compression.  Although
bit-for-bit lossless compression of floating-point data is not always
possible, zfp is usually accurate to within machine epsilon in near-lossless
mode.

zfp works best for 2D and 3D arrays that exhibit spatial correlation, such as
continuous fields from physics simulations, images, regularly sampled terrain
surfaces, etc.  Although zfp also provides a 1D array class that can be used
for 1D signals such as audio, or even unstructured floating-point streams,
the compression scheme has not been well optimized for this use case, and
rate and quality may not be competitive with floating-point compressors
designed specifically for 1D streams.

zfp is freely available as open source under a BSD license, as outlined in
the file 'LICENSE'.  For more information on zfp and comparisons with other
compressors, please see the zfp
[website](https://computation.llnl.gov/projects/floating-point-compression).
For questions, comments, requests, and bug reports, please contact
[Peter Lindstrom](mailto:pl@llnl.gov).


DOCUMENTATION
-------------

Full
[documentation](http://zfp.readthedocs.io/en/release0.5.2/)
is available online via Read the Docs.  A
[PDF](http://readthedocs.org/projects/zfp/downloads/pdf/release0.5.2/)
version is also available.


INSTALLATION
------------

zfp consists of three distinct parts: a compression library written in C;
a set of C++ header files that implement compressed arrays; and a set of
C and C++ examples.  The main compression codec is written in C and should
conform to both the ISO C89 and C99 standards.  The C++ array classes are
implemented entirely in header files and can be included as is, but since
they call the compression library, applications must link with libzfp.

On Linux, macOS, and MinGW, zfp is easiest compiled using gcc and gmake.
CMake support is also available, e.g. for Windows builds.  See below for
instructions on GNU and CMake builds.

zfp has successfully been built and tested using these compilers:

    gcc versions 4.4.7, 4.7.2, 4.8.2, 4.9.2, 5.4.1, 6.3.0
    icc versions 12.0.5, 12.1.5, 15.0.4, 16.0.1, 17.0.0, 18.0.0
    clang version 3.6.0
    xlc version 12.1
    MinGW version 5.3.0
    Visual Studio versions 14.0 (2015), 14.1 (2017)

NOTE: zfp requires 64-bit compiler and operating system support.

## GNU builds 

To compile zfp using gcc, type

    make

from this directory.  This builds libzfp as a static library as well as
utilities and example programs.  To optionally create a shared library,
type

    make shared

and set LD_LIBRARY_PATH to point to ./lib.  To test the compressor, type

    make test

If the compilation or regression tests fail, it is possible that some of
the macros in the file 'Config' have to be adjusted.  Also, the tests may
fail due to minute differences in the computed floating-point fields
being compressed (as indicated by checksum errors).  It is surprisingly
difficult to portably generate a floating-point array that agrees
bit-for-bit across platforms.  If most tests succeed and the failures
result in byte sizes and error values reasonably close to the expected
values, then it is likely that the compressor is working correctly.

## CMake builds

To build zfp using CMake on Linux or macOS, start a Unix shell and type

    mkdir build
    cd build
    cmake ..
    make

To also build the examples, replace the cmake line with

    cmake -DBUILD_EXAMPLES=ON ..

To build zfp using Visual Studio on Windows, start an MSBuild shell and type

    mkdir build
    cd build
    cmake ..
    msbuild /p:Configuration=Release zfp.sln
    msbuild /p:Configuration=Debug   zfp.sln

This builds zfp in both debug and release mode.  See the instructions for
Linux on how to change the cmake line to also build the example programs.
