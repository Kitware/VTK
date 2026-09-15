set(sources
  src/jcapimin.c
  src/jcapistd.c
  src/jccoefct.c
  src/jccolor.c
  src/jcdctmgr.c
  src/jchuff.c
  src/jcicc.c
  src/jcinit.c
  src/jcmainct.c
  src/jcmarker.c
  src/jcmaster.c
  src/jcomapi.c
  src/jcparam.c
  src/jcphuff.c
  src/jcprepct.c
  src/jcsample.c
  src/jctrans.c
  src/jdapimin.c
  src/jdapistd.c
  src/jdatadst.c
  src/jdatasrc.c
  src/jdcoefct.c
  src/jdcolor.c
  src/jddctmgr.c
  src/jdhuff.c
  src/jdicc.c
  src/jdinput.c
  src/jdmainct.c
  src/jdmarker.c
  src/jdmaster.c
  src/jdmerge.c
  src/jdphuff.c
  src/jdpostct.c
  src/jdsample.c
  src/jdtrans.c
  src/jerror.c
  src/jfdctflt.c
  src/jfdctfst.c
  src/jfdctint.c
  src/jidctflt.c
  src/jidctfst.c
  src/jidctint.c
  src/jidctred.c
  src/jquant1.c
  src/jquant2.c
  src/jutils.c
  src/jmemmgr.c
  src/jmemnobs.c

  src/jcdiffct.c
  src/jclhuff.c
  src/jclossls.c
  src/jddiffct.c
  src/jdlhuff.c
  src/jdlossls.c
  src/jpeg_nbits.c

  src/jaricom.c
  src/jcarith.c
  src/jdarith.c)

# libjpeg-turbo 3.0 supports run-time selection of 8-, 12-, and 16-bit data
# precision. The sources above are compiled as 8-bit (the default
# `BITS_IN_JSAMPLE`). These wrappers provide the 12- and 16-bit variants.
list(APPEND sources
  src/wrapper/jcapistd-12.c
  src/wrapper/jccoefct-12.c
  src/wrapper/jccolor-12.c
  src/wrapper/jcdctmgr-12.c
  src/wrapper/jcdiffct-12.c
  src/wrapper/jclossls-12.c
  src/wrapper/jcmainct-12.c
  src/wrapper/jcprepct-12.c
  src/wrapper/jcsample-12.c
  src/wrapper/jdapistd-12.c
  src/wrapper/jdcoefct-12.c
  src/wrapper/jdcolor-12.c
  src/wrapper/jddctmgr-12.c
  src/wrapper/jddiffct-12.c
  src/wrapper/jdlossls-12.c
  src/wrapper/jdmainct-12.c
  src/wrapper/jdmerge-12.c
  src/wrapper/jdpostct-12.c
  src/wrapper/jdsample-12.c
  src/wrapper/jfdctfst-12.c
  src/wrapper/jfdctint-12.c
  src/wrapper/jidctflt-12.c
  src/wrapper/jidctfst-12.c
  src/wrapper/jidctint-12.c
  src/wrapper/jidctred-12.c
  src/wrapper/jquant1-12.c
  src/wrapper/jquant2-12.c
  src/wrapper/jutils-12.c

  src/wrapper/jcapistd-16.c
  src/wrapper/jccolor-16.c
  src/wrapper/jcdiffct-16.c
  src/wrapper/jclossls-16.c
  src/wrapper/jcmainct-16.c
  src/wrapper/jcprepct-16.c
  src/wrapper/jcsample-16.c
  src/wrapper/jdapistd-16.c
  src/wrapper/jdcolor-16.c
  src/wrapper/jddiffct-16.c
  src/wrapper/jdlossls-16.c
  src/wrapper/jdmainct-16.c
  src/wrapper/jdpostct-16.c
  src/wrapper/jdsample-16.c
  src/wrapper/jutils-16.c)

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

set(BITS_IN_JSAMPLE 8)

set(JPEG_LIB_VERSION 80)
set(LIBJPEG_TURBO_VERSION "3.1.4.1")
set(LIBJPEG_TURBO_VERSION_NUMBER "3001004")
set(MEM_SRCDST_SUPPORTED 1)
set(C_ARITH_CODING_SUPPORTED 1)
set(D_ARITH_CODING_SUPPORTED 1)
set(NEED_SYS_TYPES_H 1)

if (BUILD_SHARED_LIBS)
  set(JPEG_SHARED_LIBS 1)
else ()
  set(JPEG_SHARED_LIBS 0)
endif ()

# Macro from libjpeg-turbo
macro(boolean_number var)
  if(${var})
    set(${var} 1 ${ARGN})
  else()
    set(${var} 0 ${ARGN})
  endif()
endmacro()

