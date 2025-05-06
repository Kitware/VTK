LodePNG
-------

PNG encoder and decoder in C and C++, without dependencies

Home page: http://lodev.org/lodepng/

### Documentation

Detailed documentation is included in a large comment in the second half of the
header file lodepng.h.

Source code examples using LodePNG can be found in the examples directory.

An FAQ can be found on http://lodev.org/lodepng/

### Building

Only two files are needed to encode and decode PNGs:

* lodepng.cpp (or lodepng.c)
* lodepng.h

All other files are just source code examples, tests, misc utilities, etc..., which
are normally not needed in projects using this.

You can include the files directly in your project's source tree and its
makefile, IDE project file, or other build system. No library is necessary.

In addition to C++, LodePNG also supports ANSI C (C89), with all the same
functionality: C++ only adds extra convenience API.

For C, rename lodepng.cpp to lodepng.c.

Consider using git submodules to include LodePNG in your project.

### Building in C++

If you have a hypothetical main.cpp that #includes and uses lodepng.h, you can build as follows:

g++ main.cpp lodepng.cpp -Wall -Wextra -pedantic -ansi -O3

or:

clang++ main.cpp lodepng.cpp -Wall -Wextra -pedantic -ansi -O3

This shows compiler flags it was designed for, but normally one would use the
compiler or build system of their project instead of those commands, and other
C++ compilers are supported.

### Building in C

Rename lodepng.cpp to lodepng.c for this.

If you have a hypothetical main.c that #includes and uses lodepng.h, you can build as follows:

gcc main.c lodepng.c -ansi -pedantic -Wall -Wextra -O3

or

clang main.c lodepng.c -ansi -pedantic -Wall -Wextra -O3

This shows compiler flags it was designed for, but normally one would use the
compiler or build system of their project instead of those commands, and other
C compilers are supported.
