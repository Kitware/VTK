PROJECT(VTKNETCDF)

INCLUDE_REGULAR_EXPRESSION(".*")

SET(VERSION "4.1.2")
SET(PACKAGE "netcdf")

# Include all the necessary files for macros
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckTypeSize)
INCLUDE (CheckSymbolExists)
INCLUDE (TestBigEndian)

MACRO(MANGLE_VARIABLE_NAME str var prefix)
  STRING(TOUPPER "${str}" mangle_variable_name_var)
  STRING(REGEX REPLACE "[/. ]" "_" mangle_variable_name_var "${mangle_variable_name_var}")
  SET(${var} "${prefix}${mangle_variable_name_var}")
ENDMACRO(MANGLE_VARIABLE_NAME str var)

# Check if header file exists and add it to the list.
MACRO(CHECK_INCLUDE_FILE_CONCAT FILE)
  MANGLE_VARIABLE_NAME("${FILE}" "CHECK_INCLUDE_FILE_CONCAT_VAR" "HAVE_")
  CHECK_INCLUDE_FILES("${HEADER_INCLUDES};${FILE}" ${CHECK_INCLUDE_FILE_CONCAT_VAR})
  IF(${CHECK_INCLUDE_FILE_CONCAT_VAR})
    SET(HEADER_INCLUDES ${HEADER_INCLUDES} ${FILE})
  ENDIF(${CHECK_INCLUDE_FILE_CONCAT_VAR})
ENDMACRO(CHECK_INCLUDE_FILE_CONCAT)

MACRO(CHECK_FUNCTION_EXISTS_EX FUNC)
  MANGLE_VARIABLE_NAME("${FUNC}" "CHECK_FUNCTION_EXISTS_EX_VAR" "HAVE_")
  CHECK_FUNCTION_EXISTS("${FUNC}" "${CHECK_FUNCTION_EXISTS_EX_VAR}")
ENDMACRO(CHECK_FUNCTION_EXISTS_EX)

MACRO(CHECK_SYMBOL_EXISTS_EX SYM)
  MANGLE_VARIABLE_NAME("${SYM}" "CHECK_SYMBOL_EXISTS_EX_VAR" "HAVE_")
  CHECK_SYMBOL_EXISTS("${SYM}" "${HEADER_INCLUDES}" "${CHECK_SYMBOL_EXISTS_EX_VAR}")
ENDMACRO(CHECK_SYMBOL_EXISTS_EX)

MACRO(CHECK_TYPE_SIZE_EX type)
  MANGLE_VARIABLE_NAME("${type}" "check_type_size_var" "")
  CHECK_TYPE_SIZE("${type}" "SIZEOF_${check_type_size_var}")
  IF(HAVE_${check_type_size_var})
    SET("HAVE_${check_type_size_var}" 1)
  ENDIF(HAVE_${check_type_size_var})
ENDMACRO(CHECK_TYPE_SIZE_EX)

MACRO(C_SOURCE_COMPILES SOURCE VAR)
  IF("${VAR}" MATCHES "^${VAR}$")
    SET(MACRO_CHECK_FUNCTION_DEFINITIONS
      "-D${VAR} ${CMAKE_REQUIRED_FLAGS}")
    IF(CMAKE_REQUIRED_LIBRARIES)
      SET(C_SOURCE_COMPILES_ADD_LIBRARIES
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    ENDIF(CMAKE_REQUIRED_LIBRARIES)
    FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/src.c"
      "${SOURCE}")

    MESSAGE(STATUS "Performing NetCDF Test ${VAR}")
    TRY_COMPILE(${VAR}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}/CMakeTmp/src.c
      CMAKE_FLAGS
      "${C_SOURCE_COMPILES_ADD_LIBRARIES}"
      OUTPUT_VARIABLE OUTPUT)
    IF(${VAR})
      SET(${VAR} 1 CACHE INTERNAL "NetCDF test ${FUNCTION}")
      MESSAGE(STATUS "Performing NetCDF Test ${VAR} - Success")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Performing C SOURCE FILE Test ${VAR} succeded with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n")
    ELSE(${VAR})
      MESSAGE(STATUS "Performing NetCDF Test ${VAR} - Failed")
      SET(${VAR} "" CACHE INTERNAL "NetCDF test ${FUNCTION}")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Performing C SOURCE FILE Test ${VAR} failed with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n")
    ENDIF(${VAR})
  ENDIF("${VAR}" MATCHES "^${VAR}$")
