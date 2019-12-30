#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the COPYING file, which can be found at the root of the source code
# distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.
# If you do not have access to either file, you may request a copy from
# help@hdfgroup.org.
#
#-----------------------------------------------------------------------------
# Include all the necessary files for macros
#-----------------------------------------------------------------------------
include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckIncludeFiles)
include (CheckLibraryExists)
include (CheckSymbolExists)
include (CheckTypeSize)
include (CheckVariableExists)
include (TestBigEndian)

#-----------------------------------------------------------------------------
# APPLE/Darwin setup
#-----------------------------------------------------------------------------
if (APPLE)
  list (LENGTH CMAKE_OSX_ARCHITECTURES ARCH_LENGTH)
  if (ARCH_LENGTH GREATER 1)
    set (CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
    message (FATAL_ERROR "Building Universal Binaries on OS X is NOT supported by the HDF5 project. This is"
    "due to technical reasons. The best approach would be build each architecture in separate directories"
    "and use the 'lipo' tool to combine them into a single executable or library. The 'CMAKE_OSX_ARCHITECTURES'"
    "variable has been set to a blank value which will build the default architecture for this system.")
  endif ()
  set (${HDF_PREFIX}_AC_APPLE_UNIVERSAL_BUILD 0)
endif ()

# Check for Darwin (not just Apple - we also want to catch OpenDarwin)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (${HDF_PREFIX}_HAVE_DARWIN 1)
endif ()

# Check for Solaris
if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    set (${HDF_PREFIX}_HAVE_SOLARIS 1)
endif ()

#-----------------------------------------------------------------------------
# This MACRO checks IF the symbol exists in the library and IF it
# does, it appends library to the list.
#-----------------------------------------------------------------------------
set (LINK_LIBS "")
macro (CHECK_LIBRARY_EXISTS_CONCAT LIBRARY SYMBOL VARIABLE)
  CHECK_LIBRARY_EXISTS ("${LIBRARY};${LINK_LIBS}" ${SYMBOL} "" ${VARIABLE})
  if (${VARIABLE})
    set (LINK_LIBS ${LINK_LIBS} ${LIBRARY})
  endif ()
endmacro ()

# ----------------------------------------------------------------------
# WINDOWS Hard code Values
# ----------------------------------------------------------------------
set (WINDOWS)

if (MINGW)
  set (${HDF_PREFIX}_HAVE_MINGW 1)
  set (WINDOWS 1) # MinGW tries to imitate Windows
  set (CMAKE_REQUIRED_FLAGS "-DWIN32_LEAN_AND_MEAN=1 -DNOGDI=1")
  set (${HDF_PREFIX}_HAVE_WINSOCK2_H 1)
endif ()

if (WIN32 AND NOT MINGW)
  if (NOT UNIX)
    set (WINDOWS 1)
    set (CMAKE_REQUIRED_FLAGS "/DWIN32_LEAN_AND_MEAN=1 /DNOGDI=1")
    if (MSVC)
      set (${HDF_PREFIX}_HAVE_VISUAL_STUDIO 1)
    endif ()
  endif ()
endif ()

if (WINDOWS)
  set (HDF5_REQUIRED_LIBRARIES "ws2_32.lib;wsock32.lib")
  set (${HDF_PREFIX}_HAVE_WIN32_API 1)
  set (${HDF_PREFIX}_HAVE_LIBM 1)
  set (${HDF_PREFIX}_HAVE_STRDUP 1)
  set (${HDF_PREFIX}_HAVE_SYSTEM 1)
  set (${HDF_PREFIX}_HAVE_LONGJMP 1)
  if (NOT MINGW)
    set (${HDF_PREFIX}_HAVE_GETHOSTNAME 1)
    set (${HDF_PREFIX}_HAVE_FUNCTION 1)
  endif ()
  if (NOT UNIX AND NOT CYGWIN)
    set (${HDF_PREFIX}_HAVE_GETCONSOLESCREENBUFFERINFO 1)
    set (${HDF_PREFIX}_GETTIMEOFDAY_GIVES_TZ 1)
    set (${HDF_PREFIX}_HAVE_TIMEZONE 1)
    set (${HDF_PREFIX}_HAVE_GETTIMEOFDAY 1)
    set (${HDF_PREFIX}_HAVE_LIBWS2_32 1)
    set (${HDF_PREFIX}_HAVE_LIBWSOCK32 1)
  endif ()
endif ()

# ----------------------------------------------------------------------
# END of WINDOWS Hard code Values
# ----------------------------------------------------------------------

if (NOT WINDOWS)
  TEST_BIG_ENDIAN (${HDF_PREFIX}_WORDS_BIGENDIAN)
endif ()

#-----------------------------------------------------------------------------
# Check IF header file exists and add it to the list.
#-----------------------------------------------------------------------------
macro (CHECK_INCLUDE_FILE_CONCAT FILE VARIABLE)
  CHECK_INCLUDE_FILES ("${USE_INCLUDES};${FILE}" ${VARIABLE})
  if (${VARIABLE})
    set (USE_INCLUDES ${USE_INCLUDES} ${FILE})
  endif ()
endmacro ()

#-----------------------------------------------------------------------------
#  Check for the existence of certain header files
#-----------------------------------------------------------------------------
CHECK_INCLUDE_FILE_CONCAT ("sys/file.h"      ${HDF_PREFIX}_HAVE_SYS_FILE_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/ioctl.h"     ${HDF_PREFIX}_HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/resource.h"  ${HDF_PREFIX}_HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/stat.h"      ${HDF_PREFIX}_HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/time.h"      ${HDF_PREFIX}_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/types.h"     ${HDF_PREFIX}_HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CONCAT ("features.h"      ${HDF_PREFIX}_HAVE_FEATURES_H)
CHECK_INCLUDE_FILE_CONCAT ("dirent.h"        ${HDF_PREFIX}_HAVE_DIRENT_H)
CHECK_INCLUDE_FILE_CONCAT ("setjmp.h"        ${HDF_PREFIX}_HAVE_SETJMP_H)
CHECK_INCLUDE_FILE_CONCAT ("stddef.h"        ${HDF_PREFIX}_HAVE_STDDEF_H)
CHECK_INCLUDE_FILE_CONCAT ("stdint.h"        ${HDF_PREFIX}_HAVE_STDINT_H)
CHECK_INCLUDE_FILE_CONCAT ("unistd.h"        ${HDF_PREFIX}_HAVE_UNISTD_H)

# Darwin
CHECK_INCLUDE_FILE_CONCAT ("mach/mach_time.h" ${HDF_PREFIX}_HAVE_MACH_MACH_TIME_H)

# Windows
CHECK_INCLUDE_FILE_CONCAT ("io.h"            ${HDF_PREFIX}_HAVE_IO_H)
if (NOT CYGWIN)
  CHECK_INCLUDE_FILE_CONCAT ("winsock2.h"      ${HDF_PREFIX}_HAVE_WINSOCK2_H)
endif ()

CHECK_INCLUDE_FILE_CONCAT ("pthread.h"       ${HDF_PREFIX}_HAVE_PTHREAD_H)
CHECK_INCLUDE_FILE_CONCAT ("dlfcn.h"         ${HDF_PREFIX}_HAVE_DLFCN_H)
CHECK_INCLUDE_FILE_CONCAT ("inttypes.h"      ${HDF_PREFIX}_HAVE_INTTYPES_H)
# _Bool type support
CHECK_INCLUDE_FILE_CONCAT (stdbool.h    ${HDF_PREFIX}_HAVE_STDBOOL_H)

## Check for non-standard extenstion quadmath.h

CHECK_INCLUDE_FILES(quadmath.h C_HAVE_QUADMATH)
if (${C_HAVE_QUADMATH})
  set(${HDF_PREFIX}_HAVE_QUADMATH_H 1)
else ()
  set(${HDF_PREFIX}_HAVE_QUADMATH_H 0)
endif ()

if (CYGWIN)
  set (${HDF_PREFIX}_HAVE_LSEEK64 0)
endif ()

# For other tests to use the same libraries
set (HDF5_REQUIRED_LIBRARIES ${HDF5_REQUIRED_LIBRARIES} ${LINK_LIBS})

set (USE_INCLUDES "")
if (WINDOWS)
  set (USE_INCLUDES ${USE_INCLUDES} "windows.h")
endif ()

# For other specific tests, use this MACRO.
macro (HDF_FUNCTION_TEST OTHER_TEST)
  if (NOT DEFINED ${HDF_PREFIX}_${OTHER_TEST})
    set (MACRO_CHECK_FUNCTION_DEFINITIONS "-D${OTHER_TEST} ${CMAKE_REQUIRED_FLAGS}")

    foreach (def
        HAVE_SYS_TIME_H
        HAVE_UNISTD_H
        HAVE_SYS_TYPES_H
    )
      if ("${${HDF_PREFIX}_${def}}")
        set (MACRO_CHECK_FUNCTION_DEFINITIONS "${MACRO_CHECK_FUNCTION_DEFINITIONS} -D${def}")
      endif ()
    endforeach ()

    if (LARGEFILE)
      set (MACRO_CHECK_FUNCTION_DEFINITIONS
          "${MACRO_CHECK_FUNCTION_DEFINITIONS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE"
      )
    endif ()

    #message (STATUS "Performing ${OTHER_TEST}")
    try_compile (${OTHER_TEST}
        ${CMAKE_BINARY_DIR}
        ${HDF_RESOURCES_EXT_DIR}/HDFTests.c
        COMPILE_DEFINITIONS "${MACRO_CHECK_FUNCTION_DEFINITIONS}"
        LINK_LIBRARIES "${HDF5_REQUIRED_LIBRARIES}"
        OUTPUT_VARIABLE OUTPUT
    )
    if (${OTHER_TEST})
      set (${HDF_PREFIX}_${OTHER_TEST} 1 CACHE INTERNAL "Other test ${FUNCTION}")
      message (STATUS "Performing Other Test ${OTHER_TEST} - Success")
    else ()
      message (STATUS "Performing Other Test ${OTHER_TEST} - Failed")
      set (${HDF_PREFIX}_${OTHER_TEST} "" CACHE INTERNAL "Other test ${FUNCTION}")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Performing Other Test ${OTHER_TEST} failed with the following output:\n"
          "${OUTPUT}\n"
      )
    endif ()
  endif ()
endmacro ()

#-----------------------------------------------------------------------------
# Check for these functions before the time headers are checked
#-----------------------------------------------------------------------------
HDF_FUNCTION_TEST (STDC_HEADERS)

#-----------------------------------------------------------------------------
#  Check for large file support
#-----------------------------------------------------------------------------

# The linux-lfs option is deprecated.
set (LINUX_LFS 0)

set (HDF_EXTRA_C_FLAGS)
set (HDF_EXTRA_FLAGS)
if (MINGW OR NOT WINDOWS)
  # Might want to check explicitly for Linux and possibly Cygwin
  # instead of checking for not Solaris or Darwin.
  if (NOT ${HDF_PREFIX}_HAVE_SOLARIS AND NOT ${HDF_PREFIX}_HAVE_DARWIN)
  # Linux Specific flags
  # This was originally defined as _POSIX_SOURCE which was updated to
  # _POSIX_C_SOURCE=199506L to expose a greater amount of POSIX
  # functionality so clock_gettime and CLOCK_MONOTONIC are defined
  # correctly. This was later updated to 200112L so that
  # posix_memalign() is visible for the direct VFD code on Linux
  # systems.
  # POSIX feature information can be found in the gcc manual at:
  # http://www.gnu.org/s/libc/manual/html_node/Feature-Test-Macros.html
  set (HDF_EXTRA_C_FLAGS -D_POSIX_C_SOURCE=200112L)

  # Need to add this so that O_DIRECT is visible for the direct
  # VFD on Linux systems.
  set (HDF_EXTRA_C_FLAGS -D_GNU_SOURCE)

  if (FALSE) # XXX(kitware): Hardcode settings.
  option (HDF_ENABLE_LARGE_FILE "Enable support for large (64-bit) files on Linux." ON)
  else ()
  set(HDF_ENABLE_LARGE_FILE ON)
  endif ()
  if (HDF_ENABLE_LARGE_FILE AND NOT DEFINED TEST_LFS_WORKS_RUN)
    set (msg "Performing TEST_LFS_WORKS")
    try_run (TEST_LFS_WORKS_RUN   TEST_LFS_WORKS_COMPILE
        ${CMAKE_BINARY_DIR}
        ${HDF_RESOURCES_EXT_DIR}/HDFTests.c
        COMPILE_DEFINITIONS "-DTEST_LFS_WORKS"
    )

    # The LARGEFILE definitions were from the transition period
    # and are probably no longer needed. The FILE_OFFSET_BITS
    # check should be generalized for all POSIX systems as it
    # is in the Autotools.
    if (TEST_LFS_WORKS_COMPILE)
      if (TEST_LFS_WORKS_RUN MATCHES 0)
        set (TEST_LFS_WORKS 1 CACHE INTERNAL ${msg})
        set (LARGEFILE 1)
        set (HDF_EXTRA_FLAGS ${HDF_EXTRA_FLAGS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE)
        message (STATUS "${msg}... yes")
      else ()
        set (TEST_LFS_WORKS "" CACHE INTERNAL ${msg})
        message (STATUS "${msg}... no")
        file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
              "Test TEST_LFS_WORKS Run failed with the following exit code:\n ${TEST_LFS_WORKS_RUN}\n"
        )
      endif ()
    else ()
      set (TEST_LFS_WORKS "" CACHE INTERNAL ${msg})
      message (STATUS "${msg}... no")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Test TEST_LFS_WORKS Compile failed\n"
      )
    endif ()
  endif ()
  set (CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} ${HDF_EXTRA_FLAGS})
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Check for HAVE_OFF64_T functionality
#-----------------------------------------------------------------------------
if (MINGW OR NOT WINDOWS)
  HDF_FUNCTION_TEST (HAVE_OFF64_T)
  if (${HDF_PREFIX}_HAVE_OFF64_T)
    CHECK_FUNCTION_EXISTS (lseek64            ${HDF_PREFIX}_HAVE_LSEEK64)
  endif ()

  CHECK_FUNCTION_EXISTS (fseeko               ${HDF_PREFIX}_HAVE_FSEEKO)

  HDF_FUNCTION_TEST (HAVE_STAT64_STRUCT)
  if (HAVE_STAT64_STRUCT)
    CHECK_FUNCTION_EXISTS (stat64             ${HDF_PREFIX}_HAVE_STAT64)
  endif ()
