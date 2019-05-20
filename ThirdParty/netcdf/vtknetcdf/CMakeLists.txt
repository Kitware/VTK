set(NC_VERSION_MAJOR 4)
set(NC_VERSION_MINOR 5)
set(NC_VERSION_PATCH 0)
set(NC_VERSION_NOTE "")
set(netCDF_VERSION ${NC_VERSION_MAJOR}.${NC_VERSION_MINOR}.${NC_VERSION_PATCH}${NC_VERSION_NOTE})
set(VERSION ${netCDF_VERSION})
set(NC_VERSION ${netCDF_VERSION})

set(sources
  libdispatch/datt.c
  libdispatch/dattget.c
  libdispatch/dattinq.c
  libdispatch/dattput.c
  libdispatch/dcompound.c
  libdispatch/dcopy.c
  libdispatch/ddim.c
  libdispatch/ddispatch.c
  libdispatch/denum.c
  libdispatch/derror.c
  libdispatch/dfile.c
  libdispatch/dgroup.c
  libdispatch/dinternal.c
  libdispatch/doffsets.c
  libdispatch/dopaque.c
  libdispatch/dparallel.c
  libdispatch/drc.c
  libdispatch/dstring.c
  libdispatch/dtype.c
  libdispatch/dutf8.c
  libdispatch/dvar.c
  libdispatch/dvarget.c
  libdispatch/dvarinq.c
  libdispatch/dvarput.c
  libdispatch/dvlen.c
  libdispatch/dwinpath.c
  libdispatch/nc.c
  libdispatch/ncaux.c
  libdispatch/ncbytes.c
  libdispatch/nchashmap.c
  libdispatch/nclist.c
  libdispatch/nclistmgr.c
  libdispatch/nclog.c
  libdispatch/nctime.c
  libdispatch/ncuri.c
  libdispatch/utf8proc.c

  liblib/nc_initialize.c

  libsrc/attr.c
  libsrc/dim.c
  libsrc/lookup3.c
  libsrc/memio.c
  libsrc/nc3dispatch.c
  libsrc/nc3internal.c
  libsrc/nc_hashmap.c
  libsrc/ncio.c
  libsrc/ncx.c
  libsrc/posixio.c
  libsrc/putget.c
  libsrc/v1hpg.c
  libsrc/var.c

  libsrc4/error4.c
  libsrc4/nc4attr.c
  libsrc4/nc4dim.c
  libsrc4/nc4dispatch.c
  libsrc4/nc4file.c
  libsrc4/nc4grp.c
  libsrc4/nc4hdf.c
  libsrc4/nc4info.c
  libsrc4/nc4internal.c
  libsrc4/nc4printer.c
  libsrc4/nc4type.c
  libsrc4/nc4var.c
  libsrc4/ncfunc.c)

set(CHUNK_CACHE_NELEMS 1009)
set(CHUNK_CACHE_PREEMPTION 0.75)
set(CHUNK_CACHE_SIZE 4194304)
set(DEFAULT_CHUNKS_IN_CACHE 10)
set(DEFAULT_CHUNK_SIZE 4194304)
set(MAX_DEFAULT_CACHE_SIZE 67108864)
set(NCIO_MINBLOCKSIZE 256)

set(JNA 1)
set(NO_NETCDF_2 1)
set(RELAX_COORD_BOUND 0)
set(USE_CDF4 0)
set(USE_DAP 0)
set(USE_DISKLESS 1)
set(USE_FFIO 0)
set(USE_FSYNC 0)
set(USE_HDF4 0)
set(USE_MMAP 0)
set(USE_NETCDF4 1)
set(USE_NETCDF_2 0)
set(USE_PARALLEL 0)
set(USE_PARALLEL4 0)
set(USE_PARALLEL_MPIO 0)
set(USE_PARALLEL_POSIX 0)
set(USE_PNETCDF 0)
set(USE_SZIP 0)
set(USE_ZLIB 0)

function(is_enabled feature ret_val)
  if(${feature})
    set("NC_${ret_val}" 1 PARENT_SCOPE)
  else()
    set("NC_${ret_val}" 0 PARENT_SCOPE)
  endif()
endfunction()

is_enabled(ENABLE_V2_API HAS_NC2)
is_enabled(ENABLE_NETCDF_4 HAS_NC4)
is_enabled(ENABLE_HDF4 HAS_HDF4)
is_enabled(ENABLE_NETCDF_4 HAS_HDF5)
is_enabled(USE_SZIP HAS_SZIP)
is_enabled(STATUS_PNETCDF HAS_PNETCDF)
is_enabled(STATUS_PARALLEL HAS_PARALLEL)
is_enabled(ENABLE_PARALLEL4 HAS_PARALLEL4)
is_enabled(ENABLE_DAP HAS_DAP)
is_enabled(ENABLE_DAP HAS_DAP2)
is_enabled(ENABLE_DAP4 HAS_DAP4)
is_enabled(USE_DISKLESS HAS_DISKLESS)
is_enabled(USE_MMAP HAS_MMAP)
is_enabled(JNA HAS_JNA)
is_enabled(STATUS_RELAX_COORD_BOUND RELAX_COORD_BOUND)
is_enabled(USE_CDF5 HAS_CDF5)

