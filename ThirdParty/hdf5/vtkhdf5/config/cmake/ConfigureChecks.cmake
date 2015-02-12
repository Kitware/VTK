#-----------------------------------------------------------------------------
# Include all the necessary files for macros
#-----------------------------------------------------------------------------
include (${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
include (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include (${CMAKE_ROOT}/Modules/CheckIncludeFileCXX.cmake)
include (${CMAKE_ROOT}/Modules/CheckIncludeFiles.cmake)
include (${CMAKE_ROOT}/Modules/CheckLibraryExists.cmake)
include (${CMAKE_ROOT}/Modules/CheckSymbolExists.cmake)
include (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
include (${CMAKE_ROOT}/Modules/CheckVariableExists.cmake)
include (${CMAKE_ROOT}/Modules/CheckFortranFunctionExists.cmake)
include (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
include (${CMAKE_ROOT}/Modules/TestForSTDNamespace.cmake)

#-----------------------------------------------------------------------------
# APPLE/Darwin setup
#-----------------------------------------------------------------------------
if (APPLE)
  list (LENGTH CMAKE_OSX_ARCHITECTURES ARCH_LENGTH)
  if (ARCH_LENGTH GREATER 1)
    set (CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
    message(FATAL_ERROR "HDF5: Building Universal Binaries on OS X is NOT supported by the HDF5 project. This is"
    "due to technical reasons. The best approach would be build each architecture in separate directories"
    "and use the 'lipo' tool to combine them into a single executable or library. The 'CMAKE_OSX_ARCHITECTURES'"
    "variable has been set to a blank value which will build the default architecture for this system.")
  endif ()
  set (H5_AC_APPLE_UNIVERSAL_BUILD 0)
endif (APPLE)

# Check for Darwin (not just Apple - we also want to catch OpenDarwin)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 
    set (H5_HAVE_DARWIN 1) 
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Check for Solaris
if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS") 
    set (H5_HAVE_SOLARIS 1) 
endif (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

#-----------------------------------------------------------------------------
# Option to Clear File Buffers before write --enable-clear-file-buffers
#-----------------------------------------------------------------------------
option (HDF5_Enable_Clear_File_Buffers "Securely clear file buffers before writing to file" ON)
if (HDF5_Enable_Clear_File_Buffers)
  set (H5_CLEAR_MEMORY 1)
endif (HDF5_Enable_Clear_File_Buffers)
MARK_AS_ADVANCED (HDF5_Enable_Clear_File_Buffers)

#-----------------------------------------------------------------------------
# Option for --enable-strict-format-checks
#-----------------------------------------------------------------------------
option (HDF5_STRICT_FORMAT_CHECKS "Whether to perform strict file format checks" OFF)
if (HDF5_STRICT_FORMAT_CHECKS)
  set (H5_STRICT_FORMAT_CHECKS 1)
endif (HDF5_STRICT_FORMAT_CHECKS)
MARK_AS_ADVANCED (HDF5_STRICT_FORMAT_CHECKS)

#-----------------------------------------------------------------------------
# Option for --enable-metadata-trace-file
#-----------------------------------------------------------------------------
option (HDF5_METADATA_TRACE_FILE "Enable metadata trace file collection" OFF)
if (HDF5_METADATA_TRACE_FILE)
  set (H5_METADATA_TRACE_FILE 1)
endif (HDF5_METADATA_TRACE_FILE)
MARK_AS_ADVANCED (HDF5_METADATA_TRACE_FILE)

# ----------------------------------------------------------------------
# Decide whether the data accuracy has higher priority during data
# conversions.  If not, some hard conversions will still be prefered even
# though the data may be wrong (for example, some compilers don't
# support denormalized floating values) to maximize speed.
#
option (HDF5_WANT_DATA_ACCURACY "IF data accuracy is guaranteed during data conversions" ON)
if (HDF5_WANT_DATA_ACCURACY)
  set (H5_WANT_DATA_ACCURACY 1)
endif (HDF5_WANT_DATA_ACCURACY)
MARK_AS_ADVANCED (HDF5_WANT_DATA_ACCURACY)

# ----------------------------------------------------------------------
# Decide whether the presence of user's exception handling functions is
# checked and data conversion exceptions are returned.  This is mainly
# for the speed optimization of hard conversions.  Soft conversions can
# actually benefit little.
#
option (HDF5_WANT_DCONV_EXCEPTION "exception handling functions is checked during data conversions" ON)
if (HDF5_WANT_DCONV_EXCEPTION)
  set (H5_WANT_DCONV_EXCEPTION 1)
endif (HDF5_WANT_DCONV_EXCEPTION)
MARK_AS_ADVANCED (HDF5_WANT_DCONV_EXCEPTION)

# ----------------------------------------------------------------------
# Check if they would like the function stack support compiled in
#
option (HDF5_ENABLE_CODESTACK "Enable the function stack tracing (for developer debugging)." OFF)
if (HDF5_ENABLE_CODESTACK)
  set (H5_HAVE_CODESTACK 1)
endif (HDF5_ENABLE_CODESTACK)
MARK_AS_ADVANCED (HDF5_ENABLE_CODESTACK)

option (HDF5_ENABLE_HSIZET "Enable datasets larger than memory" ON)

# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can handle converting
# floating-point to long long values.
# (This flag should be _unset_ for all machines)
#
#  set (H5_HW_FP_TO_LLONG_NOT_WORKS 0)

# so far we have no check for this
set (H5_HAVE_TMPFILE 1)

#-----------------------------------------------------------------------------
# This MACRO checks IF the symbol exists in the library and IF it
# does, it appends library to the list.
#-----------------------------------------------------------------------------
set (LINK_LIBS "")
MACRO (CHECK_LIBRARY_EXISTS_CONCAT LIBRARY SYMBOL VARIABLE)
  CHECK_LIBRARY_EXISTS ("${LIBRARY};${LINK_LIBS}" ${SYMBOL} "" ${VARIABLE})
  if (${VARIABLE})
    set (LINK_LIBS ${LINK_LIBS} ${LIBRARY})
  endif (${VARIABLE})
ENDMACRO (CHECK_LIBRARY_EXISTS_CONCAT)

# ----------------------------------------------------------------------
# WINDOWS Hard code Values
# ----------------------------------------------------------------------

set (WINDOWS)
if (WIN32)
  if (MINGW)
    set (H5_HAVE_MINGW 1)
    set (WINDOWS 1) # MinGW tries to imitate Windows
    set (CMAKE_REQUIRED_FLAGS "-DWIN32_LEAN_AND_MEAN=1 -DNOGDI=1")
  endif (MINGW)
  set (H5_HAVE_WIN32_API 1)
  set (CMAKE_REQUIRED_LIBRARIES "ws2_32.lib;wsock32.lib")
  if (NOT UNIX AND NOT CYGWIN AND NOT MINGW)
    set (WINDOWS 1)
    set (CMAKE_REQUIRED_FLAGS "/DWIN32_LEAN_AND_MEAN=1 /DNOGDI=1")
    if (MSVC)
      set (H5_HAVE_VISUAL_STUDIO 1)
    endif (MSVC)
  endif (NOT UNIX AND NOT CYGWIN AND NOT MINGW)
endif (WIN32)

# TODO --------------------------------------------------------------------------
# Should the Default Virtual File Driver be compiled?
# This is hard-coded now but option should added to match configure
#
set (H5_DEFAULT_VFD H5FD_SEC2)

if (NOT DEFINED "H5_DEFAULT_PLUGINDIR")
  if (WINDOWS)
    set (H5_DEFAULT_PLUGINDIR "%ALLUSERSPROFILE%/hdf5/lib/plugin")
  else (WINDOWS)
    set (H5_DEFAULT_PLUGINDIR "/usr/local/hdf5/lib/plugin")
  endif (WINDOWS)
endif (NOT DEFINED "H5_DEFAULT_PLUGINDIR")

if (WINDOWS)
  set (H5_HAVE_WINDOWS 1)
  # ----------------------------------------------------------------------
  # Set the flag to indicate that the machine has window style pathname,
  # that is, "drive-letter:\" (e.g. "C:") or "drive-letter:/" (e.g. "C:/").
  # (This flag should be _unset_ for all machines, except for Windows)
  set (H5_HAVE_WINDOW_PATH 1)
endif (WINDOWS)

if (WINDOWS)
  set (H5_HAVE_STDDEF_H 1)
  set (H5_HAVE_SYS_STAT_H 1)
  set (H5_HAVE_SYS_TYPES_H 1)
  set (H5_HAVE_LIBM 1)
  set (H5_HAVE_STRDUP 1)
  set (H5_HAVE_SYSTEM 1)
  set (H5_HAVE_LONGJMP 1)
  if (NOT MINGW)
    set (H5_HAVE_GETHOSTNAME 1)
  endif (NOT MINGW)
  if (NOT UNIX AND NOT CYGWIN AND NOT MINGW)
    set (H5_HAVE_GETCONSOLESCREENBUFFERINFO 1)
  endif (NOT UNIX AND NOT CYGWIN AND NOT MINGW)
  set (H5_HAVE_FUNCTION 1)
  set (H5_GETTIMEOFDAY_GIVES_TZ 1)
  set (H5_HAVE_TIMEZONE 1)
  set (H5_HAVE_GETTIMEOFDAY 1)
  set (H5_LONE_COLON 0)
  if (MINGW)
    set (H5_HAVE_WINSOCK2_H 1)
  endif (MINGW)
  set (H5_HAVE_LIBWS2_32 1)
  set (H5_HAVE_LIBWSOCK32 1)

  #-----------------------------------------------------------------------------
  # These tests need to be manually SET for windows since there is currently
  # something not quite correct with the actual test implementation. This affects
  # the 'dt_arith' test and most likely lots of other code
  # ----------------------------------------------------------------------------
  set (H5_FP_TO_ULLONG_RIGHT_MAXIMUM "" CACHE INTERNAL "")
endif (WINDOWS)

# ----------------------------------------------------------------------
# END of WINDOWS Hard code Values
# ----------------------------------------------------------------------

if (CYGWIN)
  set (H5_HAVE_LSEEK64 0)
endif (CYGWIN)

#-----------------------------------------------------------------------------
#  Check for the math library "m"
#-----------------------------------------------------------------------------
if (NOT WINDOWS)
  CHECK_LIBRARY_EXISTS_CONCAT ("m" ceil     H5_HAVE_LIBM)
  CHECK_LIBRARY_EXISTS_CONCAT ("dl" dlopen     H5_HAVE_LIBDL)
  CHECK_LIBRARY_EXISTS_CONCAT ("ws2_32" WSAStartup  H5_HAVE_LIBWS2_32)
  CHECK_LIBRARY_EXISTS_CONCAT ("wsock32" gethostbyname H5_HAVE_LIBWSOCK32)
endif (NOT WINDOWS)

CHECK_LIBRARY_EXISTS_CONCAT ("ucb"    gethostname  H5_HAVE_LIBUCB)
CHECK_LIBRARY_EXISTS_CONCAT ("socket" connect      H5_HAVE_LIBSOCKET)
CHECK_LIBRARY_EXISTS ("c" gethostbyname "" NOT_NEED_LIBNSL)

if (NOT NOT_NEED_LIBNSL)
  CHECK_LIBRARY_EXISTS_CONCAT ("nsl"    gethostbyname  H5_HAVE_LIBNSL)
endif (NOT NOT_NEED_LIBNSL)

# For other tests to use the same libraries
set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${LINK_LIBS})

set (USE_INCLUDES "")
if (WINDOWS)
  set (USE_INCLUDES ${USE_INCLUDES} "windows.h")
endif (WINDOWS)

if (NOT WINDOWS)
  TEST_BIG_ENDIAN (H5_WORDS_BIGENDIAN)
endif (NOT WINDOWS)

# For other specific tests, use this MACRO.
MACRO (HDF5_FUNCTION_TEST OTHER_TEST)
  if ("H5_${OTHER_TEST}" MATCHES "^H5_${OTHER_TEST}$")
    set (MACRO_CHECK_FUNCTION_DEFINITIONS "-D${OTHER_TEST} ${CMAKE_REQUIRED_FLAGS}")
    set (OTHER_TEST_ADD_LIBRARIES)
    if (CMAKE_REQUIRED_LIBRARIES)
      set (OTHER_TEST_ADD_LIBRARIES "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    endif (CMAKE_REQUIRED_LIBRARIES)

    foreach (def ${HDF5_EXTRA_TEST_DEFINITIONS})
      set (MACRO_CHECK_FUNCTION_DEFINITIONS "${MACRO_CHECK_FUNCTION_DEFINITIONS} -D${def}=${${def}}")
    endforeach (def)

    foreach (def
        HAVE_SYS_TIME_H
        HAVE_UNISTD_H
        HAVE_SYS_TYPES_H
        HAVE_SYS_SOCKET_H
    )
      if ("${H5_${def}}")
        set (MACRO_CHECK_FUNCTION_DEFINITIONS "${MACRO_CHECK_FUNCTION_DEFINITIONS} -D${def}")
      endif ("${H5_${def}}")
    endforeach (def)

    if (LARGEFILE)
      set (MACRO_CHECK_FUNCTION_DEFINITIONS
          "${MACRO_CHECK_FUNCTION_DEFINITIONS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE"
      )
    endif (LARGEFILE)

    #message (STATUS "Performing ${OTHER_TEST}")
    TRY_COMPILE (${OTHER_TEST}
        ${CMAKE_BINARY_DIR}
        ${HDF5_RESOURCES_DIR}/HDF5Tests.c
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
        "${OTHER_TEST_ADD_LIBRARIES}"
        OUTPUT_VARIABLE OUTPUT
    )
    if (${OTHER_TEST})
      set (H5_${OTHER_TEST} 1 CACHE INTERNAL "Other test ${FUNCTION}")
      message (STATUS "HDF5: Performing Other Test ${OTHER_TEST} - Success")
    else (${OTHER_TEST})
      message (STATUS "HDF5: Performing Other Test ${OTHER_TEST} - Failed")
      set (H5_${OTHER_TEST} "" CACHE INTERNAL "Other test ${FUNCTION}")
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Performing Other Test ${OTHER_TEST} failed with the following output:\n"
          "${OUTPUT}\n"
      )
    endif (${OTHER_TEST})
  endif ("H5_${OTHER_TEST}" MATCHES "^H5_${OTHER_TEST}$")
ENDMACRO (HDF5_FUNCTION_TEST)

#-----------------------------------------------------------------------------
# Check for these functions before the time headers are checked
#-----------------------------------------------------------------------------
HDF5_FUNCTION_TEST (STDC_HEADERS)

CHECK_FUNCTION_EXISTS (difftime          H5_HAVE_DIFFTIME)
#CHECK_FUNCTION_EXISTS (gettimeofday      H5_HAVE_GETTIMEOFDAY)
#  Since gettimeofday is not defined any where standard, lets look in all the
#  usual places. On MSVC we are just going to use ::clock()
if (NOT MSVC)
  if ("H5_HAVE_TIME_GETTIMEOFDAY" MATCHES "^H5_HAVE_TIME_GETTIMEOFDAY$")
    TRY_COMPILE (HAVE_TIME_GETTIMEOFDAY
        ${CMAKE_BINARY_DIR}
        ${HDF5_RESOURCES_DIR}/GetTimeOfDayTest.cpp
        COMPILE_DEFINITIONS -DTRY_TIME_H
        OUTPUT_VARIABLE OUTPUT
    )
    if (HAVE_TIME_GETTIMEOFDAY STREQUAL "TRUE")
      set (H5_HAVE_TIME_GETTIMEOFDAY "1" CACHE INTERNAL "H5_HAVE_TIME_GETTIMEOFDAY")
      set (H5_HAVE_GETTIMEOFDAY "1" CACHE INTERNAL "H5_HAVE_GETTIMEOFDAY")
    endif (HAVE_TIME_GETTIMEOFDAY STREQUAL "TRUE")
  endif ("H5_HAVE_TIME_GETTIMEOFDAY" MATCHES "^H5_HAVE_TIME_GETTIMEOFDAY$")

  if ("H5_HAVE_SYS_TIME_GETTIMEOFDAY" MATCHES "^H5_HAVE_SYS_TIME_GETTIMEOFDAY$")
    TRY_COMPILE (HAVE_SYS_TIME_GETTIMEOFDAY
        ${CMAKE_BINARY_DIR}
        ${HDF5_RESOURCES_DIR}/GetTimeOfDayTest.cpp
        COMPILE_DEFINITIONS -DTRY_SYS_TIME_H
        OUTPUT_VARIABLE OUTPUT
    )
    if (HAVE_SYS_TIME_GETTIMEOFDAY STREQUAL "TRUE")
      set (H5_HAVE_SYS_TIME_GETTIMEOFDAY "1" CACHE INTERNAL "H5_HAVE_SYS_TIME_GETTIMEOFDAY")
      set (H5_HAVE_GETTIMEOFDAY "1" CACHE INTERNAL "H5_HAVE_GETTIMEOFDAY")
    endif (HAVE_SYS_TIME_GETTIMEOFDAY STREQUAL "TRUE")
  endif ("H5_HAVE_SYS_TIME_GETTIMEOFDAY" MATCHES "^H5_HAVE_SYS_TIME_GETTIMEOFDAY$")

  if (NOT HAVE_SYS_TIME_GETTIMEOFDAY AND NOT H5_HAVE_GETTIMEOFDAY)
    message (STATUS "HDF5 ---------------------------------------------------------------")
    message (STATUS "HDF5: Function 'gettimeofday()' was not found. HDF5 will use its")
    message (STATUS "HDF5:  own implementation.. This can happen on older versions of")
    message (STATUS "HDF5:  MinGW on Windows. Consider upgrading your MinGW installation")
    message (STATUS "HDF5:  to a newer version such as MinGW 3.12")
    message (STATUS "HDF5:---------------------------------------------------------------")
  endif (NOT HAVE_SYS_TIME_GETTIMEOFDAY AND NOT H5_HAVE_GETTIMEOFDAY)
endif (NOT MSVC)

# Find the library containing clock_gettime()
if (NOT WINDOWS)
  CHECK_FUNCTION_EXISTS(clock_gettime CLOCK_GETTIME_IN_LIBC)
  CHECK_LIBRARY_EXISTS(rt clock_gettime "" CLOCK_GETTIME_IN_LIBRT)
  CHECK_LIBRARY_EXISTS(posix4 clock_gettime "" CLOCK_GETTIME_IN_LIBPOSIX4)
  if (CLOCK_GETTIME_IN_LIBC)
    set (H5_HAVE_CLOCK_GETTIME 1)
  elseif (CLOCK_GETTIME_IN_LIBRT)
    set (H5_HAVE_CLOCK_GETTIME 1)
    list (APPEND LINK_LIBS rt)
  elseif (CLOCK_GETTIME_IN_LIBPOSIX4)
    set (H5_HAVE_CLOCK_GETTIME 1)
    list (APPEND LINK_LIBS posix4)
  endif (CLOCK_GETTIME_IN_LIBC)
endif (NOT WINDOWS)
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Check IF header file exists and add it to the list.
#-----------------------------------------------------------------------------
MACRO (CHECK_INCLUDE_FILE_CONCAT FILE VARIABLE)
  CHECK_INCLUDE_FILES ("${USE_INCLUDES};${FILE}" ${VARIABLE})
  if (${VARIABLE})
    set (USE_INCLUDES ${USE_INCLUDES} ${FILE})
  endif (${VARIABLE})
ENDMACRO (CHECK_INCLUDE_FILE_CONCAT)

#-----------------------------------------------------------------------------
#  Check for the existence of certain header files
#-----------------------------------------------------------------------------
CHECK_INCLUDE_FILE_CONCAT ("sys/resource.h"  H5_HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/time.h"      H5_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CONCAT ("unistd.h"        H5_HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/ioctl.h"     H5_HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/stat.h"      H5_HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/socket.h"    H5_HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/types.h"     H5_HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CONCAT ("stddef.h"        H5_HAVE_STDDEF_H)
CHECK_INCLUDE_FILE_CONCAT ("setjmp.h"        H5_HAVE_SETJMP_H)
CHECK_INCLUDE_FILE_CONCAT ("features.h"      H5_HAVE_FEATURES_H)
CHECK_INCLUDE_FILE_CONCAT ("dirent.h"        H5_HAVE_DIRENT_H)
CHECK_INCLUDE_FILE_CONCAT ("stdint.h"        H5_HAVE_STDINT_H)

# IF the c compiler found stdint, check the C++ as well. On some systems this
# file will be found by C but not C++, only do this test IF the C++ compiler
# has been initialized (e.g. the project also includes some c++)
if (H5_HAVE_STDINT_H AND CMAKE_CXX_COMPILER_LOADED)
  CHECK_INCLUDE_FILE_CXX ("stdint.h" H5_HAVE_STDINT_H_CXX)
  if (NOT H5_HAVE_STDINT_H_CXX)
    set (H5_HAVE_STDINT_H "" CACHE INTERNAL "Have includes HAVE_STDINT_H")
    set (USE_INCLUDES ${USE_INCLUDES} "stdint.h")
  endif (NOT H5_HAVE_STDINT_H_CXX)
endif (H5_HAVE_STDINT_H AND CMAKE_CXX_COMPILER_LOADED)

# Darwin
CHECK_INCLUDE_FILE_CONCAT ("mach/mach_time.h" H5_HAVE_MACH_MACH_TIME_H)

# Windows
CHECK_INCLUDE_FILE_CONCAT ("io.h"            H5_HAVE_IO_H)
if (NOT CYGWIN)
  CHECK_INCLUDE_FILE_CONCAT ("winsock2.h"      H5_HAVE_WINSOCK2_H)
endif (NOT CYGWIN)
CHECK_INCLUDE_FILE_CONCAT ("sys/timeb.h"     H5_HAVE_SYS_TIMEB_H)

if (CMAKE_SYSTEM_NAME MATCHES "OSF")
  CHECK_INCLUDE_FILE_CONCAT ("sys/sysinfo.h" H5_HAVE_SYS_SYSINFO_H)
  CHECK_INCLUDE_FILE_CONCAT ("sys/proc.h"    H5_HAVE_SYS_PROC_H)
else (CMAKE_SYSTEM_NAME MATCHES "OSF")
  set (H5_HAVE_SYS_SYSINFO_H "" CACHE INTERNAL "" FORCE)
  set (H5_HAVE_SYS_PROC_H    "" CACHE INTERNAL "" FORCE)
endif (CMAKE_SYSTEM_NAME MATCHES "OSF")

CHECK_INCLUDE_FILE_CONCAT ("globus/common.h" H5_HAVE_GLOBUS_COMMON_H)
CHECK_INCLUDE_FILE_CONCAT ("pdb.h"           H5_HAVE_PDB_H)
CHECK_INCLUDE_FILE_CONCAT ("pthread.h"       H5_HAVE_PTHREAD_H)
CHECK_INCLUDE_FILE_CONCAT ("srbclient.h"     H5_HAVE_SRBCLIENT_H)
CHECK_INCLUDE_FILE_CONCAT ("string.h"        H5_HAVE_STRING_H)
CHECK_INCLUDE_FILE_CONCAT ("strings.h"       H5_HAVE_STRINGS_H)
CHECK_INCLUDE_FILE_CONCAT ("time.h"          H5_HAVE_TIME_H)
CHECK_INCLUDE_FILE_CONCAT ("stdlib.h"        H5_HAVE_STDLIB_H)
CHECK_INCLUDE_FILE_CONCAT ("memory.h"        H5_HAVE_MEMORY_H)
CHECK_INCLUDE_FILE_CONCAT ("dlfcn.h"         H5_HAVE_DLFCN_H)
CHECK_INCLUDE_FILE_CONCAT ("inttypes.h"      H5_HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE_CONCAT ("netinet/in.h"    H5_HAVE_NETINET_IN_H)

#-----------------------------------------------------------------------------
#  Check for large file support
#-----------------------------------------------------------------------------

# The linux-lfs option is deprecated.
set (LINUX_LFS 0)

set (HDF5_EXTRA_C_FLAGS)
set (HDF5_EXTRA_FLAGS)
if (NOT WINDOWS)
  if (NOT H5_HAVE_SOLARIS)
  # Linux Specific flags
  # This was originally defined as _POSIX_SOURCE which was updated to
  # _POSIX_C_SOURCE=199506L to expose a greater amount of POSIX
  # functionality so clock_gettime and CLOCK_MONOTONIC are defined
  # correctly.
  # POSIX feature information can be found in the gcc manual at:
  # http://www.gnu.org/s/libc/manual/html_node/Feature-Test-Macros.html
  set (HDF5_EXTRA_C_FLAGS -D_POSIX_C_SOURCE=199506L)
  set (HDF5_EXTRA_FLAGS -D_DEFAULT_SOURCE -D_BSD_SOURCE)
  
  option (HDF5_ENABLE_LARGE_FILE "Enable support for large (64-bit) files on Linux." ON)
  if (HDF5_ENABLE_LARGE_FILE)
    set (msg "Performing TEST_LFS_WORKS")
    TRY_RUN (TEST_LFS_WORKS_RUN   TEST_LFS_WORKS_COMPILE
        ${HDF5_BINARY_DIR}/CMake
        ${HDF5_RESOURCES_DIR}/HDF5Tests.c
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=-DTEST_LFS_WORKS
        OUTPUT_VARIABLE OUTPUT
    )
    if (TEST_LFS_WORKS_COMPILE)
      if (TEST_LFS_WORKS_RUN  MATCHES 0)
        set (TEST_LFS_WORKS 1 CACHE INTERNAL ${msg})
        set (LARGEFILE 1)
        set (HDF5_EXTRA_FLAGS ${HDF5_EXTRA_FLAGS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE)
        message (STATUS "HDF5: ${msg}... yes")
      else (TEST_LFS_WORKS_RUN  MATCHES 0)
        set (TEST_LFS_WORKS "" CACHE INTERNAL ${msg})
        message (STATUS "HDF5: ${msg}... no")
        file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
              "Test TEST_LFS_WORKS Run failed with the following output and exit code:\n ${OUTPUT}\n"
        )
      endif (TEST_LFS_WORKS_RUN  MATCHES 0)
    else (TEST_LFS_WORKS_COMPILE )
      set (TEST_LFS_WORKS "" CACHE INTERNAL ${msg})
      message (STATUS "HDF5: ${msg}... no")
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Test TEST_LFS_WORKS Compile failed with the following output:\n ${OUTPUT}\n"
      )
    endif (TEST_LFS_WORKS_COMPILE)
  endif (HDF5_ENABLE_LARGE_FILE)
  set (CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} ${HDF5_EXTRA_FLAGS})
  endif (NOT H5_HAVE_SOLARIS)
endif (NOT WINDOWS)

add_definitions (${HDF5_EXTRA_FLAGS})

#-----------------------------------------------------------------------------
# Check for HAVE_OFF64_T functionality
#-----------------------------------------------------------------------------
if (NOT WINDOWS OR MINGW)
  HDF5_FUNCTION_TEST (HAVE_OFF64_T)
  if (H5_HAVE_OFF64_T)
    CHECK_FUNCTION_EXISTS (lseek64            H5_HAVE_LSEEK64)
    CHECK_FUNCTION_EXISTS (fseeko64           H5_HAVE_FSEEKO64)
    CHECK_FUNCTION_EXISTS (ftello64           H5_HAVE_FTELLO64)
    CHECK_FUNCTION_EXISTS (ftruncate64        H5_HAVE_FTRUNCATE64)
  endif (H5_HAVE_OFF64_T)

  CHECK_FUNCTION_EXISTS (fseeko               H5_HAVE_FSEEKO)
  CHECK_FUNCTION_EXISTS (ftello               H5_HAVE_FTELLO)

  HDF5_FUNCTION_TEST (HAVE_STAT64_STRUCT)
  if (HAVE_STAT64_STRUCT)
    CHECK_FUNCTION_EXISTS (fstat64            H5_HAVE_FSTAT64)
    CHECK_FUNCTION_EXISTS (stat64             H5_HAVE_STAT64)
  endif (HAVE_STAT64_STRUCT)
endif (NOT WINDOWS OR MINGW)

#-----------------------------------------------------------------------------
#  Check the size in bytes of all the int and float types
#-----------------------------------------------------------------------------
MACRO (H5_CHECK_TYPE_SIZE type var)
  set (aType ${type})
  set (aVar  ${var})
#  message (STATUS "Checking size of ${aType} and storing into ${aVar}")
  CHECK_TYPE_SIZE (${aType}   ${aVar})
  if (NOT ${aVar})
    set (${aVar} 0 CACHE INTERNAL "SizeOf for ${aType}")
#    message (STATUS "Size of ${aType} was NOT Found")
  endif (NOT ${aVar})
ENDMACRO (H5_CHECK_TYPE_SIZE)

H5_CHECK_TYPE_SIZE (char           H5_SIZEOF_CHAR)
H5_CHECK_TYPE_SIZE (short          H5_SIZEOF_SHORT)
H5_CHECK_TYPE_SIZE (int            H5_SIZEOF_INT)
H5_CHECK_TYPE_SIZE (unsigned       H5_SIZEOF_UNSIGNED)
if (NOT APPLE)
  H5_CHECK_TYPE_SIZE (long         H5_SIZEOF_LONG)
endif (NOT APPLE)
H5_CHECK_TYPE_SIZE ("long long"    H5_SIZEOF_LONG_LONG)
H5_CHECK_TYPE_SIZE (__int64        H5_SIZEOF___INT64)
if (NOT H5_SIZEOF___INT64)
  set (H5_SIZEOF___INT64 0)
endif (NOT H5_SIZEOF___INT64)

H5_CHECK_TYPE_SIZE (float          H5_SIZEOF_FLOAT)
H5_CHECK_TYPE_SIZE (double         H5_SIZEOF_DOUBLE)
H5_CHECK_TYPE_SIZE ("long double"  H5_SIZEOF_LONG_DOUBLE)

H5_CHECK_TYPE_SIZE (int8_t         H5_SIZEOF_INT8_T)
H5_CHECK_TYPE_SIZE (uint8_t        H5_SIZEOF_UINT8_T)
H5_CHECK_TYPE_SIZE (int_least8_t   H5_SIZEOF_INT_LEAST8_T)
H5_CHECK_TYPE_SIZE (uint_least8_t  H5_SIZEOF_UINT_LEAST8_T)
H5_CHECK_TYPE_SIZE (int_fast8_t    H5_SIZEOF_INT_FAST8_T)
H5_CHECK_TYPE_SIZE (uint_fast8_t   H5_SIZEOF_UINT_FAST8_T)

H5_CHECK_TYPE_SIZE (int16_t        H5_SIZEOF_INT16_T)
H5_CHECK_TYPE_SIZE (uint16_t       H5_SIZEOF_UINT16_T)
H5_CHECK_TYPE_SIZE (int_least16_t  H5_SIZEOF_INT_LEAST16_T)
H5_CHECK_TYPE_SIZE (uint_least16_t H5_SIZEOF_UINT_LEAST16_T)
H5_CHECK_TYPE_SIZE (int_fast16_t   H5_SIZEOF_INT_FAST16_T)
H5_CHECK_TYPE_SIZE (uint_fast16_t  H5_SIZEOF_UINT_FAST16_T)

H5_CHECK_TYPE_SIZE (int32_t        H5_SIZEOF_INT32_T)
H5_CHECK_TYPE_SIZE (uint32_t       H5_SIZEOF_UINT32_T)
H5_CHECK_TYPE_SIZE (int_least32_t  H5_SIZEOF_INT_LEAST32_T)
H5_CHECK_TYPE_SIZE (uint_least32_t H5_SIZEOF_UINT_LEAST32_T)
H5_CHECK_TYPE_SIZE (int_fast32_t   H5_SIZEOF_INT_FAST32_T)
H5_CHECK_TYPE_SIZE (uint_fast32_t  H5_SIZEOF_UINT_FAST32_T)

H5_CHECK_TYPE_SIZE (int64_t        H5_SIZEOF_INT64_T)
H5_CHECK_TYPE_SIZE (uint64_t       H5_SIZEOF_UINT64_T)
H5_CHECK_TYPE_SIZE (int_least64_t  H5_SIZEOF_INT_LEAST64_T)
H5_CHECK_TYPE_SIZE (uint_least64_t H5_SIZEOF_UINT_LEAST64_T)
H5_CHECK_TYPE_SIZE (int_fast64_t   H5_SIZEOF_INT_FAST64_T)
H5_CHECK_TYPE_SIZE (uint_fast64_t  H5_SIZEOF_UINT_FAST64_T)

if (NOT APPLE)
  H5_CHECK_TYPE_SIZE (size_t       H5_SIZEOF_SIZE_T)
  H5_CHECK_TYPE_SIZE (ssize_t      H5_SIZEOF_SSIZE_T)
  if (NOT H5_SIZEOF_SSIZE_T)
    set (H5_SIZEOF_SSIZE_T 0)
  endif (NOT H5_SIZEOF_SSIZE_T)
  if (NOT WINDOWS)
    H5_CHECK_TYPE_SIZE (ptrdiff_t    H5_SIZEOF_PTRDIFF_T)
  endif (NOT WINDOWS)
endif (NOT APPLE)

H5_CHECK_TYPE_SIZE (off_t          H5_SIZEOF_OFF_T)
H5_CHECK_TYPE_SIZE (off64_t        H5_SIZEOF_OFF64_T)
if (NOT H5_SIZEOF_OFF64_T)
  set (H5_SIZEOF_OFF64_T 0)
endif (NOT H5_SIZEOF_OFF64_T)

if (NOT WINDOWS)
  #-----------------------------------------------------------------------------
  # Check if the dev_t type is a scalar type
  #-----------------------------------------------------------------------------
  HDF5_FUNCTION_TEST (DEV_T_IS_SCALAR)

  # ----------------------------------------------------------------------
  # Check for MONOTONIC_TIMER support (used in clock_gettime).  This has
  # to be done after any POSIX/BSD defines to ensure that the test gets
  # the correct POSIX level on linux.
  CHECK_VARIABLE_EXISTS (CLOCK_MONOTONIC HAVE_CLOCK_MONOTONIC)

  #-----------------------------------------------------------------------------
  # Check a bunch of time functions
  #-----------------------------------------------------------------------------
  foreach (test
      HAVE_TM_GMTOFF
      HAVE___TM_GMTOFF
#      HAVE_TIMEZONE
      HAVE_STRUCT_TIMEZONE
      GETTIMEOFDAY_GIVES_TZ
      TIME_WITH_SYS_TIME
      HAVE_TM_ZONE
      HAVE_STRUCT_TM_TM_ZONE
  )
    HDF5_FUNCTION_TEST (${test})
  endforeach (test)
  if (NOT CYGWIN AND NOT MINGW)
      HDF5_FUNCTION_TEST (HAVE_TIMEZONE)
#      HDF5_FUNCTION_TEST (HAVE_STAT_ST_BLOCKS)
  endif (NOT CYGWIN AND NOT MINGW)

  # ----------------------------------------------------------------------
  # Does the struct stat have the st_blocks field?  This field is not Posix.
  #
  HDF5_FUNCTION_TEST (HAVE_STAT_ST_BLOCKS)

  # ----------------------------------------------------------------------
  # How do we figure out the width of a tty in characters?
  #
  CHECK_FUNCTION_EXISTS (ioctl             H5_HAVE_IOCTL)
  HDF5_FUNCTION_TEST (HAVE_STRUCT_VIDEOCONFIG)
  HDF5_FUNCTION_TEST (HAVE_STRUCT_TEXT_INFO)
  CHECK_FUNCTION_EXISTS (_getvideoconfig   H5_HAVE__GETVIDEOCONFIG)
  CHECK_FUNCTION_EXISTS (gettextinfo       H5_HAVE_GETTEXTINFO)
  CHECK_FUNCTION_EXISTS (_scrsize          H5_HAVE__SCRSIZE)
  if (NOT CYGWIN AND NOT MINGW)
    CHECK_FUNCTION_EXISTS (GetConsoleScreenBufferInfo    H5_HAVE_GETCONSOLESCREENBUFFERINFO)
  endif (NOT CYGWIN AND NOT MINGW)
  CHECK_SYMBOL_EXISTS (TIOCGWINSZ "sys/ioctl.h" H5_HAVE_TIOCGWINSZ)
  CHECK_SYMBOL_EXISTS (TIOCGETD   "sys/ioctl.h" H5_HAVE_TIOCGETD)
endif (NOT WINDOWS)

#-----------------------------------------------------------------------------
# Check for some functions that are used
#
CHECK_FUNCTION_EXISTS (alarm             H5_HAVE_ALARM)
#CHECK_FUNCTION_EXISTS (BSDgettimeofday   H5_HAVE_BSDGETTIMEOFDAY)
CHECK_FUNCTION_EXISTS (fork              H5_HAVE_FORK)
CHECK_FUNCTION_EXISTS (frexpf            H5_HAVE_FREXPF)
CHECK_FUNCTION_EXISTS (frexpl            H5_HAVE_FREXPL)

CHECK_FUNCTION_EXISTS (gethostname       H5_HAVE_GETHOSTNAME)
CHECK_FUNCTION_EXISTS (getpwuid          H5_HAVE_GETPWUID)
CHECK_FUNCTION_EXISTS (getrusage         H5_HAVE_GETRUSAGE)
CHECK_FUNCTION_EXISTS (lstat             H5_HAVE_LSTAT)

CHECK_FUNCTION_EXISTS (rand_r            H5_HAVE_RAND_R)
CHECK_FUNCTION_EXISTS (random            H5_HAVE_RANDOM)
CHECK_FUNCTION_EXISTS (setsysinfo        H5_HAVE_SETSYSINFO)

CHECK_FUNCTION_EXISTS (signal            H5_HAVE_SIGNAL)
CHECK_FUNCTION_EXISTS (longjmp           H5_HAVE_LONGJMP)
CHECK_FUNCTION_EXISTS (setjmp            H5_HAVE_SETJMP)
CHECK_FUNCTION_EXISTS (siglongjmp        H5_HAVE_SIGLONGJMP)
CHECK_FUNCTION_EXISTS (sigsetjmp         H5_HAVE_SIGSETJMP)
CHECK_FUNCTION_EXISTS (sigaction         H5_HAVE_SIGACTION)
CHECK_FUNCTION_EXISTS (sigprocmask       H5_HAVE_SIGPROCMASK)

CHECK_FUNCTION_EXISTS (snprintf          H5_HAVE_SNPRINTF)
CHECK_FUNCTION_EXISTS (srandom           H5_HAVE_SRANDOM)
CHECK_FUNCTION_EXISTS (strdup            H5_HAVE_STRDUP)
CHECK_FUNCTION_EXISTS (symlink           H5_HAVE_SYMLINK)
CHECK_FUNCTION_EXISTS (system            H5_HAVE_SYSTEM)

CHECK_FUNCTION_EXISTS (tmpfile           H5_HAVE_TMPFILE)
CHECK_FUNCTION_EXISTS (vasprintf         H5_HAVE_VASPRINTF)
CHECK_FUNCTION_EXISTS (waitpid           H5_HAVE_WAITPID)

CHECK_FUNCTION_EXISTS (vsnprintf         H5_HAVE_VSNPRINTF)
if (NOT WINDOWS)
  if (H5_HAVE_VSNPRINTF)
    HDF5_FUNCTION_TEST (VSNPRINTF_WORKS)
  endif (H5_HAVE_VSNPRINTF)
endif (NOT WINDOWS)

#-----------------------------------------------------------------------------
# sigsetjmp is special; may actually be a macro
#-----------------------------------------------------------------------------
if (NOT H5_HAVE_SIGSETJMP)
  if (H5_HAVE_SETJMP_H)
    CHECK_SYMBOL_EXISTS (sigsetjmp "setjmp.h" H5_HAVE_MACRO_SIGSETJMP)
    if (H5_HAVE_MACRO_SIGSETJMP)
      set (H5_HAVE_SIGSETJMP 1)
    endif (H5_HAVE_MACRO_SIGSETJMP)
  endif (H5_HAVE_SETJMP_H)
endif (NOT H5_HAVE_SIGSETJMP)

#-----------------------------------------------------------------------------
# Check for Symbols
CHECK_SYMBOL_EXISTS (tzname "time.h" H5_HAVE_DECL_TZNAME)

#-----------------------------------------------------------------------------
# Check a bunch of other functions
#-----------------------------------------------------------------------------
if (NOT WINDOWS)
  foreach (test
      LONE_COLON
      HAVE_ATTRIBUTE
      HAVE_C99_FUNC
      HAVE_FUNCTION
      HAVE_C99_DESIGNATED_INITIALIZER
      SYSTEM_SCOPE_THREADS
      HAVE_SOCKLEN_T
      CXX_HAVE_OFFSETOF
  )
    HDF5_FUNCTION_TEST (${test})
  endforeach (test)
endif (NOT WINDOWS)

# For other CXX specific tests, use this MACRO.
MACRO (HDF5_CXX_FUNCTION_TEST OTHER_TEST)
  if ("${OTHER_TEST}" MATCHES "^${OTHER_TEST}$")
    set (MACRO_CHECK_FUNCTION_DEFINITIONS "-D${OTHER_TEST} ${CMAKE_REQUIRED_FLAGS}")
    set (OTHER_TEST_ADD_LIBRARIES)
    if (CMAKE_REQUIRED_LIBRARIES)
      set (OTHER_TEST_ADD_LIBRARIES "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    endif (CMAKE_REQUIRED_LIBRARIES)

    foreach (def ${HDF5_EXTRA_TEST_DEFINITIONS})
      set (MACRO_CHECK_FUNCTION_DEFINITIONS "${MACRO_CHECK_FUNCTION_DEFINITIONS} -D${def}=${${def}}")
    endforeach (def)

    foreach (def
        HAVE_SYS_TIME_H
        HAVE_UNISTD_H
        HAVE_SYS_TYPES_H
        HAVE_SYS_SOCKET_H
    )
      if ("${H5_${def}}")
        set (MACRO_CHECK_FUNCTION_DEFINITIONS "${MACRO_CHECK_FUNCTION_DEFINITIONS} -D${def}")
      endif ("${H5_${def}}")
    endforeach (def)

    if (LARGEFILE)
      set (MACRO_CHECK_FUNCTION_DEFINITIONS
          "${MACRO_CHECK_FUNCTION_DEFINITIONS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE"
      )
    endif (LARGEFILE)

    #message (STATUS "Performing ${OTHER_TEST}")
    TRY_COMPILE (${OTHER_TEST}
        ${CMAKE_BINARY_DIR}
        ${HDF5_RESOURCES_DIR}/HDF5CXXTests.cpp
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
        "${OTHER_TEST_ADD_LIBRARIES}"
        OUTPUT_VARIABLE OUTPUT
    )
    if ("${OTHER_TEST}" EQUAL 0)
      set (${OTHER_TEST} 1 CACHE INTERNAL "CXX test ${FUNCTION}")
      message (STATUS "HDF5: Performing CXX Test ${OTHER_TEST} - Success")
    else ("${OTHER_TEST}" EQUAL 0)
      message (STATUS "HDF5: Performing CXX Test ${OTHER_TEST} - Failed")
      set (${OTHER_TEST} "" CACHE INTERNAL "CXX test ${FUNCTION}")
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Performing CXX Test ${OTHER_TEST} failed with the following output:\n"
          "${OUTPUT}\n"
      )
    endif ("${OTHER_TEST}" EQUAL 0)
  endif ("${OTHER_TEST}" MATCHES "^${OTHER_TEST}$")
ENDMACRO (HDF5_CXX_FUNCTION_TEST)

#-----------------------------------------------------------------------------
# Check a bunch of cxx functions
#-----------------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_LOADED)
  foreach (test
      OLD_HEADER_FILENAME
      H5_NO_NAMESPACE
      H5_NO_STD
      BOOL_NOTDEFINED
      NO_STATIC_CAST
  )
    HDF5_CXX_FUNCTION_TEST (${test})
  endforeach (test)
endif (CMAKE_CXX_COMPILER_LOADED)

#-----------------------------------------------------------------------------
#  Check if Direct I/O driver works
#-----------------------------------------------------------------------------
if (NOT WINDOWS)
  option (HDF5_ENABLE_DIRECT_VFD "Build the Direct I/O Virtual File Driver" ON)
  mark_as_advanced(HDF5_ENABLE_DIRECT_VFD)
  if (HDF5_ENABLE_DIRECT_VFD)
    set (msg "Performing TEST_DIRECT_VFD_WORKS")
    set (MACRO_CHECK_FUNCTION_DEFINITIONS "-DTEST_DIRECT_VFD_WORKS -D_GNU_SOURCE ${CMAKE_REQUIRED_FLAGS}")
    TRY_RUN (TEST_DIRECT_VFD_WORKS_RUN   TEST_DIRECT_VFD_WORKS_COMPILE
        ${HDF5_BINARY_DIR}/CMake
        ${HDF5_RESOURCES_DIR}/HDF5Tests.c
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
        OUTPUT_VARIABLE OUTPUT
    )
    if (TEST_DIRECT_VFD_WORKS_COMPILE)
      if (TEST_DIRECT_VFD_WORKS_RUN  MATCHES 0)
        HDF5_FUNCTION_TEST (HAVE_DIRECT)
        set (CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} -D_GNU_SOURCE")
        add_definitions ("-D_GNU_SOURCE")
      else (TEST_DIRECT_VFD_WORKS_RUN  MATCHES 0)
        set (TEST_DIRECT_VFD_WORKS "" CACHE INTERNAL ${msg})
        message (STATUS "HDF5: ${msg}... no")
        file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
              "Test TEST_DIRECT_VFD_WORKS Run failed with the following output and exit code:\n ${OUTPUT}\n"
        )
      endif (TEST_DIRECT_VFD_WORKS_RUN  MATCHES 0)
    else (TEST_DIRECT_VFD_WORKS_COMPILE )
      set (TEST_DIRECT_VFD_WORKS "" CACHE INTERNAL ${msg})
      message (STATUS "HDF5: ${msg}... no")
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Test TEST_DIRECT_VFD_WORKS Compile failed with the following output:\n ${OUTPUT}\n"
      )
    endif (TEST_DIRECT_VFD_WORKS_COMPILE)
  endif (HDF5_ENABLE_DIRECT_VFD)
endif (NOT WINDOWS)

#-----------------------------------------------------------------------------
# Check if InitOnceExecuteOnce is available
#-----------------------------------------------------------------------------
if (WINDOWS)
  if (NOT HDF5_NO_IOEO_TEST)
  message (STATUS "HDF5: Checking for InitOnceExecuteOnce:")
  if ("${H5_HAVE_IOEO}" MATCHES "^${H5_HAVE_IOEO}$")
    if (LARGEFILE)
      set (CMAKE_REQUIRED_DEFINITIONS
          "${CURRENT_TEST_DEFINITIONS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE"
      )
    endif (LARGEFILE)
    set (MACRO_CHECK_FUNCTION_DEFINITIONS 
      "-DHAVE_IOEO ${CMAKE_REQUIRED_FLAGS}")
    if (CMAKE_REQUIRED_LIBRARIES)
      set (CHECK_C_SOURCE_COMPILES_ADD_LIBRARIES
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    else (CMAKE_REQUIRED_LIBRARIES)
      set (CHECK_C_SOURCE_COMPILES_ADD_LIBRARIES)
    endif (CMAKE_REQUIRED_LIBRARIES)
    if (CMAKE_REQUIRED_INCLUDES)
      set (CHECK_C_SOURCE_COMPILES_ADD_INCLUDES
        "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
    else (CMAKE_REQUIRED_INCLUDES)
      set (CHECK_C_SOURCE_COMPILES_ADD_INCLUDES)
    endif (CMAKE_REQUIRED_INCLUDES)

    TRY_RUN(HAVE_IOEO_EXITCODE HAVE_IOEO_COMPILED
      ${CMAKE_BINARY_DIR}
      ${HDF5_RESOURCES_DIR}/HDF5Tests.c
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
      CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
      -DCMAKE_SKIP_RPATH:BOOL=${CMAKE_SKIP_RPATH}
      "${CHECK_C_SOURCE_COMPILES_ADD_LIBRARIES}"
      "${CHECK_C_SOURCE_COMPILES_ADD_INCLUDES}"
      COMPILE_OUTPUT_VARIABLE OUTPUT)
    # if it did not compile make the return value fail code of 1
    if (NOT HAVE_IOEO_COMPILED)
      set (HAVE_IOEO_EXITCODE 1)
    endif (NOT HAVE_IOEO_COMPILED)
    # if the return value was 0 then it worked
    if ("${HAVE_IOEO_EXITCODE}" EQUAL 0)
      set (H5_HAVE_IOEO 1 CACHE INTERNAL "Test InitOnceExecuteOnce")
      message (STATUS "HDF5: Performing Test InitOnceExecuteOnce - Success")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log 
        "Performing C SOURCE FILE Test InitOnceExecuteOnce succeded with the following output:\n"
        "${OUTPUT}\n"
        "Return value: ${HAVE_IOEO}\n")
    else ("${HAVE_IOEO_EXITCODE}" EQUAL 0)
      if (CMAKE_CROSSCOMPILING AND "${HAVE_IOEO_EXITCODE}" MATCHES  "FAILED_TO_RUN")
        set (H5_HAVE_IOEO "${HAVE_IOEO_EXITCODE}")
      else (CMAKE_CROSSCOMPILING AND "${HAVE_IOEO_EXITCODE}" MATCHES  "FAILED_TO_RUN")
        set (H5_HAVE_IOEO "" CACHE INTERNAL "Test InitOnceExecuteOnce")
      endif (CMAKE_CROSSCOMPILING AND "${HAVE_IOEO_EXITCODE}" MATCHES  "FAILED_TO_RUN")

      message (STATUS "HDF5: Performing Test InitOnceExecuteOnce - Failed")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log 
        "Performing InitOnceExecuteOnce Test  failed with the following output:\n"
        "${OUTPUT}\n"
        "Return value: ${HAVE_IOEO_EXITCODE}\n")
    endif ("${HAVE_IOEO_EXITCODE}" EQUAL 0)
  endif ("${H5_HAVE_IOEO}" MATCHES "^${H5_HAVE_IOEO}$")
  endif (NOT HDF5_NO_IOEO_TEST)
endif (WINDOWS)

#-----------------------------------------------------------------------------
# Determine how 'inline' is used
#-----------------------------------------------------------------------------
set (HDF5_EXTRA_TEST_DEFINITIONS INLINE_TEST_INLINE)
foreach (inline_test inline __inline__ __inline)
  set (INLINE_TEST_INLINE ${inline_test})
  HDF5_FUNCTION_TEST (INLINE_TEST_${inline_test})
endforeach (inline_test)

set (HDF5_EXTRA_TEST_DEFINITIONS)
if (INLINE_TEST___inline__)
  set (H5_inline __inline__)
else (INLINE_TEST___inline__)
  if (INLINE_TEST___inline)
    set (H5_inline __inline)
  else (INLINE_TEST___inline)
    if (INLINE_TEST_inline)
      set (H5_inline inline)
    endif (INLINE_TEST_inline)
  endif (INLINE_TEST___inline)
endif (INLINE_TEST___inline__)

#-----------------------------------------------------------------------------
# Check how to print a Long Long integer
#-----------------------------------------------------------------------------
if (NOT H5_PRINTF_LL_WIDTH OR H5_PRINTF_LL_WIDTH MATCHES "unknown")
  set (PRINT_LL_FOUND 0)
  message (STATUS "HDF5: Checking for appropriate format for 64 bit long:")
  foreach (HDF5_PRINTF_LL l64 l L q I64 ll)
    set (CURRENT_TEST_DEFINITIONS "-DPRINTF_LL_WIDTH=${HDF5_PRINTF_LL}")
    if (H5_SIZEOF_LONG_LONG)
      set (CURRENT_TEST_DEFINITIONS "${CURRENT_TEST_DEFINITIONS} -DHAVE_LONG_LONG")
    endif (H5_SIZEOF_LONG_LONG)
    TRY_RUN (HDF5_PRINTF_LL_TEST_RUN   HDF5_PRINTF_LL_TEST_COMPILE
        ${HDF5_BINARY_DIR}/CMake
        ${HDF5_RESOURCES_DIR}/HDF5Tests.c
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${CURRENT_TEST_DEFINITIONS}
        OUTPUT_VARIABLE OUTPUT
    )
    if (HDF5_PRINTF_LL_TEST_COMPILE)
      if (HDF5_PRINTF_LL_TEST_RUN MATCHES 0)
        set (H5_PRINTF_LL_WIDTH "\"${HDF5_PRINTF_LL}\"" CACHE INTERNAL "Width for printf for type `long long' or `__int64', us. `ll")
        set (PRINT_LL_FOUND 1)
      else (HDF5_PRINTF_LL_TEST_RUN MATCHES 0)
        message ("HDF5: Width with ${HDF5_PRINTF_LL} failed with result: ${HDF5_PRINTF_LL_TEST_RUN}")
      endif (HDF5_PRINTF_LL_TEST_RUN MATCHES 0)
    else (HDF5_PRINTF_LL_TEST_COMPILE)
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Test H5_PRINTF_LL_WIDTH for ${HDF5_PRINTF_LL} failed with the following output:\n ${OUTPUT}\n"
      )
    endif (HDF5_PRINTF_LL_TEST_COMPILE)
  endforeach (HDF5_PRINTF_LL)

  if (PRINT_LL_FOUND)
    message (STATUS "HDF5: Checking for apropriate format for 64 bit long: found ${H5_PRINTF_LL_WIDTH}")
  else (PRINT_LL_FOUND)
    message (STATUS "HDF5: Checking for apropriate format for 64 bit long: not found")
    set (H5_PRINTF_LL_WIDTH "\"unknown\"" CACHE INTERNAL
        "Width for printf for type `long long' or `__int64', us. `ll"
    )
  endif (PRINT_LL_FOUND)
endif (NOT H5_PRINTF_LL_WIDTH OR H5_PRINTF_LL_WIDTH MATCHES "unknown")

# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can handle converting
# denormalized floating-point values.
# (This flag should be set for all machines, except for the Crays, where
# the cache value is set in it's config file)
#
set (H5_CONVERT_DENORMAL_FLOAT 1)

#-----------------------------------------------------------------------------
#  Are we going to use HSIZE_T
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_HSIZET)
  set (H5_HAVE_LARGE_HSIZET 1)
endif (HDF5_ENABLE_HSIZET)

#-----------------------------------------------------------------------------
# Macro to determine the various conversion capabilities
#-----------------------------------------------------------------------------
MACRO (H5ConversionTests TEST msg)
  if ("${TEST}" MATCHES "^${TEST}$")
   # message (STATUS "===> ${TEST}")
    TRY_RUN (${TEST}_RUN   ${TEST}_COMPILE
        ${HDF5_BINARY_DIR}/CMake
        ${HDF5_RESOURCES_DIR}/ConversionTests.c
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=-D${TEST}_TEST
        OUTPUT_VARIABLE OUTPUT
    )
    if (${TEST}_COMPILE)
      if (${TEST}_RUN  MATCHES 0)
        set (${TEST} 1 CACHE INTERNAL ${msg})
        message (STATUS "HDF5: ${msg}... yes")
      else (${TEST}_RUN  MATCHES 0)
        set (${TEST} "" CACHE INTERNAL ${msg})
        message (STATUS "HDF5: ${msg}... no")
        file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
              "Test ${TEST} Run failed with the following output and exit code:\n ${OUTPUT}\n"
        )
      endif (${TEST}_RUN  MATCHES 0)
    else (${TEST}_COMPILE )
      set (${TEST} "" CACHE INTERNAL ${msg})
      message (STATUS "HDF5: ${msg}... no")
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Test ${TEST} Compile failed with the following output:\n ${OUTPUT}\n"
      )
    endif (${TEST}_COMPILE)

  endif ("${TEST}" MATCHES "^${TEST}$")
