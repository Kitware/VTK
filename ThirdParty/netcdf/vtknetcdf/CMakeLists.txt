set(NC_VERSION_MAJOR 4)
set(NC_VERSION_MINOR 9)
set(NC_VERSION_PATCH 3)
set(NC_VERSION_NOTE "")
set(netCDF_VERSION ${NC_VERSION_MAJOR}.${NC_VERSION_MINOR}.${NC_VERSION_PATCH}${NC_VERSION_NOTE})
set(PACKAGE_VERSION "${netCDF_VERSION}")
set(VERSION ${netCDF_VERSION})
set(NC_VERSION ${netCDF_VERSION})
set(NC_DISPATCH_VERSION 5)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)

test_big_endian(WORDS_BIGENDIAN)

set(DEFAULT_CHUNKS_IN_CACHE 10)
set(DEFAULT_CHUNK_SIZE 4194304)
set(DEFAULT_CHUNK_CACHE_SIZE 67108864U)
set(DEFAULT_CHUNK_CACHE_PREEMPTION 0.75)

set(CHUNK_CACHE_SIZE 4194304)
set(CHUNK_CACHE_NELEMS 1009)
set(CHUNK_CACHE_PREEMPTION 0.75)

set(NCIO_MINBLOCKSIZE 256)

# Library include checks
check_include_file("math.h"      HAVE_MATH_H)
check_include_file("unistd.h"    HAVE_UNISTD_H)
# Solve a compatibility issue in ncgen/, which checks
# for NO_UNISTD_H
if (NOT HAVE_UNISTD_H)
  set(YY_NO_UNISTD_H TRUE)
endif()

