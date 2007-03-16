This directory contains a subset of the Freetype2 library (2.1.9) + some later
updates made to the cache subsystem.

We only include enough of the distribution to provide the functionalities
required by VTK.

We would like to thank the Freetype team for distributing this library.
http://www.freetype.org

Modifications
-------------

Added Files
-----------
builds\win32\freetype\config\ftoption.h: 
  -new file, created from include\freetype\config\ftoption.h, used to 
  support DLL build for Windows
  -comment out FT_CONFIG_OPTION_USE_ZLIB and FT_CONFIG_OPTION_USE_LZW:
/* #define FT_CONFIG_OPTION_USE_ZLIB */
/* #define FT_CONFIG_OPTION_USE_LZW */

builds\unix\ftconfig.h.in: 
  -new file, created from ftconfig.in, use CMake vars

CMakeLists.txt
  -to support CMake builds

include/vtk_freetype_mangle.h
  -mangles all symbols exported from the freetype library

include/vtkFreeTypeConfig.h.in
  -purpose unknown

Changed Files
-------------
include\ft2build.h:
  -added:
#include "vtk_freetype_mangle.h"  
  -added:
#if defined(VTKFREETYPE)
#include "vtkFreeTypeConfig.h"
#endif

src/base/ftmac.c
  -fixed warnings
  -other misc changes

include\freetype\config\ftoption.h:
  -comment out FT_CONFIG_OPTION_USE_ZLIB and FT_CONFIG_OPTION_USE_LZW:
/* #define FT_CONFIG_OPTION_USE_ZLIB */
/* #define FT_CONFIG_OPTION_USE_LZW */

builds/unix/ftsystem.c
  -fixed some 64 to 32 implicit conversion warnings in FT_Stream_Open

include/freetype/config/ftstdlib.h
  -fixed a warning with the sgi compiler

Other files are changed compared to freetype 2.1.9, but those changes
are by the freetype people.