endif ()

#-----------------------------------------------------------------------------
#  Check the size in bytes of all the int and float types
#-----------------------------------------------------------------------------
macro (HDF_CHECK_TYPE_SIZE type var)
  set (aType ${type})
  set (aVar  ${var})
#  message (STATUS "Checking size of ${aType} and storing into ${aVar}")
  CHECK_TYPE_SIZE (${aType}   ${aVar})
  if (NOT ${aVar})
    set (${aVar} 0 CACHE INTERNAL "SizeOf for ${aType}")
#    message (STATUS "Size of ${aType} was NOT Found")
  endif ()
endmacro ()

HDF_CHECK_TYPE_SIZE (char           ${HDF_PREFIX}_SIZEOF_CHAR)
HDF_CHECK_TYPE_SIZE (short          ${HDF_PREFIX}_SIZEOF_SHORT)
HDF_CHECK_TYPE_SIZE (int            ${HDF_PREFIX}_SIZEOF_INT)
HDF_CHECK_TYPE_SIZE (unsigned       ${HDF_PREFIX}_SIZEOF_UNSIGNED)
if (NOT APPLE)
  HDF_CHECK_TYPE_SIZE (long         ${HDF_PREFIX}_SIZEOF_LONG)
endif ()
HDF_CHECK_TYPE_SIZE ("long long"    ${HDF_PREFIX}_SIZEOF_LONG_LONG)
HDF_CHECK_TYPE_SIZE (__int64        ${HDF_PREFIX}_SIZEOF___INT64)
if (NOT ${HDF_PREFIX}_SIZEOF___INT64)
  set (${HDF_PREFIX}_SIZEOF___INT64 0)
