This directory contains a subset of the Freetype2 library (2.4.7) and
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
  -you'll need to manually merge changes from newer freetypes.
  -the changes from the file it's based on are marked with VTK_FREETYPE_CHANGE:
    -conditional disabling of compiler warnings
    -disable FT_CONFIG_OPTION_INLINE_MULFIX, FT_CONFIG_OPTION_USE_LZW, FT_CONFIG_OPTION_USE_ZLIB, FT_CONFIG_OPTION_OLD_INTERNALS
    -additions to support DLL build for Windows

builds/unix/ftconfig.h.in:
  -new file, created from builds/unix/ftconfig.in
  -you'll need to manually merge changes from newer freetypes.
  -the changes from the file it's based on are marked with VTK_FREETYPE_CHANGE:
    -use CMake variables

CMakeLists.txt
  -to support CMake builds

README.VTK.txt
  - this file

include/vtk_freetype_mangle.h
  -mangles all symbols exported from the freetype library
  -should be regenerated when updating freetype, see the file for instructions

include/vtk_ftmodule.h
  -new file, created from include/freetype/config/ftmodule.h
  -you'll need to manually merge changes from newer freetypes.
  -the changes from the file it's based on are marked with VTK_FREETYPE_CHANGE:
    -removed one module, which is not needed by VTK

include/vtkFreeTypeConfig.h.in
  -purpose unknown
  -does not appear to be based on a file from freetype itself

Changed Files
-------------
include/ft2build.h:
  -extensive changes, see file for comments

include/freetype/config/ftoption.h:
  -disable FT_CONFIG_OPTION_INLINE_MULFIX, FT_CONFIG_OPTION_USE_LZW, FT_CONFIG_OPTION_USE_ZLIB, FT_CONFIG_OPTION_OLD_INTERNALS

src/pshinter/pshalgo.c:
  -commented out piece of code to workaround a bug, see bug 10052.

searching for "VTK_FREETYPE_CHANGE" is a good idea too.