ENDMACRO (H5ConversionTests)

#-----------------------------------------------------------------------------
# Macro to make some of the conversion tests easier to write/read
#-----------------------------------------------------------------------------
MACRO (H5MiscConversionTest  VAR TEST msg)
  if ("${TEST}" MATCHES "^${TEST}$")
    if (${VAR})
      set (${TEST} 1 CACHE INTERNAL ${msg})
      message (STATUS "HDF5: ${msg}... yes")
    else (${VAR})
      set (${TEST} "" CACHE INTERNAL ${msg})
      message (STATUS "HDF5: ${msg}... no")
    endif (${VAR})
  endif ("${TEST}" MATCHES "^${TEST}$")
ENDMACRO (H5MiscConversionTest)

#-----------------------------------------------------------------------------
# Check various conversion capabilities
#-----------------------------------------------------------------------------

# -----------------------------------------------------------------------
# Set flag to indicate that the machine can handle conversion from
# long double to integers accurately.  This flag should be set "yes" for
# all machines except all SGIs.  For SGIs, some conversions are
# incorrect and its cache value is set "no" in its config/irix6.x and
# irix5.x.
#
H5MiscConversionTest (H5_SIZEOF_LONG_DOUBLE H5_LDOUBLE_TO_INTEGER_ACCURATE "checking IF converting from long double to integers is accurate")
# -----------------------------------------------------------------------
# Set flag to indicate that the machine can do conversion from
# long double to integers regardless of accuracy.  This flag should be
# set "yes" for all machines except HP-UX 11.00.  For HP-UX 11.00, the
# compiler has 'floating exception' when converting 'long double' to all
# integers except 'unsigned long long'.  Other HP-UX systems are unknown
# yet. (1/8/05 - SLU)
#
if (NOT MSVC)
  H5ConversionTests (H5_LDOUBLE_TO_INTEGER_WORKS "Checking IF converting from long double to integers works")