endif ()

HDF_CHECK_TYPE_SIZE (float          ${HDF_PREFIX}_SIZEOF_FLOAT)
HDF_CHECK_TYPE_SIZE (double         ${HDF_PREFIX}_SIZEOF_DOUBLE)
HDF_CHECK_TYPE_SIZE ("long double"  ${HDF_PREFIX}_SIZEOF_LONG_DOUBLE)

HDF_CHECK_TYPE_SIZE (int8_t         ${HDF_PREFIX}_SIZEOF_INT8_T)
HDF_CHECK_TYPE_SIZE (uint8_t        ${HDF_PREFIX}_SIZEOF_UINT8_T)
HDF_CHECK_TYPE_SIZE (int_least8_t   ${HDF_PREFIX}_SIZEOF_INT_LEAST8_T)
HDF_CHECK_TYPE_SIZE (uint_least8_t  ${HDF_PREFIX}_SIZEOF_UINT_LEAST8_T)
HDF_CHECK_TYPE_SIZE (int_fast8_t    ${HDF_PREFIX}_SIZEOF_INT_FAST8_T)
HDF_CHECK_TYPE_SIZE (uint_fast8_t   ${HDF_PREFIX}_SIZEOF_UINT_FAST8_T)

HDF_CHECK_TYPE_SIZE (int16_t        ${HDF_PREFIX}_SIZEOF_INT16_T)
HDF_CHECK_TYPE_SIZE (uint16_t       ${HDF_PREFIX}_SIZEOF_UINT16_T)
HDF_CHECK_TYPE_SIZE (int_least16_t  ${HDF_PREFIX}_SIZEOF_INT_LEAST16_T)
HDF_CHECK_TYPE_SIZE (uint_least16_t ${HDF_PREFIX}_SIZEOF_UINT_LEAST16_T)
HDF_CHECK_TYPE_SIZE (int_fast16_t   ${HDF_PREFIX}_SIZEOF_INT_FAST16_T)
HDF_CHECK_TYPE_SIZE (uint_fast16_t  ${HDF_PREFIX}_SIZEOF_UINT_FAST16_T)

