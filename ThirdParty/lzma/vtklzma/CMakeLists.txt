# liblzma wrapped for VTK by Quincy Wofford and John Patchett,
# Los Alamos National Laboratory (2017).
# Thanks to Lasse Collin for integration help

if (FALSE) # XXX(kitware): hide subprojects.
project(LZMA VERSION 5.2.3 LANGUAGES C)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LZMA compression library")
endif ()

# XXX(kitware): avoid quoted variable policy warning from CMake.
IF(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
  SET(GNU_COMPATIBLE_COMPILER 1)
ENDIF()

include(TestBigEndian)
include(CheckIncludeFile)
include(CheckTypeSize)
include(ProcessorCount)

test_big_endian(WORDS_BIGENDIAN)

check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("byteswap.h" HAVE_BYTESWAP_H)
check_include_file("sys/endian.h" HAVE_SYS_ENDIAN_H)
if (HAVE_BYTESWAP_H)
  check_symbol_exists(HAVE_BSWAP_16 "byteswap.h" bswap_16)
  check_symbol_exists(HAVE_BSWAP_32 "byteswap.h" bswap_32)
  check_symbol_exists(HAVE_BSWAP_64 "byteswap.h" bswap_64)
endif ()

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
if (CMAKE_USE_WIN32_THREADS_INIT)
  set(MYTHREAD_VISTA 1)
elseif (CMAKE_USE_PTHREADS_INIT)
  set(MYTHREAD_POSIX 1)
else ()
  message(FATAL_ERROR
    "Unsupported threading backend for LZMA.")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h")
add_definitions(-DHAVE_CONFIG_H)

include_directories(
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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/common)

set(sources
  src/common/tuklib_cpucores.c
  src/common/tuklib_physmem.c
  src/liblzma/check/check.c
  src/liblzma/check/crc32_fast.c
  src/liblzma/check/crc32_table.c
  src/liblzma/check/crc64_fast.c
  src/liblzma/check/crc64_table.c
  src/liblzma/check/sha256.c
  src/liblzma/common/alone_decoder.c
  src/liblzma/common/alone_encoder.c
  src/liblzma/common/auto_decoder.c
  src/liblzma/common/block_buffer_decoder.c
  src/liblzma/common/block_buffer_encoder.c
  src/liblzma/common/block_decoder.c
  src/liblzma/common/block_encoder.c
  src/liblzma/common/block_header_decoder.c
  src/liblzma/common/block_header_encoder.c
  src/liblzma/common/block_util.c
  src/liblzma/common/common.c
  src/liblzma/common/easy_buffer_encoder.c
  src/liblzma/common/easy_decoder_memusage.c
  src/liblzma/common/easy_encoder.c
  src/liblzma/common/easy_encoder_memusage.c
  src/liblzma/common/easy_preset.c
  src/liblzma/common/filter_buffer_decoder.c
  src/liblzma/common/filter_buffer_encoder.c
  src/liblzma/common/filter_common.c
  src/liblzma/common/filter_decoder.c
  src/liblzma/common/filter_encoder.c
  src/liblzma/common/filter_flags_decoder.c
  src/liblzma/common/filter_flags_encoder.c
  src/liblzma/common/hardware_cputhreads.c
  src/liblzma/common/hardware_physmem.c
  src/liblzma/common/index.c
  src/liblzma/common/index_decoder.c
  src/liblzma/common/index_encoder.c
  src/liblzma/common/index_hash.c
  src/liblzma/common/outqueue.c
  src/liblzma/common/stream_buffer_decoder.c
  src/liblzma/common/stream_buffer_encoder.c
  src/liblzma/common/stream_decoder.c
  src/liblzma/common/stream_encoder.c
  src/liblzma/common/stream_encoder_mt.c
  src/liblzma/common/stream_flags_common.c
  src/liblzma/common/stream_flags_decoder.c
  src/liblzma/common/stream_flags_encoder.c
  src/liblzma/common/vli_decoder.c
  src/liblzma/common/vli_encoder.c
  src/liblzma/common/vli_size.c
  src/liblzma/delta/delta_common.c
  src/liblzma/delta/delta_decoder.c
  src/liblzma/delta/delta_encoder.c
  src/liblzma/lzma/fastpos_table.c
  src/liblzma/lzma/lzma2_decoder.c
  src/liblzma/lzma/lzma2_encoder.c
  src/liblzma/lzma/lzma_decoder.c
  src/liblzma/lzma/lzma_encoder.c
  src/liblzma/lzma/lzma_encoder_optimum_fast.c
  src/liblzma/lzma/lzma_encoder_optimum_normal.c
  src/liblzma/lzma/lzma_encoder_presets.c
  src/liblzma/lz/lz_decoder.c
  src/liblzma/lz/lz_encoder.c
  src/liblzma/lz/lz_encoder_mf.c
  src/liblzma/rangecoder/price_table.c
  src/liblzma/simple/arm.c
  src/liblzma/simple/armthumb.c
  src/liblzma/simple/ia64.c
  src/liblzma/simple/powerpc.c
  src/liblzma/simple/simple_coder.c
  src/liblzma/simple/simple_decoder.c
  src/liblzma/simple/simple_encoder.c
  src/liblzma/simple/sparc.c
  src/liblzma/simple/x86.c)

set(private_headers
  src/common/mythread.h
  src/common/sysdefs.h
  src/common/tuklib_common.h
  src/common/tuklib_config.h
  src/common/tuklib_cpucores.h
  src/common/tuklib_integer.h
  src/common/tuklib_physmem.h
  src/liblzma/check/check.h
  src/liblzma/check/crc32_table_be.h
  src/liblzma/check/crc32_table_le.h
  src/liblzma/check/crc64_table_be.h
  src/liblzma/check/crc64_table_le.h
  src/liblzma/check/crc_macros.h
  src/liblzma/common/alone_decoder.h
  src/liblzma/common/block_buffer_encoder.h
  src/liblzma/common/block_decoder.h
  src/liblzma/common/block_encoder.h
  src/liblzma/common/common.h
  src/liblzma/common/easy_preset.h
  src/liblzma/common/filter_common.h
  src/liblzma/common/filter_decoder.h
  src/liblzma/common/filter_encoder.h
  src/liblzma/common/index.h
  src/liblzma/common/index_encoder.h
  src/liblzma/common/memcmplen.h
  src/liblzma/common/outqueue.h
  src/liblzma/common/stream_decoder.h
  src/liblzma/common/stream_flags_common.h
  src/liblzma/delta/delta_common.h
  src/liblzma/delta/delta_decoder.h
  src/liblzma/delta/delta_encoder.h
  src/liblzma/delta/delta_private.h
  src/liblzma/lzma/fastpos.h
  src/liblzma/lzma/lzma2_decoder.h
  src/liblzma/lzma/lzma2_encoder.h
  src/liblzma/lzma/lzma_common.h
  src/liblzma/lzma/lzma_decoder.h
  src/liblzma/lzma/lzma_encoder.h
  src/liblzma/lzma/lzma_encoder_private.h
  src/liblzma/lz/lz_decoder.h
  src/liblzma/lz/lz_encoder.h
  src/liblzma/lz/lz_encoder_hash.h
  src/liblzma/lz/lz_encoder_hash_table.h
  src/liblzma/rangecoder/price.h
  src/liblzma/rangecoder/range_common.h
  src/liblzma/rangecoder/range_decoder.h
  src/liblzma/rangecoder/range_encoder.h
  src/liblzma/simple/simple_coder.h
  src/liblzma/simple/simple_decoder.h
  src/liblzma/simple/simple_encoder.h
  src/liblzma/simple/simple_private.h)

set(headers
  src/liblzma/api/lzma.h
  src/liblzma/api/lzma/base.h
  src/liblzma/api/lzma/bcj.h
  src/liblzma/api/lzma/block.h
  src/liblzma/api/lzma/check.h
  src/liblzma/api/lzma/container.h
  src/liblzma/api/lzma/delta.h
  src/liblzma/api/lzma/filter.h
  src/liblzma/api/lzma/hardware.h
  src/liblzma/api/lzma/index.h
  src/liblzma/api/lzma/index_hash.h
  src/liblzma/api/lzma/lzma12.h
  src/liblzma/api/lzma/stream_flags.h
  src/liblzma/api/lzma/version.h
  src/liblzma/api/lzma/vli.h
  src/liblzma/api/vtk_lzma_mangle.h)

vtk_module_add_module(VTK::lzma
  SOURCES ${sources}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  HEADERS_SUBDIR "vtklzma/src/liblzma/api")
vtk_module_install_headers(
  DIRECTORIES "src/liblzma/api/lzma"
  SUBDIR      "vtklzma/src/liblzma/api")
set_target_properties(lzma
  PROPERTIES
    C_STANDARD 99
    POSITION_INDEPENDENT_CODE 1)

if (BUILD_SHARED_LIBS)
  set_target_properties(lzma
    PROPERTIES
      DEFINE_SYMBOL DLL_EXPORT)
  if (CMAKE_USE_PTHREADS_INIT)
    target_link_libraries(lzma
      PRIVATE
        ${CMAKE_THREAD_LIBS_INIT})
  endif ()
endif ()
