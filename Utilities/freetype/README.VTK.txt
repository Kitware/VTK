This directory contains a subset of the Freetype library (2.1.2).
We only include enough of distribution to build it.

We'd like to thank the Freetype team for distributing this library.
http://www.freetype.org

Modifications
-------------
freetype\builds\win32\freetype\config\ftoption.h: 
  new file used to support DLL build for Windows

freetype\builds\unix\ftconfig.h.in: 
  new file, created from ftconfig.in, use CMake vars

src\base\ftmac.c:
  added: 
#if defined(FT_USE_CARBON_HEADER)
#include <Carbon/Carbon.h>
#else
[...]
#endif

src\raster\ftrend1.c:
  replaced:
   pitch = ( ( width + 15 ) >> 4 ) << 1;
  by the old code:
   pitch = ( width + 7 ) >> 3;

include\ft2build.h:
  added:
#if defined(VTKFREETYPE)
#include "vtkfreetypeConfig.h"
#endif