HDF_CHECK_TYPE_SIZE (int32_t        ${HDF_PREFIX}_SIZEOF_INT32_T)
HDF_CHECK_TYPE_SIZE (uint32_t       ${HDF_PREFIX}_SIZEOF_UINT32_T)
HDF_CHECK_TYPE_SIZE (int_least32_t  ${HDF_PREFIX}_SIZEOF_INT_LEAST32_T)
HDF_CHECK_TYPE_SIZE (uint_least32_t ${HDF_PREFIX}_SIZEOF_UINT_LEAST32_T)
HDF_CHECK_TYPE_SIZE (int_fast32_t   ${HDF_PREFIX}_SIZEOF_INT_FAST32_T)
HDF_CHECK_TYPE_SIZE (uint_fast32_t  ${HDF_PREFIX}_SIZEOF_UINT_FAST32_T)

HDF_CHECK_TYPE_SIZE (int64_t        ${HDF_PREFIX}_SIZEOF_INT64_T)
HDF_CHECK_TYPE_SIZE (uint64_t       ${HDF_PREFIX}_SIZEOF_UINT64_T)
HDF_CHECK_TYPE_SIZE (int_least64_t  ${HDF_PREFIX}_SIZEOF_INT_LEAST64_T)
HDF_CHECK_TYPE_SIZE (uint_least64_t ${HDF_PREFIX}_SIZEOF_UINT_LEAST64_T)
HDF_CHECK_TYPE_SIZE (int_fast64_t   ${HDF_PREFIX}_SIZEOF_INT_FAST64_T)
HDF_CHECK_TYPE_SIZE (uint_fast64_t  ${HDF_PREFIX}_SIZEOF_UINT_FAST64_T)

