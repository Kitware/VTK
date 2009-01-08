This directory contains a subset of the Freetype2 library (2.3.7) and
some custom changes that VTK needs.

We only include enough of the distribution to provide the functionalities
required by VTK.

We would like to thank the Freetype team for distributing this library.
http://www.freetype.org

Modifications
-------------
You can search for code for "VTK_FREETYPE_CHANGE" to find modifications
vs the original freetype code

Added Files
-----------
builds/win32/freetype/config/ftoption.h: 
  -new file, created from include/freetype/config/ftoption.h
  -additions to support DLL build for Windows
  -comment out FT_CONFIG_OPTION_USE_ZLIB and FT_CONFIG_OPTION_USE_LZW

builds/unix/ftconfig.h.in: 
  -new file, created from builds/unix/ftconfig.in
  -use CMake variables

CMakeLists.txt
  -to support CMake builds

include/vtk_freetype_mangle.h
  -mangles all symbols exported from the freetype library

include/vtk_ftmodule.h
  -override the default module headers

include/vtkFreeTypeConfig.h.in
  -purpose unknown

Changed Files
-------------
include/ft2build.h:
  -extensive changes, see file for comments

include/freetype/config/ftoption.h:
  -comment out FT_CONFIG_OPTION_USE_ZLIB and FT_CONFIG_OPTION_USE_LZW
  
src/pshinter/pshalgo.c:
  -commented out piece of code to workaround a bug, see bug 7199.

src/base/ftmac.c
  -removed code to fix a warning (change from freetype CVS)

other files have changes too, search for "VTK_FREETYPE_CHANGE"

Renamed Files
-------------
docs/FTL.TXT -> FTL.txt
docs/LICENSE.TXT -> license.txt