endif (NOT MSVC)
# -----------------------------------------------------------------------
# Set flag to indicate that the machine can handle conversion from
# integers to long double.  (This flag should be set "yes" for all
# machines except all SGIs, where some conversions are
# incorrect and its cache value is set "no" in its config/irix6.x and
# irix5.x)
#
H5MiscConversionTest (H5_SIZEOF_LONG_DOUBLE H5_INTEGER_TO_LDOUBLE_ACCURATE "checking IF accurately converting from integers to long double")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# 'unsigned long' to 'float' values.
# (This flag should be set for all machines, except for Pathscale compiler
# on Sandia's Linux machine where the compiler interprets 'unsigned long'
# values as negative when the first bit of 'unsigned long' is on during
# the conversion to float.)
#
H5ConversionTests (H5_ULONG_TO_FLOAT_ACCURATE "Checking IF accurately converting unsigned long to float values")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# 'unsigned (long) long' values to 'float' and 'double' values.
# (This flag should be set for all machines, except for the SGIs, where
# the cache value is set in the config/irix6.x config file) and Solaris
# 64-bit machines, where the short program below tests if round-up is
# correctly handled.
#
if (CMAKE_SYSTEM MATCHES "solaris2.*")
  H5ConversionTests (H5_ULONG_TO_FP_BOTTOM_BIT_ACCURATE "Checking IF accurately converting unsigned long long to floating-point values")