if (NOT APPLE)
  HDF_CHECK_TYPE_SIZE (size_t       ${HDF_PREFIX}_SIZEOF_SIZE_T)
  HDF_CHECK_TYPE_SIZE (ssize_t      ${HDF_PREFIX}_SIZEOF_SSIZE_T)
  if (NOT ${HDF_PREFIX}_SIZEOF_SSIZE_T)
    set (${HDF_PREFIX}_SIZEOF_SSIZE_T 0)
  endif ()
  if (MINGW OR NOT WINDOWS)
    HDF_CHECK_TYPE_SIZE (ptrdiff_t    ${HDF_PREFIX}_SIZEOF_PTRDIFF_T)
  endif ()
endif ()

HDF_CHECK_TYPE_SIZE (off_t          ${HDF_PREFIX}_SIZEOF_OFF_T)
HDF_CHECK_TYPE_SIZE (off64_t        ${HDF_PREFIX}_SIZEOF_OFF64_T)
if (NOT ${HDF_PREFIX}_SIZEOF_OFF64_T)
  set (${HDF_PREFIX}_SIZEOF_OFF64_T 0)
endif ()
HDF_CHECK_TYPE_SIZE (time_t          ${HDF_PREFIX}_SIZEOF_TIME_T)

#-----------------------------------------------------------------------------
# Extra C99 types
#-----------------------------------------------------------------------------

# _Bool type support
if (HAVE_STDBOOL_H)
  set (CMAKE_EXTRA_INCLUDE_FILES stdbool.h)
  HDF_CHECK_TYPE_SIZE (bool         ${HDF_PREFIX}_SIZEOF_BOOL)
else ()
  HDF_CHECK_TYPE_SIZE (_Bool        ${HDF_PREFIX}_SIZEOF_BOOL)
endif ()

