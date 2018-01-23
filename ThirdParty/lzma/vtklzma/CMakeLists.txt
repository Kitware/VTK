# liblzma wrapped for VTK by Quincy Wofford and John Patchett,
# Los Alamos National Laboratory (2017).
# Thanks to Lasse Collin for integration help

# Note: In general, liblzma checks for the presense of absense
#       of an environment variable to make build decisions, NOT
#       the value assigned to that definition.

cmake_minimum_required(VERSION 3.2)

project(LZMA VERSION 5.2.3 LANGUAGES C)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LZMA compression library")

# XXX(kitware): avoid quoted variable policy warning from CMake.
IF(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
  SET(GNU_COMPATIBLE_COMPILER 1)
ENDIF()

if(GNU_COMPATIBLE_COMPILER)
  if(UNIX AND BUILD_LIBS)
    add_definitions(-fPIC)
  endif()
endif()

include(TestBigEndian)
include(CheckIncludeFile)
include(CheckTypeSize)
include(ProcessorCount)

test_big_endian(WORDS_BIGENDIAN)
set(HAVE_BSWAP_16 1)
set(HAVE_BSWAP_32 1)
set(HAVE_BSWAP_64 1)

# Only set if true! leave undef if false
# See Note at start of this file
check_include_file("strings.h" HAVE_STRINGS_H_)
if(HAVE_STRINGS_H_)
  set(HAVE_STRINGS_H 1)
endif()

# Only set if true! leave undef if false
# See Note at start of this file
check_include_file("byteswap.h" HAVE_BYTESWAP_H_)
if(HAVE_BYTESWAP_H_)
  set(HAVE_BYTESWAP_H)
endif()

check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("limits.h" HAVE_LIMITS_H)
check_include_file("stdbool.h" HAVE_STDBOOL_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("memory.h" HAVE_MEMORY_H)

check_type_size(int16_t INT16_T)
check_type_size(int32_t INT32_T)
check_type_size(int64_t INT64_T)
check_type_size(intmax_t INTMAX_T)
check_type_size(uint8_t UINT8_T)
check_type_size(uint16_t UINT16_T)
check_type_size(uint32_t UINT32_T)
check_type_size(uint64_t UINT64_T)
check_type_size(uintmax_t UINTMAX_T)

check_type_size("short" SIZE_OF_SHORT)
check_type_size("int" SIZE_OF_INT)
check_type_size("long" SIZE_OF_LONG)
check_type_size("long long" SIZE_OF_LONG_LONG)

check_type_size("unsigned short" SIZE_OF_UNSIGNED_SHORT)
check_type_size("unsigned" SIZE_OF_UNSIGNED)
check_type_size("unsigned long" SIZE_OF_UNSIGNED_LONG)
check_type_size("unsigned long long" SIZE_OF_UNSIGNED_LONG_LONG)
check_type_size("size_t" SIZE_OF_SIZE_T)

check_type_size("__int64" __INT64)
check_type_size("unsigned __int64" UNSIGNED___INT64)

check_type_size(uintptr_t UINTPTR_T)
IF(NOT HAVE_UINTPTR_T)
  IF("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    SET(uintptr_t "uint64_t")
  ELSE()
    SET(uintptr_t "uint32_t")
  ENDIF()
ENDIF()

# We will need to add some encoder/decoder checking here
set(HAVE_DECODER_ARM 1)
set(HAVE_DECODER_ARMTHUMB 1)
set(HAVE_DECODER_DELTA 1)
set(HAVE_DECODER_IA64 1)
set(HAVE_DECODER_LZMA1 1)
set(HAVE_DECODER_LZMA2 1)
set(HAVE_DECODER_POWERPC 1)
set(HAVE_DECODER_SPARC 1)
set(HAVE_DECODER_X86 1)
set(HAVE_DECODERS 1)

set(HAVE_DLFCN_H 1)

set(HAVE_ENCODER_ARM 1)
set(HAVE_ENCODER_ARMTHUMB 1)
set(HAVE_ENCODER_DELTA 1)
set(HAVE_ENCODER_IA64 1)
set(HAVE_ENCODER_LZMA1 1)
set(HAVE_ENCODER_LZMA2 1)
set(HAVE_ENCODER_POWERPC 1)
set(HAVE_ENCODER_SPARC 1)
set(HAVE_ENCODER_X86 1)
set(HAVE_ENCODERS 1)

set(HAVE_MF_BT2 1)
set(HAVE_MF_BT3 1)
set(HAVE_MF_BT4 1)
set(HAVE_MF_HC3 1)
set(HAVE_MF_HC4 1)

# Define some checks
set(HAVE_CHECK_CRC32 1)
set(HAVE_CHECK_CRC64 1)
set(HAVE_CHECK_SHA256 1)
set(HAVE_DECODERS 1)

# Performance optimization
set(TUKLIB_FAST_UNALIGNED_ACCESS 1)
# Additional required mangling (var used by liblzma during build)
set(TUKLIB_SYMBOL_PREFIX vtklzma_)

find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
  set(MYTHREAD_POSIX 1)
elseif (CMAKE_USE_WIN32_THREADS_INIT)
  set(MYTHREAD_VISTA 1)
else ()
  message(FATAL_ERROR
    "Unsupported threading backend for LZMA.")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h")
add_definitions(-DHAVE_CONFIG_H)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma
  ${CMAKE_CURRENT_SOURCE_DIR}/src/common
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lz
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/delta
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/rangecoder
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common
  ${CMAKE_CURRENT_SOURCE_DIR}/windows
)


set(LZMA_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/tuklib_cpucores.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/tuklib_physmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/check.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc32_fast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc32_table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc64_fast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc64_table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/sha256.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/alone_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/alone_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/auto_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_buffer_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_buffer_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_header_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_header_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/easy_buffer_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/easy_decoder_memusage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/easy_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/easy_encoder_memusage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/easy_preset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_buffer_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_buffer_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_flags_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_flags_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/hardware_cputhreads.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/hardware_physmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/index.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/index_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/index_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/index_hash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/outqueue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_buffer_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_buffer_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_encoder_mt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_flags_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_flags_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_flags_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/vli_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/vli_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/vli_size.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/delta/delta_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/delta/delta_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/delta/delta_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/fastpos_table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma2_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma2_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_encoder_optimum_fast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_encoder_optimum_normal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_encoder_presets.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lz/lz_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lz/lz_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lz/lz_encoder_mf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/rangecoder/price_table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/arm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/armthumb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/ia64.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/powerpc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/simple_coder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/simple_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/simple_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/sparc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/x86.c
)
set(LZMA_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/mythread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/sysdefs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/tuklib_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/tuklib_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/tuklib_cpucores.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/tuklib_integer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/tuklib_physmem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/base.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/bcj.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/block.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/check.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/container.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/delta.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/hardware.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/index.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/index_hash.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/lzma12.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/stream_flags.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/version.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/lzma/vli.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/check.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc32_table_be.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc32_table_le.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc64_table_be.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc64_table_le.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/check/crc_macros.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/alone_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_buffer_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/block_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/easy_preset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/filter_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/index.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/index_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/memcmplen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/outqueue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common/stream_flags_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/delta/delta_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/delta/delta_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/delta/delta_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/delta/delta_private.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/fastpos.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma2_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma2_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lzma/lzma_encoder_private.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lz/lz_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lz/lz_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lz/lz_encoder_hash.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/lz/lz_encoder_hash_table.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/rangecoder/price.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/rangecoder/range_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/rangecoder/range_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/rangecoder/range_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/simple_coder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/simple_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/simple_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/simple/simple_private.h
    ${CMAKE_CURRENT_SOURCE_DIR}/windows/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/vtklzma_mangle.h
)
vtk_add_library(vtklzma ${LZMA_HDRS} ${LZMA_SRCS})
set_target_properties(vtklzma PROPERTIES DEFINE_SYMBOL LZMA_DLL)

# Export lib files from dll files if using MSVC
if (MSVC)
    set_target_properties(vtklzma PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)
endif ()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/api/
        DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtklzma)