check_include_file("alloca.h"  HAVE_ALLOCA_H)
check_include_file("malloc.h"  HAVE_MALLOC_H)
check_include_file("fcntl.h"   HAVE_FCNTL_H)
check_include_file("getopt.h"  HAVE_GETOPT_H)
check_include_file("locale.h"  HAVE_LOCALE_H)
check_include_file("stdint.h"  HAVE_STDINT_H)
check_include_file("stdio.h"   HAVE_STDIO_H)
if (MSVC)
check_include_file("io.h"      HAVE_IO_H)
endif ()
check_include_file("stdlib.h"  HAVE_STDLIB_H)
check_include_file("ctype.h"   HAVE_CTYPE_H)
check_include_file("sys/xattr_h" HAVE_SYS_XATTR_H)
check_include_file("stdarg.h"    HAVE_STDARG_H)
check_include_file("strings.h"   HAVE_STRINGS_H)
check_include_file("signal.h"    HAVE_SIGNAL_H)
check_include_file("sys/param.h" HAVE_SYS_PARAM_H)
check_include_file("sys/stat.h"  HAVE_SYS_STAT_H)
check_include_file("sys/time.h"  HAVE_SYS_TIME_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/mman.h"  HAVE_SYS_MMAN_H)
check_include_file("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_file("fcntl.h"  HAVE_FCNTL_H)
check_include_file("inttypes.h"  HAVE_INTTYPES_H)
check_include_file("pstdint.h"  HAVE_PSTDINT_H)
check_include_file("endian.h" HAVE_ENDIAN_H)
check_include_file("BaseTsd.h"  HAVE_BASETSD_H)
check_include_file("stddef.h"   HAVE_STDDEF_H)
check_include_file("string.h"  HAVE_STRING_H)
check_include_file("winsock2.h" HAVE_WINSOCK2_H)
check_include_file("ftw.h"  HAVE_FTW_H)
check_include_file("libgen.h" HAVE_LIBGEN_H)
check_include_file("execinfo.h" HAVE_EXECINFO_H)
check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("time.h" HAVE_TIME_H)
check_include_file("dlfcn.h" HAVE_DLFCN_H)

# Symbol Exists
check_symbol_exists(isfinite "math.h" HAVE_DECL_ISFINITE)
check_symbol_exists(isnan "math.h" HAVE_DECL_ISNAN)
check_symbol_exists(isinf "math.h" HAVE_DECL_ISINF)
check_symbol_exists(st_blksize "sys/stat.h" HAVE_STRUCT_STAT_ST_BLKSIZE)
check_symbol_exists(alloca "alloca.h" HAVE_ALLOCA)
check_symbol_exists(snprintf "stdio.h" HAVE_SNPRINTF)

# Check for various functions.
check_function_exists(fsync HAVE_FSYNC)
check_function_exists(strlcat   HAVE_STRLCAT)
check_function_exists(strlcpy  HAVE_STRLCPY)
check_function_exists(strdup  HAVE_STRDUP)
check_function_exists(strndup HAVE_STRNDUP)
check_function_exists(strlen  HAVE_STRLEN)
check_function_exists(strtoll HAVE_STRTOLL)
check_function_exists(strcasecmp  HAVE_STRCASECMP)
check_function_exists(strtoull  HAVE_STRTOULL)
check_function_exists(mkstemp HAVE_MKSTEMP)
check_function_exists(mktemp HAVE_MKTEMP)
check_function_exists(random HAVE_RANDOM)
check_function_exists(gettimeofday  HAVE_GETTIMEOFDAY)
check_function_exists(MPI_Comm_f2c  HAVE_MPI_COMM_F2C)
check_function_exists(MPI_Info_f2c  HAVE_MPI_INFO_F2C)
check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(getpagesize HAVE_GETPAGESIZE)
check_function_exists(sysconf HAVE_SYSCONF)
check_function_exists(getrlimit HAVE_GETRLIMIT)
check_function_exists(_filelengthi64 HAVE_FILE_LENGTH_I64)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(mremap HAVE_MREMAP)
check_function_exists(fileno HAVE_FILENO)
check_function_exists(H5Literate2 HAVE_H5LITERATE2)

check_function_exists(clock_gettime  HAVE_CLOCK_GETTIME)
check_symbol_exists("struct timespec" "time.h" HAVE_STRUCT_TIMESPEC)
check_function_exists(atexit HAVE_ATEXIT)

set(SIZEOF_VOIDSTAR ${CMAKE_SIZEOF_VOID_P})
set(SIZEOF_VOIDP ${SIZEOF_VOIDSTAR})
check_type_size("char"      SIZEOF_CHAR)
check_type_size("double"    SIZEOF_DOUBLE)
check_type_size("float"     SIZEOF_FLOAT)
check_type_size("int"       SIZEOF_INT)
check_type_size("uint"      SIZEOF_UINT)
if (SIZEOF_UINT)
  set(HAVE_UINT TRUE)
endif ()
check_type_size("schar" SIZEOF_SCHAR)
if (SIZEOF_SCHAR)
  set(HAVE_SCHAR TRUE)
endif ()
check_type_size("long"      SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
if (SIZEOF_LONG_LONG)
  set(HAVE_LONG_LONG_INT TRUE)
endif ()
check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)
check_type_size("off_t"     SIZEOF_OFF_T)
check_type_size("off64_t"   SIZEOF_OFF64_T)
check_type_size("short"     SIZEOF_SHORT)
check_type_size("ushort"    SIZEOF_USHORT)
if (SIZEOF_USHORT)
  set(HAVE_USHORT TRUE)
endif ()
check_type_size("_Bool"     SIZEOF__BOOL)
check_type_size("size_t"    SIZEOF_SIZE_T)

check_type_size("ssize_t"   SIZEOF_SSIZE_T)
if (SIZEOF_SSIZE_T)
  set(HAVE_SSIZE_T TRUE)
endif ()
check_type_size("ptrdiff_t" SIZEOF_PTRDIFF_T)
if (SIZEOF_PTRDIFF_T)
  set(HAVE_PTRDIFF_T TRUE)
endif ()
check_type_size("uintptr_t" SIZEOF_UINTPTR_T)
if (SIZEOF_UINTPTR_T)
  set(HAVE_UINTPTR_T TRUE)
endif ()
check_type_size("mode_t" SIZEOF_MODE_T)
if (SIZEOF_MODE_T)
  set(HAVE_MODE_T TRUE)
endif ()

check_type_size("__int64"   SIZEOF___INT_64)
check_type_size("int64_t"   SIZEOF_INT64_T)
check_type_size("uint64"  SIZEOF_UINT64)
check_type_size("unsigned char"      SIZEOF_UCHAR)
check_type_size("unsigned short int" SIZEOF_UNSIGNED_SHORT_INT)
check_type_size("unsigned int"       SIZEOF_UNSIGNED_INT)
check_type_size("long long"          SIZEOF_LONGLONG)
check_type_size("unsigned long long" SIZEOF_ULONGLONG)

check_type_size("uint64_t"  SIZEOF_UINT64_T)
if (SIZEOF_UINT64_T)
  set(HAVE_UINT64_T TRUE)
endif ()

# On windows systems, we redefine off_t as __int64
# to enable LFS. This is true on 32 and 64 bit system.s
# We must redefine SIZEOF_OFF_T to match.
if (MSVC AND SIZEOF___INT_64)
  set(SIZEOF_OFF_T  ${SIZEOF___INT_64})
endif()

# NETCDF_ENABLE features
set(NETCDF_ENABLE_V2_API ON)
set(NETCDF_ENABLE_MMAP ON)
set(NETCDF_ENABLE_NETCDF_4 ON)
set(NETCDF_ENABLE_HDF5 ON)
set(NETCDF_ENABLE_NETCDF4 ON)
set(NETCDF_ENABLE_REMOTE_FUNCTIONALITY OFF)
set(NETCDF_ENABLE_DAP OFF)
set(NETCDF_ENABLE_DAP2 OFF)
set(NETCDF_ENABLE_DAP4 OFF)
set(NETCDF_ENABLE_NCZARR OFF)
set(NETCDF_ENABLE_PNETCDF OFF)
set(NETCDF_ENABLE_CDF5 AUTO)
set(NETCDF_ENABLE_HDF4 OFF)
set(NETCDF_ENABLE_LEGACY_MACROS ON)
set(NETCDF_ENABLE_LOGGING OFF)
set(NETCDF_ENABLE_STRICT_NULL_BYTE_HEADER_PADDING OFF)
set(NETCDF_ENABLE_BYTERANGE ${NETCDF_ENABLE_DAP})
set(NETCDF_ENABLE_PLUGINS OFF)
set(NETCDF_ENABLE_FILTER_SZIP OFF)
set(NETCDF_ENABLE_FILTER_BZ2 OFF)
set(NETCDF_ENABLE_FILTER_BLOSC OFF)
set(NETCDF_ENABLE_FILTER_ZSTD OFF)
if (DEFINED CMAKE_INSTALL_PREFIX) # Table row 2
    set(DEFAULT_PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/hdf5/lib/plugin")
    set(DEFAULT_PLUGIN_SEARCH_PATH "${DEFAULT_PLUGIN_INSTALL_DIR}${PLATFORMSEP}${PLATFORMDEFAULT}")
else() # Table row 1
    unset(DEFAULT_PLUGIN_INSTALL_DIR)
    set(DEFAULT_PLUGIN_SEARCH_PATH "${PLATFORMDEFAULT}")
endif()
# canonical form is all forward slashes
string(REPLACE "\\" "/" DEFAULT_PLUGIN_INSTALL_DIR "${DEFAULT_PLUGIN_INSTALL_DIR}")
string(REPLACE "\\" "/" DEFAULT_PLUGIN_SEARCH_PATH "${DEFAULT_PLUGIN_SEARCH_PATH}")
# Inferences
if (DEFINED DEFAULT_PLUGIN_INSTALL_DIR)
    set(ENABLE_PLUGIN_DIR yes)
else()
    set(ENABLE_PLUGIN_DIR no)
endif()
set(NETCDF_PLUGIN_INSTALL_DIR "${DEFAULT_PLUGIN_INSTALL_DIR}")
set(NETCDF_PLUGIN_SEARCH_PATH "${DEFAULT_PLUGIN_SEARCH_PATH}")
set(NETCDF_ENABLE_NCZARR_ZIP ${NETCDF_ENABLE_NCZARR})
if (MSVC)
  set(NETCDF_ENABLE_XGETOPT ON)
endif ()
if (NOT WIN32)
  set(NETCDF_ENABLE_STDIO OFF)
  set(NETCDF_ENABLE_FFIO OFF)
endif ()
set(NETCDF_ENABLE_S3 OFF)
set(NETCDF_ENABLE_S3_INTERNAL OFF)
set(NETCDF_ENABLE_S3_AWS OFF)
set(NETCDF_ENABLE_NCZARR_S3 OFF)
set(NETCDF_ENABLE_LIBXML2 OFF)
set(NETCDF_ENABLE_FSYNC OFF)
set(NETCDF_ENABLE_PARALLEL4 OFF)
set(NETCDF_ENABLE_ERANGE_FILL OFF)
set(NETCDF_ENABLE_NCZARR_FILTERS OFF)
set(NETCDF_ENABLE_DISKLESS ON)
set(NETCDF_ENABLE_QUANTIZE ON)
set(ENABLE_ZERO_LENGTH_COORD_BOUND OFF)

# set the feature flags based on the NETCDF_ENABLE options
if (NETCDF_ENABLE_V2_API)
  set(NO_NETCDF_2 OFF)
  set(USE_NETCDF_2 TRUE)
endif ()
set(BUILD_V2 ${NETCDF_ENABLE_V2_API})
set(USE_HDF5 ${NETCDF_ENABLE_HDF5})
if (MSVC)
  set(NETCDF_ENABLE_MMAP OFF)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(NETCDF_ENABLE_MMAP OFF)
else ()
  check_c_source_compiles("
  #include <sys/mman.h>
  int main() {int x = MAP_ANONYMOUS;}" HAVE_MAPANON)
  if (NOT HAVE_MMAP OR NOT HAVE_MAPANON)
    set(NETCDF_ENABLE_MMAP OFF)
  endif()
endif ()
if (NETCDF_ENABLE_MMAP)
  # Aliases
  set(BUILD_MMAP ON)
  set(USE_MMAP ON)
endif ()
if (NETCDF_ENABLE_NETCDF4)
  set(USE_NETCDF4 ON)
  set(USE_HDF5 ON)
else ()
  set(USE_NETCDF4 OFF)
  set(USE_HDF5 OFF)
endif ()
if (NETCDF_ENABLE_DAP)
  set(USE_DAP ON)
endif ()
set(STATUS_PNETCDF ${NETCDF_ENABLE_PNETCDF})
set(USE_PNETCDF ${NETCDF_ENABLE_PNETCDF})
if (NETCDF_ENABLE_PNETCDF)
  set(STATUS_PARALLEL ON)
  set(USE_PARALLEL ON)
endif ()
if (SIZEOF_SIZE_T EQUAL 4)
  if (NETCDF_ENABLE_CDF5) # enable or auto
    string(TOUPPER ${NETCDF_ENABLE_CDF5} NETCDF_ENABLE_CDF5)
    if (NETCDF_ENABLE_CDF5 AND NOT NETCDF_ENABLE_CDF5 STREQUAL "AUTO") # explicitly enabled
      message(STATUS "Unable to support CDF5 feature because size_t is less than 8 bytes")
    endif ()
    set(NETCDF_ENABLE_CDF5 OFF) # cannot support CDF5
    set(USE_CDF5 OFF) # cannot support CDF5
  endif ()
else ()
  if (NETCDF_ENABLE_CDF5) # explicitly set by user or not set
    set(USE_CDF5 ON)
  else () # explicitly disabled by user
    set(USE_CDF5 OFF)
  endif ()
endif ()
if(NETCDF_ENABLE_HDF4)
  set(USE_HDF4 ON)
endif()
set(LOGGING ${NETCDF_ENABLE_LOGGING})
set(NETCDF_ENABLE_SET_LOG_LEVEL ${NETCDF_ENABLE_LOGGING})
set(USE_STRICT_NULL_BYTE_HEADER_PADDING ${NETCDF_ENABLE_STRICT_NULL_BYTE_HEADER_PADDING})
if(NETCDF_ENABLE_PLUGINS)
  set(USEPLUGINS yes)
endif()
if (MSVC)
  if (NETCDF_ENABLE_XGETOPT)
    set(USE_X_GETOPT ON)
  endif ()
endif ()
if (NOT WIN32)
  if (NETCDF_ENABLE_STDIO)
    set(USE_STDIO ON)
  endif ()
  if (NETCDF_ENABLE_FFIO)
    set(USE_FFIO ON)
  endif ()
endif ()
if (NETCDF_ENABLE_FSYNC)
  set(USE_FSYNC ON)
endif ()
set(HAVE_HDF5_H 1)
set(H5_USE_16_API 1) # Enable HDF5 1.6.x Compatibility(Required)
if (VTK_MODULE_USE_EXTERNAL_VTK_hdf5)
  set(CMAKE_REQUIRED_INCLUDES_save "${CMAKE_REQUIRED_INCLUDES}")
  list(APPEND CMAKE_REQUIRED_INCLUDES
    "${CMAKE_CURRENT_BINARY_DIR}/../../hdf5")
  # The symbol detections can still fail for an external HDF5 that isn't in a
  # standard location. We need generate-time knowledge to get the real include
  # directories. These checks should really be replaced at some point with some
  # compile-time logic if at all possible (version checks, preprocessor
  # detections, etc.).
  check_symbol_exists("H5free_memory" "vtk_hdf5.h" HDF5_HAS_H5FREE)
  check_symbol_exists("H5Pset_all_coll_metadata_ops" "vtk_hdf5.h" H5PSET_ALL_COLL_METADATA_OPS)
  check_symbol_exists("H5Pset_libver_bounds" "vtk_hdf5.h" HDF5_HAS_LIBVER_BOUNDS)
  check_symbol_exists("HDF5_PARALLEL" "vtk_hdf5.h" HDF5_PARALLEL)
  set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES_save}")
else ()
  set(HDF5_HAS_H5FREE 1)
  set(H5PSET_ALL_COLL_METADATA_OPS 1)
  set(HDF5_HAS_LIBVER_BOUNDS 1)
  set(HDF5_PARALLEL 0)
endif ()
if (NETCDF_ENABLE_PARALLEL4 AND NETCDF_ENABLE_HDF5)
  if (NOT HDF5_PARALLEL)
    set(USE_PARALLEL4 OFF)
  else ()
    set(HDF5_PARALLEL ON)
    set(USE_PARALLEL ON)
    set(USE_PARALLEL4 ON)
  endif ()
endif ()
if (NETCDF_ENABLE_DISKLESS)
  set(USE_DISKLESS ON)
endif ()

# Set the source files for the library
set(libdispatch_sources
  libdispatch/datt.c
  libdispatch/dattget.c
  libdispatch/dattinq.c
  libdispatch/dattput.c
  libdispatch/dauth.c
  libdispatch/daux.c
  libdispatch/dcompound.c
  libdispatch/dcopy.c
  libdispatch/dcrc32.c
  libdispatch/dcrc64.c
  libdispatch/ddim.c
  libdispatch/ddispatch.c
  #libdispatch/defcheck.c       # test file with main function
  libdispatch/denum.c
  libdispatch/derror.c
  libdispatch/dfile.c
  libdispatch/dfilter.c
  libdispatch/dgroup.c
  libdispatch/dinfermodel.c
  libdispatch/dinstance.c
  libdispatch/dinstance_intern.c
  libdispatch/dinternal.c
  libdispatch/dmissing.c
  libdispatch/dnotnc3.c
  libdispatch/dnotnc4.c
  libdispatch/doffsets.c
  libdispatch/dopaque.c
  libdispatch/dparallel.c
  libdispatch/dpathmgr.c
  libdispatch/dplugins.c
  libdispatch/drc.c
  libdispatch/dreadonly.c
  libdispatch/ds3util.c
  libdispatch/dstring.c
  libdispatch/dtype.c
  #libdispatch/durlmodel.c      # not used by netCDF
  libdispatch/dutf8.c
  libdispatch/dutil.c
  libdispatch/dvar.c
  libdispatch/dvarget.c
  libdispatch/dvarinq.c
  libdispatch/dvarput.c
  libdispatch/dvlen.c
  #libdispatch/json.c           # not used by netCDF
  libdispatch/nc.c
  libdispatch/ncbytes.c
  libdispatch/ncexhash.c
  libdispatch/nchashmap.c
  libdispatch/ncjson.c
  libdispatch/nclist.c
  libdispatch/nclistmgr.c
  libdispatch/nclog.c
  libdispatch/ncproplist.c
  # libdispatch/ncrandom.c      # test file with main function
  libdispatch/nctime.c
  libdispatch/ncuri.c
  libdispatch/ncxcache.c
  libdispatch/stubdef.c
  #libdispatch/u8.c             # not used by netCDF
  libdispatch/utf8proc.c
  libdispatch/utf8proc.h
  libdispatch/utf8proc_data.c
)

if (NETCDF_ENABLE_BYTERANGE)
  list(APPEND libdispatch_sources
    libdispatch/dhttp.c
  )
endif ()

if (BUILD_V2)
  list(APPEND libdispatch_sources
    libdispatch/dv2i.c
  )
endif ()

if (NETCDF_ENABLE_S3 AND NETCDF_ENABLE_S3_INTERNAL)
  list(APPEND libdispatch_sources
    libdispatch/nccurl_hmac.c
    libdispatch/nccurl_sha256.c
    libdispatch/nch5s3comms.c
    libdispatch/ncs3sdk_h5.c
  )
endif ()

if (WIN32)
  list(APPEND libdispatch_sources
    libdispatch/dreg.c
  )
endif ()

if (USE_X_GETOPT)
  list(APPEND libdispatch_sources
    libdispatch/XGetopt.c
  )
endif ()

set(liblib_sources
  liblib/nc_initialize.c
)

set(libsrc_sources
  libsrc/attr.c
  libsrc/dim.c
  libsrc/lookup3.c
  libsrc/memio.c
  libsrc/nc3dispatch.c
  libsrc/nc3internal.c
  #libsrc/ncFile.c              # not used by netCDF
  libsrc/ncio.c
  libsrc/ncx.c
  #libsrc/ncx_cray.c            # not used by netCDF
  libsrc/putget.c
  #libsrc/t_ncio.c              # not used by netCDF
  libsrc/v1hpg.c
  libsrc/var.c
  #libsrc/winceio.c             # not used by netCDF
)

if (NETCDF_ENABLE_S3 AND NETCDF_ENABLE_S3_INTERNAL)
  list(APPEND libsrc_sources
    libsrc/s3io.c
  )
endif ()

if (BUILD_MMAP)
  list(APPEND libsrc_sources
    libsrc/mmapio.c
  )
endif ()

if (USE_FFIO)
  list(APPEND libsrc_sources libsrc/ffio.c)
elseif (USE_STDIO)
  list(APPEND libsrc_sources libsrc/ncstdio.c)
else ()
  list(APPEND libsrc_sources libsrc/posixio.c)
endif ()

if (USE_NETCDF4)
  set(libsrc4_sources
    libsrc4/nc4attr.c
    libsrc4/nc4cache.c
    libsrc4/nc4dim.c
    libsrc4/nc4dispatch.c
    libsrc4/nc4filters.c
    libsrc4/nc4grp.c
    libsrc4/nc4internal.c
    libsrc4/nc4type.c
    libsrc4/nc4var.c
    libsrc4/ncfunc.c
    libsrc4/ncindex.c
  )
endif ()

if (USE_HDF5)
  set(libhdf5_sources
    libhdf5/hdf5attr.c
    libhdf5/hdf5create.c
    libhdf5/hdf5debug.c
    libhdf5/hdf5dim.c
    libhdf5/hdf5dispatch.c
    libhdf5/hdf5file.c
    libhdf5/hdf5filter.c
    libhdf5/hdf5grp.c
    libhdf5/hdf5internal.c
    libhdf5/hdf5open.c
    libhdf5/hdf5plugins.c
    libhdf5/hdf5set_format_compatibility.c
    libhdf5/hdf5type.c
    libhdf5/hdf5var.c
    libhdf5/nc4hdf.c
    libhdf5/nc4info.c
    libhdf5/nc4mem.c
    libhdf5/nc4memcb.c
    )

  if (NETCDF_ENABLE_BYTERANGE)
    list(APPEND libhdf5_sources
      libhdf5/H5FDhttp.c
    )
  endif ()
endif ()

function(is_enabled feature ret_val)
  if(${feature})
    set("NC_${ret_val}" 1 PARENT_SCOPE)
  else()
    set("NC_${ret_val}" 0 PARENT_SCOPE)
  endif()
endfunction()

is_enabled(NETCDF_ENABLE_V2_API HAS_NC2)
is_enabled(NETCDF_ENABLE_NETCDF4 HAS_NC4)
is_enabled(NETCDF_ENABLE_HDF4 HAS_HDF4)
is_enabled(USE_HDF4 HAS_HDF4)
is_enabled(USE_HDF5 HAS_HDF5)
is_enabled(OFF HAS_BENCHMARKS)
is_enabled(STATUS_PNETCDF HAS_PNETCDF)
is_enabled(STATUS_PARALLEL HAS_PARALLEL)
is_enabled(NETCDF_ENABLE_PARALLEL4 HAS_PARALLEL4)
is_enabled(NETCDF_ENABLE_DAP HAS_DAP)
is_enabled(NETCDF_ENABLE_DAP2 HAS_DAP2)
is_enabled(NETCDF_ENABLE_DAP4 HAS_DAP4)
is_enabled(NETCDF_ENABLE_LEGACY_MACROS HAS_LEGACY_MACROS)
is_enabled(NETCDF_ENABLE_BYTERANGE HAS_BYTERANGE)
is_enabled(NETCDF_ENABLE_DISKLESS HAS_DISKLESS)
is_enabled(USE_MMAP HAS_MMAP)
is_enabled(ENABLE_ZERO_LENGTH_COORD_BOUND RELAX_COORD_BOUND)
is_enabled(USE_CDF5 HAS_CDF5)
is_enabled(NETCDF_ENABLE_ERANGE_FILL HAS_ERANGE_FILL)
is_enabled(HDF5_HAS_PAR_FILTERS HAS_PAR_FILTERS)
is_enabled(NETCDF_ENABLE_S3 HAS_S3)
is_enabled(NETCDF_ENABLE_S3_AWS HAS_S3_AWS)
is_enabled(NETCDF_ENABLE_S3_INTERNAL HAS_S3_INTERNAL)
is_enabled(HAS_HDF5_ROS3 HAS_HDF5_ROS3)
is_enabled(NETCDF_ENABLE_NCZARR HAS_NCZARR)
is_enabled(NETCDF_ENABLE_NCZARR_ZIP HAS_NCZARR_ZIP)
is_enabled(NETCDF_ENABLE_PLUGINS HAS_PLUGINS)
is_enabled(NETCDF_ENABLE_QUANTIZE HAS_QUANTIZE)
is_enabled(NETCDF_ENABLE_LOGGING HAS_LOGGING)
is_enabled(NETCDF_ENABLE_FILTER_TESTING DO_FILTER_TESTS)
is_enabled(HAVE_SZ HAS_SZIP)
is_enabled(HAVE_SZ HAS_SZLIB_WRITE)
is_enabled(HAVE_ZSTD HAS_ZSTD)
is_enabled(HAVE_BLOSC HAS_BLOSC)
is_enabled(HAVE_BZ2 HAS_BZ2)
is_enabled(NETCDF_ENABLE_REMOTE_FUNCTIONALITY DO_REMOTE_FUNCTIONALITY)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/netcdf_meta.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/netcdf_meta.h"
  @ONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/netcdf_dispatch.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/netcdf_dispatch.h"
  @ONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_netcdf_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdf_config.h"
  @ONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
  @ONLY)

set(headers
  include/netcdf.h
  include/netcdf_filter_build.h
  include/vtk_netcdf_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/include/netcdf_dispatch.h"
  "${CMAKE_CURRENT_BINARY_DIR}/include/netcdf_meta.h"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdf_config.h")

vtk_module_add_module(VTK::netcdf
  HEADER_DIRECTORIES
  SOURCES ${libdispatch_sources}
          ${liblib_sources}
          ${libsrc_sources}
          ${libsrc4_sources}
          ${libhdf5_sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtknetcdf")
vtk_module_definitions(VTK::netcdf
  PRIVATE
    HAVE_CONFIG_H)
if (WIN32 AND BUILD_SHARED_LIBS)
  vtk_module_definitions(VTK::netcdf
    PRIVATE
      DLL_EXPORT
      DLL_NETCDF)
endif ()
vtk_module_include(VTK::netcdf
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libdispatch"
    "${CMAKE_CURRENT_SOURCE_DIR}/libsrc"
    "${CMAKE_CURRENT_SOURCE_DIR}/libsrc4"
    "${CMAKE_CURRENT_BINARY_DIR}/include")
if (HDF5_PARALLEL)
  if (NOT TARGET VTK::mpi)
    message(FATAL_ERROR
      "HDF5 is parallel, but VTK's MPI support was not available; this is not "
      "supported.")
  endif ()
  vtk_module_link(VTK::netcdf
    INTERFACE
      VTK::mpi)
endif ()

if (UNIX)
  # link against the math library.
  vtk_module_link(VTK::netcdf
    PRIVATE
      m)
endif ()