if (MINGW OR NOT WINDOWS)
  #-----------------------------------------------------------------------------
  # Check if the dev_t type is a scalar type
  #-----------------------------------------------------------------------------
  HDF_FUNCTION_TEST (DEV_T_IS_SCALAR)

  #-----------------------------------------------------------------------------
  # Check a bunch of time functions
  #-----------------------------------------------------------------------------
  CHECK_FUNCTION_EXISTS (gettimeofday      ${HDF_PREFIX}_HAVE_GETTIMEOFDAY)
  foreach (time_test
      HAVE_TM_GMTOFF
      HAVE___TM_GMTOFF
#      HAVE_TIMEZONE
      HAVE_STRUCT_TIMEZONE
      GETTIMEOFDAY_GIVES_TZ
      TIME_WITH_SYS_TIME
      HAVE_TM_ZONE
      HAVE_STRUCT_TM_TM_ZONE
  )
    HDF_FUNCTION_TEST (${time_test})
  endforeach ()
  if (NOT CYGWIN AND NOT MINGW)
      HDF_FUNCTION_TEST (HAVE_TIMEZONE)
#      HDF_FUNCTION_TEST (HAVE_STAT_ST_BLOCKS)
  endif ()

  # ----------------------------------------------------------------------
  # Does the struct stat have the st_blocks field?  This field is not Posix.
  #
  HDF_FUNCTION_TEST (HAVE_STAT_ST_BLOCKS)

  # ----------------------------------------------------------------------
  # How do we figure out the width of a tty in characters?
  #
  CHECK_FUNCTION_EXISTS (ioctl             ${HDF_PREFIX}_HAVE_IOCTL)
  HDF_FUNCTION_TEST (HAVE_STRUCT_VIDEOCONFIG)
  HDF_FUNCTION_TEST (HAVE_STRUCT_TEXT_INFO)
  CHECK_FUNCTION_EXISTS (_getvideoconfig   ${HDF_PREFIX}_HAVE__GETVIDEOCONFIG)
  CHECK_FUNCTION_EXISTS (gettextinfo       ${HDF_PREFIX}_HAVE_GETTEXTINFO)
  CHECK_FUNCTION_EXISTS (_scrsize          ${HDF_PREFIX}_HAVE__SCRSIZE)
  if (NOT CYGWIN)
    CHECK_FUNCTION_EXISTS (GetConsoleScreenBufferInfo    ${HDF_PREFIX}_HAVE_GETCONSOLESCREENBUFFERINFO)
  endif ()
  CHECK_SYMBOL_EXISTS (TIOCGWINSZ "sys/ioctl.h" ${HDF_PREFIX}_HAVE_TIOCGWINSZ)
  CHECK_SYMBOL_EXISTS (TIOCGETD   "sys/ioctl.h" ${HDF_PREFIX}_HAVE_TIOCGETD)

  # ----------------------------------------------------------------------
  # cygwin user credentials are different then on linux
  #
  if (NOT CYGWIN AND NOT MINGW)
    CHECK_FUNCTION_EXISTS (getpwuid        ${HDF_PREFIX}_HAVE_GETPWUID)
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Check for some functions that are used
#
CHECK_FUNCTION_EXISTS (alarm             ${HDF_PREFIX}_HAVE_ALARM)
CHECK_FUNCTION_EXISTS (fcntl             ${HDF_PREFIX}_HAVE_FCNTL)
CHECK_FUNCTION_EXISTS (flock             ${HDF_PREFIX}_HAVE_FLOCK)
CHECK_FUNCTION_EXISTS (fork              ${HDF_PREFIX}_HAVE_FORK)
CHECK_FUNCTION_EXISTS (frexpf            ${HDF_PREFIX}_HAVE_FREXPF)
CHECK_FUNCTION_EXISTS (frexpl            ${HDF_PREFIX}_HAVE_FREXPL)

CHECK_FUNCTION_EXISTS (gethostname       ${HDF_PREFIX}_HAVE_GETHOSTNAME)
CHECK_FUNCTION_EXISTS (getrusage         ${HDF_PREFIX}_HAVE_GETRUSAGE)

CHECK_FUNCTION_EXISTS (pread             ${HDF_PREFIX}_HAVE_PREAD)
CHECK_FUNCTION_EXISTS (pwrite            ${HDF_PREFIX}_HAVE_PWRITE)
CHECK_FUNCTION_EXISTS (rand_r            ${HDF_PREFIX}_HAVE_RAND_R)
CHECK_FUNCTION_EXISTS (random            ${HDF_PREFIX}_HAVE_RANDOM)
CHECK_FUNCTION_EXISTS (setsysinfo        ${HDF_PREFIX}_HAVE_SETSYSINFO)