else (CMAKE_SYSTEM MATCHES "solaris2.*")
  set (H5_ULONG_TO_FP_BOTTOM_BIT_ACCURATE 1)
endif (CMAKE_SYSTEM MATCHES "solaris2.*")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# 'float' or 'double' to 'unsigned long long' values.
# (This flag should be set for all machines, except for PGI compiler
# where round-up happens when the fraction of float-point value is greater
# than 0.5.
#
H5ConversionTests (H5_FP_TO_ULLONG_ACCURATE "Checking IF accurately roundup converting floating-point to unsigned long long values" )
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# 'float', 'double' or 'long double' to 'unsigned long long' values.
# (This flag should be set for all machines, except for HP-UX machines
# where the maximal number for unsigned long long is 0x7fffffffffffffff
# during conversion.
#
H5ConversionTests (H5_FP_TO_ULLONG_RIGHT_MAXIMUM "Checking IF right maximum converting floating-point to unsigned long long values" )
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# 'long double' to 'unsigned int' values.  (This flag should be set for
# all machines, except for some Intel compilers on some Linux.)
#
H5ConversionTests (H5_LDOUBLE_TO_UINT_ACCURATE "Checking IF correctly converting long double to unsigned int values")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can _compile_
# 'unsigned long long' to 'float' and 'double' typecasts.
# (This flag should be set for all machines.)
#
if (H5_ULLONG_TO_FP_CAST_WORKS MATCHES ^H5_ULLONG_TO_FP_CAST_WORKS$)
  set (H5_ULLONG_TO_FP_CAST_WORKS 1 CACHE INTERNAL "Checking IF compiling unsigned long long to floating-point typecasts work")
  message (STATUS "HDF5: Checking IF compiling unsigned long long to floating-point typecasts work... yes")