set(USE_STDIO 0)

include(CheckSymbolExists)
check_symbol_exists("_filelengthi64" "io.h" HAVE_FILE_LENGTH_I64)
check_symbol_exists("alloca" "alloca.h" HAVE_ALLOCA)
check_symbol_exists("isfinite" "math.h" HAVE_DECL_ISFINITE)
check_symbol_exists("isinf" "math.h" HAVE_DECL_ISINF)
check_symbol_exists("isnan" "math.h" HAVE_DECL_ISNAN)
check_symbol_exists("fsync" "unistd.h" HAVE_FSYNC)
check_symbol_exists("getpagesize" "unistd.h" HAVE_GETPAGESIZE)
check_symbol_exists("getrlimit" "sys/time.h;sys/resource.h" HAVE_GETRLIMIT)
check_symbol_exists("memmove" "string.h" HAVE_MEMMOVE)
check_symbol_exists("random" "stdlib.h" HAVE_RANDOM)
check_symbol_exists("snprintf" "stdio.h" HAVE_SNPRINTF)
check_symbol_exists("strdup" "string.h" HAVE_STRDUP)
check_symbol_exists("strlcat" "string.h" HAVE_STRLCAT)
check_symbol_exists("st_blocksize" "sys/stat.h" HAVE_STRUCT_STAT_ST_BLKSIZE)
check_symbol_exists("sysconf" "unistd.h" HAVE_SYSCONF)

set(HAVE_HDF5_H 1)
check_symbol_exists("H5free_memory" "vtk_hdf5.h" HDF5_HAS_H5FREE)
check_symbol_exists("H5Pset_all_coll_metadata_ops" "vtk_hdf5.h" H5PSET_ALL_COLL_METADATA_OPS)
check_symbol_exists("H5Pset_libver_bounds" "vtk_hdf5.h" HDF5_HAS_LIBVER_BOUNDS)
check_symbol_exists("HDF5_PARALLEL" "vtk_hdf5.h" HDF5_PARALLEL)

include(CheckIncludeFiles)
check_include_file("alloca.h" HAVE_ALLOCA_H)
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("malloc.h" HAVE_MALLOC_H)
check_include_file("stdarg.h" HAVE_STDARG_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("sys/param.h" HAVE_SYS_PARAM_H)
check_include_file("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

include(CheckTypeSize)
check_type_size("longlong" HAVE_LONGLONG)
check_type_size("long long int" HAVE_LONG_LONG_INT)
check_type_size("ssize_t" HAVE_SSIZE_T)
check_type_size("uchar" HAVE_UCHAR)
check_type_size("uint" HAVE_UINT)
check_type_size("ushort" HAVE_USHORT)
check_type_size("double" SIZEOF_DOUBLE)
check_type_size("float" SIZEOF_FLOAT)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size("off_t" SIZEOF_OFF_T)
check_type_size("short" SIZEOF_SHORT)
check_type_size("size_t" SIZEOF_SIZE_T)
check_type_size("unsigned char" SIZEOF_UNSIGNED_CHAR)
check_type_size("unsigned int" SIZEOF_UNSIGNED_INT)
check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)
check_type_size("unsigned short int" SIZEOF_UNSIGNED_SHORT_INT)

set(SIZEOF_UCHAR ${HAVE_UCHAR})
set(SIZEOF_UINT ${HAVE_UINT})
set(SIZEOF_USHORT ${HAVE_USHORT})
set(SIZEOF_VOIDP ${CMAKE_SIZEOF_VOID_P})

include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)

set(PACKAGE_VERSION "4.5.0")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/netcdf_meta.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/netcdf_meta.h"
  @ONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_netcdf_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdf_config.h"
  @ONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
  @ONLY)

set(headers
  include/netcdf.h
  include/vtk_netcdf_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/include/netcdf_meta.h"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdf_config.h")

vtk_add_library(vtknetcdf ${sources} ${headers})
target_link_libraries(vtknetcdf PUBLIC ${vtkhdf5_LIBRARIES})

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtknetcdf/include"
    COMPONENT Development)
endif()
target_compile_definitions(vtknetcdf
  PRIVATE
    HAVE_CONFIG_H)
if (WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(vtknetcdf
    PRIVATE
      DLL_EXPORT
      DLL_NETCDF)
endif ()
target_include_directories(vtknetcdf
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libdispatch"
    "${CMAKE_CURRENT_SOURCE_DIR}/libsrc"
    "${CMAKE_CURRENT_SOURCE_DIR}/libsrc4"
    "${CMAKE_CURRENT_BINARY_DIR}/include")

if (DEFINED VTK_CUSTOM_LIBRARY_SUFFIX)
  set(_lib_suffix "${VTK_CUSTOM_LIBRARY_SUFFIX}")
else ()
  set(_lib_suffix "-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
endif ()
set_property(TARGET vtknetcdf PROPERTY OUTPUT_NAME vtkNetCDF${_lib_suffix})

if (UNIX)
  # link against the math library.
  target_link_libraries(vtknetcdf
    PRIVATE
      m)
endif ()