CHECK_FUNCTION_EXISTS (signal            ${HDF_PREFIX}_HAVE_SIGNAL)
CHECK_FUNCTION_EXISTS (longjmp           ${HDF_PREFIX}_HAVE_LONGJMP)
CHECK_FUNCTION_EXISTS (setjmp            ${HDF_PREFIX}_HAVE_SETJMP)
CHECK_FUNCTION_EXISTS (siglongjmp        ${HDF_PREFIX}_HAVE_SIGLONGJMP)
CHECK_FUNCTION_EXISTS (sigsetjmp         ${HDF_PREFIX}_HAVE_SIGSETJMP)
CHECK_FUNCTION_EXISTS (sigprocmask       ${HDF_PREFIX}_HAVE_SIGPROCMASK)

CHECK_FUNCTION_EXISTS (strdup            ${HDF_PREFIX}_HAVE_STRDUP)
CHECK_FUNCTION_EXISTS (strtoll           ${HDF_PREFIX}_HAVE_STRTOLL)
CHECK_FUNCTION_EXISTS (strtoull          ${HDF_PREFIX}_HAVE_STRTOULL)
CHECK_FUNCTION_EXISTS (symlink           ${HDF_PREFIX}_HAVE_SYMLINK)
CHECK_FUNCTION_EXISTS (system            ${HDF_PREFIX}_HAVE_SYSTEM)

CHECK_FUNCTION_EXISTS (tmpfile           ${HDF_PREFIX}_HAVE_TMPFILE)
CHECK_FUNCTION_EXISTS (asprintf          ${HDF_PREFIX}_HAVE_ASPRINTF)
CHECK_FUNCTION_EXISTS (vasprintf         ${HDF_PREFIX}_HAVE_VASPRINTF)
CHECK_FUNCTION_EXISTS (waitpid           ${HDF_PREFIX}_HAVE_WAITPID)

#-----------------------------------------------------------------------------
# sigsetjmp is special; may actually be a macro
#-----------------------------------------------------------------------------
if (NOT ${HDF_PREFIX}_HAVE_SIGSETJMP)
  if (${HDF_PREFIX}_HAVE_SETJMP_H)
    CHECK_SYMBOL_EXISTS (sigsetjmp "setjmp.h" ${HDF_PREFIX}_HAVE_MACRO_SIGSETJMP)
    if (${HDF_PREFIX}_HAVE_MACRO_SIGSETJMP)
      set (${HDF_PREFIX}_HAVE_SIGSETJMP 1)
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Check a bunch of other functions
#-----------------------------------------------------------------------------
if (MINGW OR NOT WINDOWS)
  foreach (other_test
      HAVE_ATTRIBUTE
      HAVE_C99_FUNC
#      STDC_HEADERS
      HAVE_FUNCTION
      HAVE_C99_DESIGNATED_INITIALIZER
      SYSTEM_SCOPE_THREADS
      HAVE_SOCKLEN_T
  )
    HDF_FUNCTION_TEST (${other_test})
  endforeach ()
endif ()

#-----------------------------------------------------------------------------
# Check if InitOnceExecuteOnce is available
#-----------------------------------------------------------------------------
if (WINDOWS)
  if (NOT HDF_NO_IOEO_TEST)
  #message (STATUS "Checking for InitOnceExecuteOnce:")
  if (NOT DEFINED ${HDF_PREFIX}_HAVE_IOEO)
    if (LARGEFILE)
      set (CMAKE_REQUIRED_DEFINITIONS
          "${CURRENT_TEST_DEFINITIONS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE"
      )
    endif ()
    set (MACRO_CHECK_FUNCTION_DEFINITIONS "-DHAVE_IOEO ${CMAKE_REQUIRED_FLAGS}")
    if (CMAKE_REQUIRED_INCLUDES)
      set (CHECK_C_SOURCE_COMPILES_ADD_INCLUDES "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
    else ()
      set (CHECK_C_SOURCE_COMPILES_ADD_INCLUDES)
    endif ()

    TRY_RUN(HAVE_IOEO_EXITCODE HAVE_IOEO_COMPILED
        ${CMAKE_BINARY_DIR}
        ${HDF_RESOURCES_EXT_DIR}/HDFTests.c
        COMPILE_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} ${MACRO_CHECK_FUNCTION_DEFINITIONS}"
        LINK_LIBRARIES "${HDF5_REQUIRED_LIBRARIES}"
        CMAKE_FLAGS "${CHECK_C_SOURCE_COMPILES_ADD_INCLUDES} -DCMAKE_SKIP_RPATH:BOOL=${CMAKE_SKIP_RPATH}"
        COMPILE_OUTPUT_VARIABLE OUTPUT
    )
    # if it did not compile make the return value fail code of 1
    if (NOT HAVE_IOEO_COMPILED)
      set (HAVE_IOEO_EXITCODE 1)
    endif ()
    # if the return value was 0 then it worked
    if ("${HAVE_IOEO_EXITCODE}" EQUAL 0)
      set (${HDF_PREFIX}_HAVE_IOEO 1 CACHE INTERNAL "Test InitOnceExecuteOnce")
      message (STATUS "Performing Test InitOnceExecuteOnce - Success")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Performing C SOURCE FILE Test InitOnceExecuteOnce succeded with the following output:\n"
        "${OUTPUT}\n"
        "Return value: ${HAVE_IOEO}\n")
    else ()
      if (CMAKE_CROSSCOMPILING AND "${HAVE_IOEO_EXITCODE}" MATCHES  "FAILED_TO_RUN")
        set (${HDF_PREFIX}_HAVE_IOEO "${HAVE_IOEO_EXITCODE}")
      else ()
        set (${HDF_PREFIX}_HAVE_IOEO "" CACHE INTERNAL "Test InitOnceExecuteOnce")
      endif ()

      message (STATUS "Performing Test InitOnceExecuteOnce - Failed")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Performing InitOnceExecuteOnce Test  failed with the following output:\n"
        "${OUTPUT}\n"
        "Return value: ${HAVE_IOEO_EXITCODE}\n")
    endif ()
  endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Determine how 'inline' is used