ENDMACRO(C_SOURCE_COMPILES)

CHECK_INCLUDE_FILES("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)
FOREACH(file
    "alloca.h"
    "stdlib.h"
    "sys/types.h"
    "sys/stat.h"
    "unistd.h"
    "fcntl.h"
    "stdio.h"
    "string.h"
    "stddef.h"
    "stdint.h"
    "inttypes.h"
    "stdbool.h"
    )
  CHECK_INCLUDE_FILE_CONCAT("${file}")
ENDFOREACH(file)
FOREACH(func
    alloca
    strerror
    )
  CHECK_SYMBOL_EXISTS_EX("${func}")
ENDFOREACH(func)
FOREACH(type
    "size_t"
    "ssize_t"
    "ptrdiff_t"
    "off_t"
    "double"
    "float"
    "int"
    "long"
    "short"
    "uchar"
    "_Bool"
    )
  CHECK_TYPE_SIZE_EX("${type}")
ENDFOREACH(type)

SET(testsrc
"#include <sys/stat.h>
int main() { return 0; }
int t() {
struct stat s; s.st_blksize;
; return 0; }")
IF(HAVE_SYS_TYPES_H)
  SET(testsrc "#include <sys/types.h>\n${testsrc}")
ENDIF(HAVE_SYS_TYPES_H)

IF(HAVE_SYS_STAT_H)
  C_SOURCE_COMPILES("${testsrc}" HAVE_ST_BLKSIZE)
ENDIF(HAVE_SYS_STAT_H)

#
SET(testsrc
  "main()
{
  char*   path = tmpnam(NULL);
  int     exitStatus = 1;

  if (path != NULL)
    {
    int   fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);

    if (fd != -1)
      {
      if (write(fd, \"0\", 1) == 1)
        {
        off_t   pos = lseek(fd, 0, SEEK_CUR);

        if (pos != (off_t)-1)
          {
          if (ftruncate(fd, 512) != -1)
            {
            if (pos == lseek(fd, 0, SEEK_CUR))
              {
              if (lseek(fd, 0, SEEK_SET) == 0)
                {
                char  buf[512];

                if (read(fd, buf, 512) == 512)
                  exitStatus = 0;
                }
              }
            }
          }
        }
      close(fd);
      unlink(path);
      }
    }

  return exitStatus;
}")
IF(HAVE_SYS_TYPES_H)
  SET(testsrc "#include <sys/types.h>\n${testsrc}")
ENDIF(HAVE_SYS_TYPES_H)
IF(HAVE_SYS_STAT_H)
  SET(testsrc "#include <sys/stat.h>\n${testsrc}")
ENDIF(HAVE_SYS_STAT_H)
IF(HAVE_FCNTL_H)
  SET(testsrc "#include <fcntl.h>\n${testsrc}")
ENDIF(HAVE_FCNTL_H)
IF(HAVE_STDIO_H)
  SET(testsrc "#include <stdio.h>\n${testsrc}")
ENDIF(HAVE_STDIO_H)
IF(HAVE_UNISTD_H)
  SET(testsrc "#include <unistd.h>\n${testsrc}")
ENDIF(HAVE_UNISTD_H)
C_SOURCE_COMPILES("${testsrc}" HAVE_FTRUNCATE)

MACRO(INVERT_VARIABLE var val)
  SET(INVERT_VARIABLE_VAR "${val}")
  SET(${var} "1")
  IF(INVERT_VARIABLE_VAR)
    SET(${var} "0")
  ENDIF(INVERT_VARIABLE_VAR)
ENDMACRO(INVERT_VARIABLE)

INVERT_VARIABLE(NO_STDLIB_H "${HAVE_STDLIB_H}")
INVERT_VARIABLE(NO_SYS_TYPES_H "${HAVE_SYS_TYPES_H}")
INVERT_VARIABLE(NO_STRERROR "${HAVE_STRERROR}")

IF(NOT HAVE_SIZEOF_SIZE_T)
  SET(size_t long)
ENDIF(NOT HAVE_SIZEOF_SIZE_T)
IF(NOT HAVE_SIZEOF_OFF_T)
  SET(off_t long)
ENDIF(NOT HAVE_SIZEOF_OFF_T)
IF(NOT HAVE_SIZEOF_SSIZE_T)
  SET(ssize_t int)
ENDIF(NOT HAVE_SIZEOF_SSIZE_T)
IF(NOT HAVE_SIZEOF_PTRDIFF_T)
  SET(ptrdiff_t int)
ENDIF(NOT HAVE_SIZEOF_PTRDIFF_T)
#IF(NOT HAVE_SIZEOF_UCHAR)
#  SET(uchar "unsigned char")
#ENDIF(NOT HAVE_SIZEOF_UCHAR)

# use the same name for the variable as in VTK, so the result is reused
TEST_BIG_ENDIAN(CMAKE_WORDS_BIGENDIAN)
SET(WORDS_BIGENDIAN ${CMAKE_WORDS_BIGENDIAN})

OPTION(NETCDF_ENABLE_CXX "Build netcdf C++ API" ON)
mark_as_advanced(NETCDF_ENABLE_CXX)
SET(CXX NETCDF_ENABLE_CXX)

# NetCDF4 needs HDF5 HL support. Check that's available.
set (USE_NETCDF4 OFF)
if (NOT VTK_USE_SYSTEM_HDF5)
  # using VTK's HDF5, we always build that with HL support.
  set (USE_NETCDF4 ON)
elseif(HDF5_HL_LIBRARY OR HDF5_hdf5_hl_LIBRARY)
  set (USE_NETCDF4 ON)
else()
  message(STATUS "Disabling NETCDF4 support since HDF5_HL or HDF5_hdf5_hl is missing.")
endif ()

if (USE_NETCDF4)
  # message(STATUS "NETCDF4 support is enabled.")
  set(NETCDF4_DEFAULT_CHUNK_SIZE 4194304 CACHE STRING "Specify default size of chunks in bytes.")
  mark_as_advanced(NETCDF4_DEFAULT_CHUNK_SIZE)
  
  set(NETCDF4_MAX_DEFAULT_CACHE_SIZE 67108864 CACHE STRING "Specify maximum size (in bytes) for the default per-var chunk cache.")
  mark_as_advanced(NETCDF4_MAX_DEFAULT_CACHE_SIZE)

  set(NETCDF4_DEFAULT_CHUNKS_IN_CACHE 10 CACHE STRING "Specify the number of chunks to store in default per-variable cache.")
  mark_as_advanced(NETCDF4_DEFAULT_CHUNKS_IN_CACHE)

  set(NETCDF4_CHUNK_CACHE_SIZE 4194304 CACHE STRING "Specify default file cache chunk size for HDF5 files in bytes.")
  mark_as_advanced(NETCDF4_CHUNK_CACHE_SIZE)

  set(NETCDF4_CHUNK_CACHE_NELEMS 1009 CACHE STRING "Specify default maximum number of elements in the file chunk cache chunk for HDF5 files (should be prime number).")
  mark_as_advanced(NETCDF4_CHUNK_CACHE_NELEMS)

  set(NETCDF4_CHUNK_CACHE_PREEMPTION 0.75 CACHE STRING "Specify default file chunk cache preemption policy for HDF5 files (a number between 0 and 1, inclusive).")
  mark_as_advanced(NETCDF4_CHUNK_CACHE_PREEMPTION)
endif (USE_NETCDF4)
  
CONFIGURE_FILE(vtk_netcdf_config.h.in vtk_netcdf_config.h @ONLY IMMEDIATE)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/ncconfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY IMMEDIATE)

#-----------------------------------------------------------------------------
# Option to allow the user to disable compiler warnings
#-----------------------------------------------------------------------------
OPTION (NETCDF_DISABLE_COMPILER_WARNINGS "Disable compiler warnings" ON)
MARK_AS_ADVANCED(NETCDF_DISABLE_COMPILER_WARNINGS)
IF (NETCDF_DISABLE_COMPILER_WARNINGS)
  # MSVC uses /w to suppress warnings.  It also complains if another
  # warning level is given, so remove it.
  IF (MSVC)
    STRING (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " "
        CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    STRING (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " "
        CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w")
  # Borland uses -w- to suppress warnings.
  ELSEIF (BORLAND)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-")
    SET (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -w-")
  ELSE ()
  # Most compilers use -w to suppress warnings.
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
    SET (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -w")
  ENDIF ()
ENDIF (NETCDF_DISABLE_COMPILER_WARNINGS)


#-----------------------------------------------------------------------------
#configure the various requested parts of netcdf 
#-----------------------------------------------------------------------------
# Take care of symbol export/import on Windows
ADD_DEFINITIONS("-DNC_DLL_EXPORT")

INCLUDE_DIRECTORIES(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/libdispatch
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc4
  )

set(netcdf_liblib_SOURCES "" CACHE INTERNAL "" FORCE)
ADD_SUBDIRECTORY(liblib)

set(netcdf_dispatch_SOURCES "" CACHE INTERNAL "" FORCE)
ADD_SUBDIRECTORY(libdispatch)

set(netcdf_src_SOURCES "" CACHE INTERNAL "" FORCE)
ADD_SUBDIRECTORY(libsrc)

set(netcdf_src4_SOURCES "" CACHE INTERNAL "" FORCE)
IF (USE_NETCDF4)
  ADD_SUBDIRECTORY(libsrc4)
ENDIF (USE_NETCDF4)

IF (NETCDF_ENABLE_CXX)
  set(netcdf_cxx_SOURCES "" CACHE INTERNAL "" FORCE)
  ADD_SUBDIRECTORY(cxx)
ENDIF (NETCDF_ENABLE_CXX)

IF(APPLE)
  SET_SOURCE_FILES_PROPERTIES(
    v2i.c
    PROPERTIES
    COMPILE_FLAGS -fno-common)
ENDIF(APPLE)

VTK_ADD_LIBRARY(vtkNetCDF 
    ${netcdf_src_SOURCES}
    ${netcdf_src4_SOURCES}
    ${netcdf_dispatch_SOURCES}
    ${netcdf_liblib_SOURCES})
target_link_libraries(vtkNetCDF ${vtkhdf5_LIBRARIES})

if (UNIX)
  # link against the math library.
  target_link_libraries(vtkNetCDF m)
endif()

VTK_ADD_LIBRARY(vtkNetCDF_cxx ${netcdf_cxx_SOURCES})
TARGET_LINK_LIBRARIES(vtkNetCDF_cxx LINK_PUBLIC vtkNetCDF)

#-----------------------------------------------------------------------------
#package the result
#-----------------------------------------------------------------------------

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTKNETCDF_SOURCE_DIR}/include/netcdf.h
    ${VTKNETCDF_SOURCE_DIR}/include/vtk_netcdf_mangle.h
    ${VTKNETCDF_SOURCE_DIR}/cxx/netcdf.hh
    ${VTKNETCDF_SOURCE_DIR}/cxx/netcdfcpp.h
    ${VTKNETCDF_SOURCE_DIR}/cxx/ncvalues.h
    ${VTKNETCDF_BINARY_DIR}/vtk_netcdf_config.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtknetcdf/include
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
