# CMake build for libtiff
#
# Copyright © 2015 Open Microscopy Environment / University of Dundee
# Written by Roger Leigh <rleigh@codelibre.net>
#
# Permission to use, copy, modify, distribute, and sell this software and
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
#
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
#
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.

# Generate headers
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tif_config.h.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/tif_config.h
               @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tiffconf.h.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/tiffconf.h
               @ONLY)

extra_dist(
  SConstruct
  tif_config.h-vms
  tif_config.vc.h
  tif_config.wince.h
  tiffconf.vc.h
  tiffconf.wince.h
  libtiff.def
  libtiff.map
  libtiffxx.map)

set(tiff_HEADERS
  tiff.h
  tiffio.h
  tiffvers.h
  # XXX(kitware): install mangling header
  vtk_tiff_mangle.h)

set(tiff_noinst_HEADERS
  t4.h
  tif_dir.h
  tif_predict.h
  tiffiop.h
  uvcode.h)

set(nodist_tiff_HEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/tiffconf.h)

set(tiff_SOURCES
  tif_aux.c
  tif_close.c
  tif_codec.c
  tif_color.c
  tif_compress.c
  tif_dir.c
  tif_dirinfo.c
  tif_dirread.c
  tif_dirwrite.c
  tif_dumpmode.c
  tif_error.c
  tif_extension.c
  tif_fax3.c
  tif_fax3sm.c
  tif_flush.c
  tif_getimage.c
  tif_jbig.c
  tif_jpeg.c
  tif_jpeg_12.c
  tif_luv.c
  tif_lzma.c
  tif_lzw.c
  tif_next.c
  tif_ojpeg.c
  tif_open.c
  tif_packbits.c
  tif_pixarlog.c
  tif_predict.c
  tif_print.c
  tif_read.c
  tif_strip.c
  tif_swab.c
  tif_thunder.c
  tif_tile.c
  tif_version.c
  tif_warning.c
  tif_write.c
  tif_zip.c)

if (BUILD_SHARED_LIBS)
  set(tiff_DEFS
    libtiff.def)
endif ()

if(WIN32_IO)
  extra_dist(tif_unix.c)
  list(APPEND tiff_SOURCES tif_win32.c)
  add_definitions(-DUSE_WIN32_FILEIO)
else()
  extra_dist(tif_win32.c)
  list(APPEND tiff_SOURCES tif_unix.c)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${TIFF_INCLUDES})

if (FALSE) # XXX(kitware): mangle library name
add_library(tiff ${tiff_SOURCES} ${tiff_HEADERS} ${nodist_tiff_HEADERS}
            ${tiff_port_SOURCES} ${tiff_DEFS})
target_link_libraries(tiff ${TIFF_LIBRARY_DEPS})
set_target_properties(tiff PROPERTIES SOVERSION ${SO_COMPATVERSION})
endif ()

vtk_add_library(vtktiff ${tiff_SOURCES} ${tiff_HEADERS} ${nodist_tiff_HEADERS}
            ${tiff_port_SOURCES} ${tiff_DEFS})
target_link_libraries(vtktiff ${TIFF_LIBRARY_DEPS})
if(NOT CYGWIN)
    # This property causes shared libraries on Linux to have the full version
    # encoded into their final filename.  We disable this on Cygwin because
    # it causes cygz-${TIFF_FULL_VERSION}.dll to be created when cygz.dll
    # seems to be the default.
    if (FALSE) # XXX(kitware): mangle library name
    set_target_properties(tiff PROPERTIES VERSION ${SO_VERSION})
    endif ()
endif()
if(HAVE_LD_VERSION_SCRIPT)
  if (FALSE) # XXX(kitware): mangle library name
  set_target_properties(tiff PROPERTIES LINK_FLAGS
                        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libtiff.map")
  endif ()
  set_target_properties(vtktiff PROPERTIES LINK_FLAGS
                        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libtiff.map")
endif()

if (FALSE) # XXX(kitware): mangle library name
install(TARGETS tiff
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
endif ()

install(TARGETS vtktiff
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})

if (NOT_VTK_INSTALL_NO_DEVELOPMENT) # XXX(kitware): install headers when VTK does
install(FILES ${tiff_HEADERS} ${nodist_tiff_HEADERS}
        DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
endif ()