#-----------------------------------------------------------------------------
foreach (inline_test inline __inline__ __inline)
  string (TOUPPER ${inline_test} INLINE_TEST_MACRO)
  HDF_FUNCTION_TEST (HAVE_${INLINE_TEST_MACRO})
endforeach ()

#-----------------------------------------------------------------------------
# Check how to print a Long Long integer
#-----------------------------------------------------------------------------
if (NOT ${HDF_PREFIX}_PRINTF_LL_WIDTH OR ${HDF_PREFIX}_PRINTF_LL_WIDTH MATCHES "unknown")
  set (PRINT_LL_FOUND 0)
  message (STATUS "Checking for appropriate format for 64 bit long:")
  set (CURRENT_TEST_DEFINITIONS "-DPRINTF_LL_WIDTH")
  if (${HDF_PREFIX}_SIZEOF_LONG_LONG)
    set (CURRENT_TEST_DEFINITIONS "${CURRENT_TEST_DEFINITIONS} -DHAVE_LONG_LONG")
  endif ()
  TRY_RUN (${HDF_PREFIX}_PRINTF_LL_TEST_RUN   ${HDF_PREFIX}_PRINTF_LL_TEST_COMPILE
      ${CMAKE_BINARY_DIR}
      ${HDF_RESOURCES_EXT_DIR}/HDFTests.c
      COMPILE_DEFINITIONS "${CURRENT_TEST_DEFINITIONS}"
      RUN_OUTPUT_VARIABLE OUTPUT
  )
  if (${HDF_PREFIX}_PRINTF_LL_TEST_COMPILE)
    if (${HDF_PREFIX}_PRINTF_LL_TEST_RUN MATCHES 0)
      string(REGEX REPLACE ".*PRINTF_LL_WIDTH=\\[(.*)\\].*" "\\1" ${HDF_PREFIX}_PRINTF_LL "${OUTPUT}")
      set (${HDF_PREFIX}_PRINTF_LL_WIDTH "\"${${HDF_PREFIX}_PRINTF_LL}\"" CACHE INTERNAL "Width for printf for type `long long' or `__int64', us. `ll")
      set (PRINT_LL_FOUND 1)
    else ()
      message (STATUS "Width test failed with result: ${${HDF_PREFIX}_PRINTF_LL_TEST_RUN}")
    endif ()
  else ()
    file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Test ${HDF_PREFIX}_PRINTF_LL_WIDTH failed\n"
    )
  endif ()

  if (PRINT_LL_FOUND)
    message (STATUS "Checking for appropriate format for 64 bit long: found ${${HDF_PREFIX}_PRINTF_LL_WIDTH}")
  else ()
    message (STATUS "Checking for appropriate format for 64 bit long: not found")
    set (${HDF_PREFIX}_PRINTF_LL_WIDTH "\"unknown\"" CACHE INTERNAL
        "Width for printf for type `long long' or `__int64', us. `ll"
    )
  endif ()
endif ()

# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can handle converting
# denormalized floating-point values.
# (This flag should be set for all machines, except for the Crays, where
# the cache value is set in it's config file)
#
set (${HDF_PREFIX}_CONVERT_DENORMAL_FLOAT 1)
