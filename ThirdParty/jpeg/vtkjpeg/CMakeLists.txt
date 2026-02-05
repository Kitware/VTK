set(sources
  jcapimin.c
  jcapistd.c
  jccoefct.c
  jccolor.c
  jcdctmgr.c
  jchuff.c
  jcicc.c
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
  jdicc.c
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
  jquant1.c
  jquant2.c
  jutils.c
  jmemmgr.c
  jmemnobs.c

  jaricom.c
  jcarith.c
  jdarith.c)

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
set(LIBJPEG_TURBO_VERSION "2.0.0")
set(LIBJPEG_TURBO_VERSION_NUMBER "2000000")
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
else()
  list(APPEND sources
    jsimd_none.c
  )
endif()

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
check_type_size("size_t" SIZE_T)
set(SIZEOF_SIZE_T "${SIZE_T}")
set(VERSION "${LIBJPEG_TURBO_VERSION}")
# TODO: See `configure.ac` for `ljt_cv_inline` for this part.
set(INLINE "inline")
set(PACKAGE_NAME "vtkjpeg")
string(TIMESTAMP BUILD "%Y%m%d")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/jconfigint.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/jconfigint.h")

set(private_headers
  "${CMAKE_CURRENT_BINARY_DIR}/jconfigint.h")

vtk_module_add_module(VTK::jpeg
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkjpeg"
  PRIVATE_HEADERS ${private_headers})
vtk_module_include(VTK::jpeg
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

if(WITH_SIMD)
  target_include_directories(simd PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
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