endif (H5_ULLONG_TO_FP_CAST_WORKS MATCHES ^H5_ULLONG_TO_FP_CAST_WORKS$)
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can _compile_
# 'long long' to 'float' and 'double' typecasts.
# (This flag should be set for all machines.)
#
if (H5_LLONG_TO_FP_CAST_WORKS MATCHES ^H5_LLONG_TO_FP_CAST_WORKS$)
  set (H5_LLONG_TO_FP_CAST_WORKS 1 CACHE INTERNAL "Checking IF compiling long long to floating-point typecasts work")
  message (STATUS "HDF5: Checking IF compiling long long to floating-point typecasts work... yes")
endif (H5_LLONG_TO_FP_CAST_WORKS MATCHES ^H5_LLONG_TO_FP_CAST_WORKS$)
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can convert from
# 'unsigned long long' to 'long double' without precision loss.
# (This flag should be set for all machines, except for FreeBSD(sleipnir)
# where the last 2 bytes of mantissa are lost when compiler tries to do
# the conversion, and Cygwin where compiler doesn't do rounding correctly.)
#
if (NOT MSVC)
  H5ConversionTests (H5_ULLONG_TO_LDOUBLE_PRECISION "Checking IF converting unsigned long long to long double with precision")
endif (NOT MSVC)
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can handle overflow converting
# all floating-point to all integer types.
# (This flag should be set for all machines, except for Cray X1 where
# floating exception is generated when the floating-point value is greater
# than the maximal integer value).
#
H5ConversionTests (H5_FP_TO_INTEGER_OVERFLOW_WORKS  "Checking IF overflows normally converting floating-point to integer values")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine is using a special algorithm to convert
# 'long double' to '(unsigned) long' values.  (This flag should only be set for 
# the IBM Power6 Linux.  When the bit sequence of long double is 
# 0x4351ccf385ebc8a0bfcc2a3c3d855620, the converted value of (unsigned)long 
# is 0x004733ce17af227f, not the same as the library's conversion to 0x004733ce17af2282.
# The machine's conversion gets the correct value.  We define the macro and disable
# this kind of test until we figure out what algorithm they use.
#
if (H5_LDOUBLE_TO_LONG_SPECIAL MATCHES ^H5_LDOUBLE_TO_LONG_SPECIAL$)
  set (H5_LDOUBLE_TO_LONG_SPECIAL 0 CACHE INTERNAL "Define if your system converts long double to (unsigned) long values with special algorithm")
  message (STATUS "HDF5: Checking IF your system converts long double to (unsigned) long values with special algorithm... no")
