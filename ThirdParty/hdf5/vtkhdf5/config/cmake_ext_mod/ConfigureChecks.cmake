#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the COPYING file, which can be found at the root of the source code
# distribution tree, or in https://www.hdfgroup.org/licenses.
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
include (CheckStructHasMember)

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
  set (__USE_MINGW_ANSI_STDIO 1)
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
  if (NOT MINGW)
    set (${HDF_PREFIX}_HAVE_GETHOSTNAME 1)
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
CHECK_INCLUDE_FILE_CONCAT ("sys/socket.h"    ${HDF_PREFIX}_HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/stat.h"      ${HDF_PREFIX}_HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/time.h"      ${HDF_PREFIX}_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CONCAT ("sys/types.h"     ${HDF_PREFIX}_HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CONCAT ("features.h"      ${HDF_PREFIX}_HAVE_FEATURES_H)
CHECK_INCLUDE_FILE_CONCAT ("dirent.h"        ${HDF_PREFIX}_HAVE_DIRENT_H)
CHECK_INCLUDE_FILE_CONCAT ("unistd.h"        ${HDF_PREFIX}_HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CONCAT ("pwd.h"           ${HDF_PREFIX}_HAVE_PWD_H)
CHECK_INCLUDE_FILE_CONCAT ("globus/common.h" ${HDF_PREFIX}_HAVE_GLOBUS_COMMON_H)
CHECK_INCLUDE_FILE_CONCAT ("pdb.h"           ${HDF_PREFIX}_HAVE_PDB_H)
CHECK_INCLUDE_FILE_CONCAT ("pthread.h"       ${HDF_PREFIX}_HAVE_PTHREAD_H)
CHECK_INCLUDE_FILE_CONCAT ("dlfcn.h"         ${HDF_PREFIX}_HAVE_DLFCN_H)
CHECK_INCLUDE_FILE_CONCAT ("netinet/in.h"    ${HDF_PREFIX}_HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILE_CONCAT ("netdb.h"         ${HDF_PREFIX}_HAVE_NETDB_H)
CHECK_INCLUDE_FILE_CONCAT ("arpa/inet.h"     ${HDF_PREFIX}_HAVE_ARPA_INET_H)

## Check for non-standard extension quadmath.h

CHECK_INCLUDE_FILES(quadmath.h C_HAVE_QUADMATH)
if (${C_HAVE_QUADMATH})
  set(${HDF_PREFIX}_HAVE_QUADMATH_H 1)
else ()
  set(${HDF_PREFIX}_HAVE_QUADMATH_H 0)
endif ()

if (CYGWIN)
  set (${HDF_PREFIX}_HAVE_LSEEK64 0)
endif ()

if (MINGW OR NOT WINDOWS)
  CHECK_LIBRARY_EXISTS_CONCAT ("m" ceil     ${HDF_PREFIX}_HAVE_LIBM)
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

    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
      message (TRACE "Performing ${OTHER_TEST}")
    endif ()
    try_compile (${OTHER_TEST}
        ${CMAKE_BINARY_DIR}
        ${HDF_RESOURCES_EXT_DIR}/HDFTests.c
        COMPILE_DEFINITIONS "${MACRO_CHECK_FUNCTION_DEFINITIONS}"
        LINK_LIBRARIES "${HDF5_REQUIRED_LIBRARIES}"
        OUTPUT_VARIABLE OUTPUT
    )
    if (${OTHER_TEST})
      set (${HDF_PREFIX}_${OTHER_TEST} 1 CACHE INTERNAL "Other test ${FUNCTION}")
      if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
        message (VERBOSE "Performing Other Test ${OTHER_TEST} - Success")
      endif ()
    else ()
      if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
        message (VERBOSE "Performing Other Test ${OTHER_TEST} - Failed")
      endif ()
      set (${HDF_PREFIX}_${OTHER_TEST} "" CACHE INTERNAL "Other test ${FUNCTION}")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Performing Other Test ${OTHER_TEST} failed with the following output:\n"
          "${OUTPUT}\n"
      )
    endif ()
  endif ()
endmacro ()

#-----------------------------------------------------------------------------
#  Check for large file support
#-----------------------------------------------------------------------------

# The linux-lfs option is deprecated.
set (LINUX_LFS 0)

set (HDF_EXTRA_C_FLAGS)
set (HDF_EXTRA_FLAGS)
if (MINGW OR NOT WINDOWS)
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Linux Specific flags
    # This was originally defined as _POSIX_SOURCE which was updated to
    # _POSIX_C_SOURCE=199506L to expose a greater amount of POSIX
    # functionality so clock_gettime and CLOCK_MONOTONIC are defined
    # correctly. This was later updated to 200112L so that
    # posix_memalign() is visible for the direct VFD code on Linux
    # systems.
    # POSIX feature information can be found in the gcc manual at:
    # http://www.gnu.org/s/libc/manual/html_node/Feature-Test-Macros.html
    set (HDF_EXTRA_C_FLAGS -D_POSIX_C_SOURCE=200809L)

    # Need to add this so that O_DIRECT is visible for the direct
    # VFD on Linux systems.
    set (HDF_EXTRA_C_FLAGS ${HDF_EXTRA_C_FLAGS} -D_GNU_SOURCE)

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
          if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
            message (VERBOSE "${msg}... yes")
          endif ()
        else ()
          set (TEST_LFS_WORKS "" CACHE INTERNAL ${msg})
          if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
            message (VERBOSE "${msg}... no")
          endif ()
          file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
                "Test TEST_LFS_WORKS Run failed with the following exit code:\n ${TEST_LFS_WORKS_RUN}\n"
          )
        endif ()
      else ()
        set (TEST_LFS_WORKS "" CACHE INTERNAL ${msg})
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
            message (VERBOSE "${msg}... no")
        endif ()
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

  CHECK_STRUCT_HAS_MEMBER("struct stat64" st_blocks "sys/types.h;sys/stat.h" HAVE_STAT64_STRUCT)
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
  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
    message (TRACE "Checking size of ${aType} and storing into ${aVar}")
  endif ()
  CHECK_TYPE_SIZE (${aType}   ${aVar})
  if (NOT ${aVar})
    set (${aVar} 0 CACHE INTERNAL "SizeOf for ${aType}")
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
      message (TRACE "Size of ${aType} was NOT Found")
    endif ()
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

HDF_CHECK_TYPE_SIZE (size_t       ${HDF_PREFIX}_SIZEOF_SIZE_T)
HDF_CHECK_TYPE_SIZE (ssize_t      ${HDF_PREFIX}_SIZEOF_SSIZE_T)
if (NOT ${HDF_PREFIX}_SIZEOF_SSIZE_T)
  set (${HDF_PREFIX}_SIZEOF_SSIZE_T 0)
endif ()
if (MINGW OR NOT WINDOWS)
  HDF_CHECK_TYPE_SIZE (ptrdiff_t    ${HDF_PREFIX}_SIZEOF_PTRDIFF_T)
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

# Size of bool
set (CMAKE_EXTRA_INCLUDE_FILES stdbool.h)
HDF_CHECK_TYPE_SIZE (_Bool        ${HDF_PREFIX}_SIZEOF_BOOL)

if (MINGW OR NOT WINDOWS)
  #-----------------------------------------------------------------------------
  # Check if the dev_t type is a scalar type
  #-----------------------------------------------------------------------------
  HDF_FUNCTION_TEST (DEV_T_IS_SCALAR)

  #-----------------------------------------------------------------------------
  # Check a bunch of time functions
  #-----------------------------------------------------------------------------
  CHECK_STRUCT_HAS_MEMBER("struct tm" tm_gmtoff "time.h" ${HDF_PREFIX}_HAVE_TM_GMTOFF)
  CHECK_STRUCT_HAS_MEMBER("struct tm" __tm_gmtoff "time.h" ${HDF_PREFIX}_HAVE___TM_GMTOFF)
  if (${HDF_PREFIX}_HAVE_SYS_TIME_H)
    CHECK_STRUCT_HAS_MEMBER("struct tm" tz_minuteswest "sys/types.h;sys/time.h;time.h" ${HDF_PREFIX}_HAVE_STRUCT_TIMEZONE)
  else ()
    CHECK_STRUCT_HAS_MEMBER("struct tm" tz_minuteswest "sys/types.h;time.h" ${HDF_PREFIX}_HAVE_STRUCT_TIMEZONE)
  endif ()
  CHECK_FUNCTION_EXISTS (gettimeofday      ${HDF_PREFIX}_HAVE_GETTIMEOFDAY)
  foreach (time_test
#      HAVE_TIMEZONE
      GETTIMEOFDAY_GIVES_TZ
      HAVE_TM_ZONE
      HAVE_STRUCT_TM_TM_ZONE
  )
    HDF_FUNCTION_TEST (${time_test})
  endforeach ()
  if (NOT CYGWIN AND NOT MINGW)
      HDF_FUNCTION_TEST (HAVE_TIMEZONE)
  endif ()

  # ----------------------------------------------------------------------
  # Does the struct stat have the st_blocks field?  This field is not Posix.
  #
  CHECK_STRUCT_HAS_MEMBER("struct stat" st_blocks "sys/types.h;sys/stat.h" ${HDF_PREFIX}_HAVE_STAT_ST_BLOCKS)

  # ----------------------------------------------------------------------
  # How do we figure out the width of a tty in characters?
  #
  CHECK_FUNCTION_EXISTS (ioctl             ${HDF_PREFIX}_HAVE_IOCTL)
  CHECK_STRUCT_HAS_MEMBER ("struct videoconfig" numtextcols "" ${HDF_PREFIX}_HAVE_STRUCT_VIDEOCONFIG)
  CHECK_STRUCT_HAS_MEMBER ("struct text_info" screenwidth "" ${HDF_PREFIX}_HAVE_STRUCT_TEXT_INFO)
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

CHECK_FUNCTION_EXISTS (gethostname       ${HDF_PREFIX}_HAVE_GETHOSTNAME)
CHECK_FUNCTION_EXISTS (getrusage         ${HDF_PREFIX}_HAVE_GETRUSAGE)

CHECK_FUNCTION_EXISTS (pread             ${HDF_PREFIX}_HAVE_PREAD)
CHECK_FUNCTION_EXISTS (pwrite            ${HDF_PREFIX}_HAVE_PWRITE)
CHECK_FUNCTION_EXISTS (rand_r            ${HDF_PREFIX}_HAVE_RAND_R)
CHECK_FUNCTION_EXISTS (random            ${HDF_PREFIX}_HAVE_RANDOM)
CHECK_FUNCTION_EXISTS (setsysinfo        ${HDF_PREFIX}_HAVE_SETSYSINFO)

CHECK_FUNCTION_EXISTS (siglongjmp        ${HDF_PREFIX}_HAVE_SIGLONGJMP)
CHECK_FUNCTION_EXISTS (sigsetjmp         ${HDF_PREFIX}_HAVE_SIGSETJMP)
CHECK_FUNCTION_EXISTS (sigprocmask       ${HDF_PREFIX}_HAVE_SIGPROCMASK)

CHECK_FUNCTION_EXISTS (strdup            ${HDF_PREFIX}_HAVE_STRDUP)
CHECK_FUNCTION_EXISTS (symlink           ${HDF_PREFIX}_HAVE_SYMLINK)

CHECK_FUNCTION_EXISTS (tmpfile           ${HDF_PREFIX}_HAVE_TMPFILE)
CHECK_FUNCTION_EXISTS (asprintf          ${HDF_PREFIX}_HAVE_ASPRINTF)
CHECK_FUNCTION_EXISTS (vasprintf         ${HDF_PREFIX}_HAVE_VASPRINTF)
CHECK_FUNCTION_EXISTS (waitpid           ${HDF_PREFIX}_HAVE_WAITPID)

#-----------------------------------------------------------------------------
# sigsetjmp is special; may actually be a macro
#-----------------------------------------------------------------------------
if (NOT ${HDF_PREFIX}_HAVE_SIGSETJMP)
  CHECK_SYMBOL_EXISTS (sigsetjmp "setjmp.h" ${HDF_PREFIX}_HAVE_MACRO_SIGSETJMP)
  if (${HDF_PREFIX}_HAVE_MACRO_SIGSETJMP)
    set (${HDF_PREFIX}_HAVE_SIGSETJMP 1)
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Check a bunch of other functions
#-----------------------------------------------------------------------------
if (MINGW OR NOT WINDOWS)
  foreach (other_test
      HAVE_ATTRIBUTE
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
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
      #message (VERBOSE "Checking for InitOnceExecuteOnce:")
    endif ()
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
      if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
        message (VERBOSE "Performing Test InitOnceExecuteOnce - Success")
      endif ()
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Performing C SOURCE FILE Test InitOnceExecuteOnce succeeded with the following output:\n"
        "${OUTPUT}\n"
        "Return value: ${HAVE_IOEO}\n")
    else ()
      if (CMAKE_CROSSCOMPILING AND "${HAVE_IOEO_EXITCODE}" MATCHES  "FAILED_TO_RUN")
        set (${HDF_PREFIX}_HAVE_IOEO "${HAVE_IOEO_EXITCODE}")
      else ()
        set (${HDF_PREFIX}_HAVE_IOEO "" CACHE INTERNAL "Test InitOnceExecuteOnce")
      endif ()

      if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
        message (VERBOSE "Performing Test InitOnceExecuteOnce - Failed")
      endif ()
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Performing InitOnceExecuteOnce Test  failed with the following output:\n"
        "${OUTPUT}\n"
        "Return value: ${HAVE_IOEO_EXITCODE}\n")
    endif ()
  endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Check how to print a Long Long integer
#-----------------------------------------------------------------------------
if (NOT ${HDF_PREFIX}_PRINTF_LL_WIDTH OR ${HDF_PREFIX}_PRINTF_LL_WIDTH MATCHES "unknown")
  set (PRINT_LL_FOUND 0)
  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
    message (VERBOSE "Checking for appropriate format for 64 bit long:")
  endif ()
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
      if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
        message (VERBOSE "Width test failed with result: ${${HDF_PREFIX}_PRINTF_LL_TEST_RUN}")
      endif ()
    endif ()
  else ()
    file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Test ${HDF_PREFIX}_PRINTF_LL_WIDTH failed\n"
    )
  endif ()

  if (PRINT_LL_FOUND)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
      message (VERBOSE "Checking for appropriate format for 64 bit long: found ${${HDF_PREFIX}_PRINTF_LL_WIDTH}")
    endif ()
  else ()
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
      message (VERBOSE "Checking for appropriate format for 64 bit long: not found")
    endif ()
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