# Check from libjpeg-turbo
# Detect CPU type and whether we're building 64-bit or 32-bit code
math(EXPR BITS "${CMAKE_SIZEOF_VOID_P} * 8")
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} CMAKE_SYSTEM_PROCESSOR_LC)
if(CMAKE_SYSTEM_PROCESSOR_LC MATCHES "x86_64" OR
  CMAKE_SYSTEM_PROCESSOR_LC MATCHES "amd64" OR
  CMAKE_SYSTEM_PROCESSOR_LC MATCHES "i[0-9]86" OR
  CMAKE_SYSTEM_PROCESSOR_LC MATCHES "x86" OR
  CMAKE_SYSTEM_PROCESSOR_LC MATCHES "ia32")
  if(BITS EQUAL 64 OR CMAKE_C_COMPILER_ABI MATCHES "ELF X32")
    set(CPU_TYPE x86_64)
  else()
    set(CPU_TYPE i386)
  endif()
  if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL ${CPU_TYPE})
    set(CMAKE_SYSTEM_PROCESSOR ${CPU_TYPE})
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR_LC STREQUAL "aarch64" OR
  CMAKE_SYSTEM_PROCESSOR_LC MATCHES "^arm")
  if(BITS EQUAL 64)
    set(CPU_TYPE arm64)
  else()
    set(CPU_TYPE arm)
  endif()
else()
  set(CPU_TYPE ${CMAKE_SYSTEM_PROCESSOR_LC})
endif()
if(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" OR
  CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR
  CMAKE_OSX_ARCHITECTURES MATCHES "i386")
  set(CPU_TYPE ${CMAKE_OSX_ARCHITECTURES})
endif()
if(MSVC_IDE AND CMAKE_GENERATOR_PLATFORM MATCHES "arm64")
  set(CPU_TYPE arm64)
endif()

option(VTK_JPEG_ENABLE_SIMD "Enables SIMD extensions when compiling libjpeg-turbo" OFF)
mark_as_advanced(VTK_JPEG_ENABLE_SIMD)
if (VTK_JPEG_ENABLE_SIMD AND NOT EMSCRIPTEN)
  set(WITH_SIMD 1)
  # Force full ARM Neon instrinsics
  if (CPU_TYPE MATCHES "arm")
    set(NEON_INTRINSICS ON)
  endif ()
  # WITH_SIMD will be set to 0 if SIMD configuration failed
  add_subdirectory(simd)
  if(NOT WITH_SIMD)
    message(FATAL_ERROR "VTK_JPEG_ENABLE_SIMD=ON but SIMD compilation is not available")
  endif()
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/jconfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/src/jconfig.h")

set(headers
  src/jerror.h
  src/jmorecfg.h
  src/jpeglib.h
  src/vtk_jpeg_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/src/jconfig.h")

include(CheckTypeSize)
check_type_size("size_t" SIZE_T)
set(SIZEOF_SIZE_T "${SIZE_T}")
set(VERSION "${LIBJPEG_TURBO_VERSION}")
# TODO: See `configure.ac` for `ljt_cv_inline` for this part.
set(INLINE "inline")
set(PACKAGE_NAME "vtkjpeg")
string(TIMESTAMP BUILD "%Y%m%d")

# libjpeg-turbo 3.0 added `HIDDEN` and `THREAD_LOCAL` to `jconfigint.h.in`.
include(CheckCSourceCompiles)
if (NOT WIN32)
  check_c_source_compiles("extern const int table[1]; const int __attribute__((visibility(\"hidden\"))) table[1] = { 0 }; int main(void) { return table[0]; }"
    HIDDEN_WORKS)
  if (HIDDEN_WORKS)
    set(HIDDEN "__attribute__((visibility(\"hidden\")))")
  endif ()
endif ()
if (MSVC)
  set(THREAD_LOCAL "__declspec(thread)")
else ()
  set(THREAD_LOCAL "__thread")
endif ()
check_c_source_compiles("static ${THREAD_LOCAL} int i;  int main(void) { i = 0;  return i; }"
  HAVE_THREAD_LOCAL)
if (NOT HAVE_THREAD_LOCAL)
  unset(THREAD_LOCAL)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/jconfigint.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/src/jconfigint.h")

# libjpeg-turbo 3.0 turned jversion.h into a configured template.
set(COPYRIGHT_YEAR "1991-2026")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/jversion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/src/jversion.h")

set(private_headers
  "${CMAKE_CURRENT_BINARY_DIR}/src/jconfigint.h"
  "${CMAKE_CURRENT_BINARY_DIR}/src/jversion.h")

vtk_module_add_module(VTK::jpeg
  SOURCES ${sources}
  HEADERS ${headers}
  HEADER_DIRECTORIES
  HEADERS_SUBDIR "vtkjpeg"
  PRIVATE_HEADERS ${private_headers})
vtk_module_include(VTK::jpeg
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>")

if(WITH_SIMD)
  target_include_directories(simd PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>")
  # XXX(cmake-3.26): `BUILD_LOCAL_INTERFACE` genex.
  if (CMAKE_VERSION VERSION_LESS "3.26")
    # Avoid needing to export the `simd` target. Only works as long as
    # `simd` is "just" an `OBJECT` library and doesn't have other usage
    # requirements.
    vtk_module_sources(VTK::jpeg PRIVATE "$<TARGET_OBJECTS:simd>")
  else ()
    # No need to export the target at all.
    vtk_module_link(VTK::jpeg PRIVATE "$<BUILD_LOCAL_INTERFACE:simd>")
  endif ()
  if(WIN32)
    target_compile_definitions(simd PRIVATE jpeg_EXPORTS=1)
  endif()
endif()