endif (H5_LDOUBLE_TO_LONG_SPECIAL MATCHES ^H5_LDOUBLE_TO_LONG_SPECIAL$)
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine is using a special algorithm
# to convert some values of '(unsigned) long' to 'long double' values.  
# (This flag should be off for all machines, except for IBM Power6 Linux, 
# when the bit sequences are 003fff..., 007fff..., 00ffff..., 01ffff..., 
# ..., 7fffff..., the compiler uses a unknown algorithm.  We define a 
# macro and skip the test for now until we know about the algorithm.
#
if (H5_LONG_TO_LDOUBLE_SPECIAL MATCHES ^H5_LONG_TO_LDOUBLE_SPECIAL$)
  set (H5_LONG_TO_LDOUBLE_SPECIAL 0 CACHE INTERNAL "Define if your system can convert (unsigned) long to long double values with special algorithm")
  message (STATUS "HDF5: Checking IF your system can convert (unsigned) long to long double values with special algorithm... no")
endif (H5_LONG_TO_LDOUBLE_SPECIAL MATCHES ^H5_LONG_TO_LDOUBLE_SPECIAL$)
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# 'long double' to '(unsigned) long long' values.  (This flag should be set for
# all machines, except for Mac OS 10.4 and SGI IRIX64 6.5.  When the bit sequence
# of long double is 0x4351ccf385ebc8a0bfcc2a3c..., the values of (unsigned)long long
# start to go wrong on these two machines.  Adjusting it higher to
# 0x4351ccf385ebc8a0dfcc... or 0x4351ccf385ebc8a0ffcc... will make the converted
# values wildly wrong.  This test detects this wrong behavior and disable the test.
#
H5ConversionTests (H5_LDOUBLE_TO_LLONG_ACCURATE "Checking IF correctly converting long double to (unsigned) long long values")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# '(unsigned) long long' to 'long double' values.  (This flag should be set for
# all machines, except for Mac OS 10.4, when the bit sequences are 003fff...,
# 007fff..., 00ffff..., 01ffff..., ..., 7fffff..., the converted values are twice
# as big as they should be.
#
H5ConversionTests (H5_LLONG_TO_LDOUBLE_CORRECT "Checking IF correctly converting (unsigned) long long to long double values")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine generates bad code
# for the H5VM_log2_gen() routine in src/H5VMprivate.h
# (This flag should be set to no for all machines, except for SGI IRIX64,
# where the cache value is set to yes in it's config file)
#
if (H5_BAD_LOG2_CODE_GENERATED MATCHES ^H5_BAD_LOG2_CODE_GENERATED$)
  set (H5_BAD_LOG2_CODE_GENERATED 0 CACHE INTERNAL "Define if your system generates wrong code for log2 routine")
  message (STATUS "HDF5: Checking IF your system generates wrong code for log2 routine... no")
