This directory contains a subset of the Freetype library (2.1.9) + some recent
updates made to the cache subsystem.

We only include enough of the distribution to provide the functionalities
required by VTK.

We would like to thank the Freetype team for distributing this library.
http://www.freetype.org

Modifications
-------------
builds\win32\freetype\config\ftoption.h: 
  new file, created from include\freetype\config\ftoption.h, used to 
  support DLL build for Windows

builds\unix\ftconfig.h.in: 
  new file, created from ftconfig.in, use CMake vars

include\ft2build.h:
  added:
#if defined(VTKFREETYPE)
#include "vtkFreeTypeConfig.h"
#endif

include\freetype\config\ftoption.h:
builds\win32\freetype\config\ftoption.h: 
  comment out FT_CONFIG_OPTION_USE_ZLIB and FT_CONFIG_OPTION_USE_LZW:
/* #define FT_CONFIG_OPTION_USE_ZLIB */
/* #define FT_CONFIG_OPTION_USE_LZW */

---- DO NOT APPLY ANYMORE BUT KEEP AN EYE ON MAC -------
src\base\ftmac.c:
  added: 
#if defined(FT_USE_CARBON_HEADER)
#include <Carbon/Carbon.h>
#else
[...]
#endif
---- DO NOT APPLY ANYMORE BUT KEEP AN EYE ON MAC -------
    
---- DO NOT APPLY ANYMORE BUT KEEP AN EYE ON MAC -------
src\raster\ftrend1.c:
  replaced:
   pitch = ( ( width + 15 ) >> 4 ) << 1;
  by the old code:
   pitch = ( width + 7 ) >> 3;
---- DO NOT APPLY ANYMORE BUT KEEP AN EYE ON MAC -------

