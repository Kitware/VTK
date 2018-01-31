set(sources
  jcapimin.c
  jcapistd.c
  jccoefct.c
  jccolor.c
  jcdctmgr.c
  jchuff.c
  jcinit.c
  jcmainct.c
  jcmarker.c
  jcmaster.c
  jcomapi.c
  jcparam.c
  jcphuff.c
  jcprepct.c
  jcsample.c
  jctrans.c
  jdapimin.c
  jdapistd.c
  jdatadst.c
  jdatasrc.c
  jdcoefct.c
  jdcolor.c
  jddctmgr.c
  jdhuff.c
  jdinput.c
  jdmainct.c
  jdmarker.c
  jdmaster.c
  jdmerge.c
  jdphuff.c
  jdpostct.c
  jdsample.c
  jdtrans.c
  jerror.c
  jfdctflt.c
  jfdctfst.c
  jfdctint.c
  jidctflt.c
  jidctfst.c
  jidctint.c
  jidctred.c
  jmemmgr.c
  jmemnobs.c
  jquant1.c
  jquant2.c
  jutils.c

  jaricom.c
  jcarith.c
  jdarith.c
  jsimd_none.c)

include(CheckSymbolExists)
check_symbol_exists("memset" "string.h" HAVE_MEMSET)
check_symbol_exists("memcpy" "string.h" HAVE_MEMCPY)
if (NOT HAVE_MEMSET OR NOT HAVE_MEMCPY)
  set(NEED_BSD_STRINGS 1)
endif ()

include(CheckIncludeFile)
check_include_file("stddef.h" HAVE_STDDEF_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)

# VTK already assumes these exist.
set(HAVE_UNSIGNED_CHAR 1)
set(HAVE_UNSIGNED_SHORT 1)

set(JPEG_LIB_VERSION 80)
set(LIBJPEG_TURBO_VERSION "1.5.3")
set(LIBJPEG_TURBO_VERSION_NUMBER "1005003")
set(MEM_SRCDST_SUPPORTED 1)
set(C_ARITH_CODING_SUPPORTED 1)
set(D_ARITH_CODING_SUPPORTED 1)
set(NEED_SYS_TYPES_H 1)

if (WIN32)
  set(header_input_dir "/win")
else ()
  set(header_input_dir "/")
endif ()

if (BUILD_SHARED_LIBS)
  set(JPEG_SHARED_LIBS 1)
else ()
  set(JPEG_SHARED_LIBS 0)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}${header_input_dir}/jconfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/jconfig.h")

set(headers
  jerror.h
  jmorecfg.h
  jpeglib.h
  vtk_jpeg_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/jconfig.h")

include(CheckTypeSize)
check_type_size("size_t" SIZEOF_SIZE_T)
set(VERSION "1.5.3")
# TODO: See `configure.ac` for `ljt_cv_inline` for this part.
set(INLINE "inline")
set(PACKAGE_NAME "vtkjpeg")
string(TIMESTAMP BUILD "%Y%m%d")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}${header_input_dir}/jconfigint.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/jconfigint.h")

set(private_headers
  "${CMAKE_CURRENT_BINARY_DIR}/jconfigint.h")

vtk_add_library(vtkjpeg ${sources} ${headers} ${private_headers})
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkjpeg"
    COMPONENT Development)
endif()
target_include_directories(vtkjpeg
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