endif (H5_BAD_LOG2_CODE_GENERATED MATCHES ^H5_BAD_LOG2_CODE_GENERATED$)
# ----------------------------------------------------------------------
# Check if pointer alignments are enforced
#
H5ConversionTests (H5_NO_ALIGNMENT_RESTRICTIONS "Checking IF alignment restrictions are strictly enforced")

# Define a macro for Cygwin (on XP only) where the compiler has rounding
#   problem converting from unsigned long long to long double */
if (CYGWIN)
  set (H5_CYGWIN_ULLONG_TO_LDOUBLE_ROUND_PROBLEM 1)
endif (CYGWIN)

# -----------------------------------------------------------------------
# wrapper script variables
# 
set (prefix ${CMAKE_INSTALL_PREFIX})
set (exec_prefix "\${prefix}")
set (libdir "${exec_prefix}/lib")
set (includedir "\${prefix}/include")
set (host_os ${CMAKE_HOST_SYSTEM_NAME})
set (CC ${CMAKE_C_COMPILER})
set (CXX ${CMAKE_CXX_COMPILER})
set (FC ${CMAKE_Fortran_COMPILER})
foreach (LINK_LIB ${LINK_LIBS})
  set (LIBS "${LIBS} -l${LINK_LIB}")
endforeach (LINK_LIB ${LINK_LIBS})
