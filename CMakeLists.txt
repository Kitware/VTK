set(sources
  src/base/ftbase.c
  src/base/ftbbox.c
  src/base/ftbitmap.c
  src/base/ftglyph.c
  src/base/ftinit.c
  src/base/ftmm.c
  src/bdf/bdf.c
  src/cache/ftcache.c
  src/cff/cff.c
  src/cid/type1cid.c
  src/gzip/ftgzip.c
  src/lzw/ftlzw.c
  src/pcf/pcf.c
  src/pfr/pfr.c
  src/psaux/psaux.c
  src/pshinter/pshinter.c
  src/psnames/psmodule.c
  src/raster/raster.c
  src/sfnt/sfnt.c
  src/smooth/smooth.c
  src/truetype/truetype.c
  src/type1/type1.c
  src/type42/type42.c
  src/winfonts/winfnt.c)

if (WIN32)
  list(APPEND sources
    builds/windows/ftdebug.c)
else ()
  list(APPEND sources
    src/base/ftdebug.c)
endif ()

if (UNIX)
  list(APPEND sources
    builds/unix/ftsystem.c)
  include(CheckIncludeFile)
  check_include_file("fcntl.h" HAVE_FCNTL_H)
  check_include_file("unistd.h" HAVE_UNISTD_H)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/builds/unix/ftconfig.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/freetype/config/ftconfig.h")
else ()
  list(APPEND sources
    src/base/ftsystem.c)
endif ()

vtk_module_add_module(VTK::freetype
  SOURCES ${sources})
target_compile_definitions(freetype
  PRIVATE
    FT2_BUILD_LIBRARY)
target_include_directories(freetype
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/include")

if (WIN32)
  target_compile_definitions(
    freetype PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
  if (BUILD_SHARED_LIBS)
    target_compile_definitions(
      freetype PRIVATE DLL_EXPORT)
  endif ()
endif ()

# If we want freetype's Mac font support we need to link against
# a couple of Mac OS X frameworks.
if (APPLE AND VTKFREETYPE_MAC_FONT_SUPPORT)
  target_link_libraries(freetype
    "-framework ApplicationServices -framework CoreServices")
endif ()

vtk_module_install_headers(
  SUBDIR  "vtkfreetype/include"
  FILES   "include/ft2build.h"
          "include/vtk_freetype_mangle.h")
vtk_module_install_headers(
  SUBDIR  "vtkfreetype/include/freetype"
  FILES   "include/freetype/freetype.h"
          "include/freetype/ftadvanc.h"
          "include/freetype/ftbbox.h"
          "include/freetype/ftbdf.h"
          "include/freetype/ftbitmap.h"
          "include/freetype/ftbzip2.h"
          "include/freetype/ftcache.h"
          "include/freetype/ftcid.h"
          "include/freetype/fterrdef.h"
          "include/freetype/fterrors.h"
          "include/freetype/ftgasp.h"
          "include/freetype/ftglyph.h"
          "include/freetype/ftgxval.h"
          "include/freetype/ftgzip.h"
          "include/freetype/ftimage.h"
          "include/freetype/ftincrem.h"
          "include/freetype/ftlcdfil.h"
          "include/freetype/ftlist.h"
          "include/freetype/ftlzw.h"
          "include/freetype/ftmac.h"
          "include/freetype/ftmm.h"
          "include/freetype/ftmodapi.h"
          "include/freetype/ftmoderr.h"
          "include/freetype/ftotval.h"
          "include/freetype/ftoutln.h"
          "include/freetype/ftpfr.h"
          "include/freetype/ftrender.h"
          "include/freetype/ftsizes.h"
          "include/freetype/ftsnames.h"
          "include/freetype/ftstroke.h"
          "include/freetype/ftsynth.h"
          "include/freetype/ftsystem.h"
          "include/freetype/fttrigon.h"
          "include/freetype/fttypes.h"
          "include/freetype/ftwinfnt.h"
          "include/freetype/t1tables.h"
          "include/freetype/ttnameid.h"
          "include/freetype/tttables.h"
          "include/freetype/tttags.h")
vtk_module_install_headers(
  SUBDIR  "vtkfreetype/include/freetype/config"
  FILES   "include/freetype/config/ftconfig.h"
          "include/freetype/config/ftheader.h"
          "include/freetype/config/ftmodule.h"
          "include/freetype/config/ftoption.h"
          "include/freetype/config/ftstdlib.h")
