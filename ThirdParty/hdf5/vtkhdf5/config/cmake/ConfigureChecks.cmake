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
set (HDF_PREFIX "H5")
include (${HDF_RESOURCES_EXT_DIR}/ConfigureChecks.cmake)

if (HDF5_ENABLE_USING_MEMCHECKER)
  set (${HDF_PREFIX}_USING_MEMCHECKER 1)
endif ()

#-----------------------------------------------------------------------------
# Option for --enable-strict-format-checks
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_STRICT_FORMAT_CHECKS "Whether to perform strict file format checks" OFF)
else ()
set(HDF5_STRICT_FORMAT_CHECKS OFF)
endif ()
if (HDF5_STRICT_FORMAT_CHECKS)
  set (${HDF_PREFIX}_STRICT_FORMAT_CHECKS 1)
endif ()
MARK_AS_ADVANCED (HDF5_STRICT_FORMAT_CHECKS)

# ----------------------------------------------------------------------
# Decide whether the data accuracy has higher priority during data
# conversions.  If not, some hard conversions will still be preferred even
# though the data may be wrong (for example, some compilers don't
# support denormalized floating values) to maximize speed.
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_WANT_DATA_ACCURACY "IF data accuracy is guaranteed during data conversions" ON)
else ()
set(HDF5_WANT_DATA_ACCURACY ON)
endif ()
if (HDF5_WANT_DATA_ACCURACY)
  set (${HDF_PREFIX}_WANT_DATA_ACCURACY 1)
endif ()
MARK_AS_ADVANCED (HDF5_WANT_DATA_ACCURACY)

# ----------------------------------------------------------------------
# Decide whether the presence of user's exception handling functions is
# checked and data conversion exceptions are returned.  This is mainly
# for the speed optimization of hard conversions.  Soft conversions can
# actually benefit little.
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_WANT_DCONV_EXCEPTION "exception handling functions is checked during data conversions" ON)
else ()
set(HDF5_WANT_DCONV_EXCEPTION ON)
endif ()
if (HDF5_WANT_DCONV_EXCEPTION)
  set (${HDF_PREFIX}_WANT_DCONV_EXCEPTION 1)
endif ()
MARK_AS_ADVANCED (HDF5_WANT_DCONV_EXCEPTION)

# ----------------------------------------------------------------------
# Check if they would like the function stack support compiled in
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_CODESTACK "Enable the function stack tracing (for developer debugging)." OFF)
else ()
set(HDF5_ENABLE_CODESTACK OFF)
endif ()
if (HDF5_ENABLE_CODESTACK)
  set (${HDF_PREFIX}_HAVE_CODESTACK 1)
endif ()
MARK_AS_ADVANCED (HDF5_ENABLE_CODESTACK)

# ----------------------------------------------------------------------
# Check if they would like to use file locking by default
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_USE_FILE_LOCKING "Use file locking by default (mainly for SWMR)" ON)
else ()
set(HDF5_USE_FILE_LOCKING ON)
endif ()
if (HDF5_USE_FILE_LOCKING)
  set (${HDF_PREFIX}_USE_FILE_LOCKING 1)
endif ()

# ----------------------------------------------------------------------
# Check if they would like to ignore file locks when disabled on a file system
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_IGNORE_DISABLED_FILE_LOCKS "Ignore file locks when disabled on file system" ON)
else ()
set(HDF5_IGNORE_DISABLED_FILE_LOCKS ON)
endif ()
if (HDF5_IGNORE_DISABLED_FILE_LOCKS)
  set (${HDF_PREFIX}_IGNORE_DISABLED_FILE_LOCKS 1)
endif ()

# Set the libhdf5.settings file variable
if (HDF5_IGNORE_DISABLED_FILE_LOCKS AND HDF5_USE_FILE_LOCKING)
  set (HDF5_FILE_LOCKING_SETTING "best-effort")
elseif (HDF5_IGNORE_DISABLED_FILE_LOCKS)
  set (HDF5_FILE_LOCKING_SETTING "yes")
else ()
  set (HDF5_FILE_LOCKING_SETTING "no")
endif ()

# so far we have no check for this
set (${HDF_PREFIX}_HAVE_TMPFILE 1)

# TODO --------------------------------------------------------------------------
# Should the Default Virtual File Driver be compiled?
# This is hard-coded now but option should added to match configure
#-----------------------------------------------------------------------------
set (${HDF_PREFIX}_DEFAULT_VFD H5FD_SEC2)

if (NOT DEFINED "${HDF_PREFIX}_DEFAULT_PLUGINDIR")
  if (WINDOWS)
    set (${HDF_PREFIX}_DEFAULT_PLUGINDIR "%ALLUSERSPROFILE%\\\\hdf5\\\\lib\\\\plugin")
  else ()
    set (${HDF_PREFIX}_DEFAULT_PLUGINDIR "/usr/local/hdf5/lib/plugin")
  endif ()
endif ()

if (WINDOWS)
  set (${HDF_PREFIX}_HAVE_WINDOWS 1)
  # ----------------------------------------------------------------------
  # Set the flag to indicate that the machine has window style pathname,
  # that is, "drive-letter:\" (e.g. "C:") or "drive-letter:/" (e.g. "C:/").
  # (This flag should be _unset_ for all machines, except for Windows)
  #-----------------------------------------------------------------------
  set (${HDF_PREFIX}_HAVE_WINDOW_PATH 1)
endif ()

# ----------------------------------------------------------------------
# END of WINDOWS Hard code Values
# ----------------------------------------------------------------------

# Find the library containing clock_gettime()
if (MINGW OR NOT WINDOWS)
  CHECK_FUNCTION_EXISTS (clock_gettime CLOCK_GETTIME_IN_LIBC)
  CHECK_LIBRARY_EXISTS (rt clock_gettime "" CLOCK_GETTIME_IN_LIBRT)
  CHECK_LIBRARY_EXISTS (posix4 clock_gettime "" CLOCK_GETTIME_IN_LIBPOSIX4)
  if (CLOCK_GETTIME_IN_LIBC)
    set (${HDF_PREFIX}_HAVE_CLOCK_GETTIME 1)
  elseif (CLOCK_GETTIME_IN_LIBRT)
    set (${HDF_PREFIX}_HAVE_CLOCK_GETTIME 1)
    list (APPEND LINK_LIBS rt)
  elseif (CLOCK_GETTIME_IN_LIBPOSIX4)
    set (${HDF_PREFIX}_HAVE_CLOCK_GETTIME 1)
    list (APPEND LINK_LIBS posix4)
  endif ()
endif ()
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
#  Check if Direct I/O driver works
#-----------------------------------------------------------------------------
if (NOT WINDOWS)
  if (FALSE) # XXX(kitware): Hardcode settings.
  option (HDF5_ENABLE_DIRECT_VFD "Build the Direct I/O Virtual File Driver" OFF)
  else ()
  set(HDF5_ENABLE_DIRECT_VFD OFF)
  add_definitions ("-D_GNU_SOURCE")
  endif ()
  if (HDF5_ENABLE_DIRECT_VFD)
    set (msg "Performing TEST_DIRECT_VFD_WORKS")
    set (MACRO_CHECK_FUNCTION_DEFINITIONS "-DTEST_DIRECT_VFD_WORKS -D_GNU_SOURCE ${CMAKE_REQUIRED_FLAGS}")
    TRY_RUN (TEST_DIRECT_VFD_WORKS_RUN   TEST_DIRECT_VFD_WORKS_COMPILE
        ${CMAKE_BINARY_DIR}
        ${HDF_RESOURCES_EXT_DIR}/HDFTests.c
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
        OUTPUT_VARIABLE OUTPUT
    )
    if (TEST_DIRECT_VFD_WORKS_COMPILE)
      if (TEST_DIRECT_VFD_WORKS_RUN EQUAL "0")
        HDF_FUNCTION_TEST (HAVE_DIRECT)
        set (CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} -D_GNU_SOURCE")
        add_definitions ("-D_GNU_SOURCE")
      else ()
        set (TEST_DIRECT_VFD_WORKS "" CACHE INTERNAL ${msg})
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
          message (VERBOSE "${msg}... no")
        endif ()
        file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
              "Test TEST_DIRECT_VFD_WORKS Run failed with the following output and exit code:\n ${OUTPUT}\n"
        )
      endif ()
    else ()
      set (TEST_DIRECT_VFD_WORKS "" CACHE INTERNAL ${msg})
      if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
        message (VERBOSE "${msg}... no")
      endif ()
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Test TEST_DIRECT_VFD_WORKS Compile failed with the following output:\n ${OUTPUT}\n"
      )
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
#  Check if ROS3 driver can be built
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_ROS3_VFD "Build the ROS3 Virtual File Driver" OFF)
else ()
set(HDF5_ENABLE_ROS3_VFD OFF)
endif ()
  if (HDF5_ENABLE_ROS3_VFD)
    find_package(CURL REQUIRED)
    find_package(OpenSSL REQUIRED)
    if (${CURL_FOUND} AND ${OPENSSL_FOUND})
      set (${HDF_PREFIX}_HAVE_ROS3_VFD 1)
      list (APPEND LINK_LIBS ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES})
      INCLUDE_DIRECTORIES (${CURL_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})
    else ()
      message (WARNING "The Read-Only S3 VFD was requested but cannot be built.\nPlease check that openssl and cURL are available on your\nsystem, and/or re-configure without option HDF5_ENABLE_ROS3_VFD.")
    endif ()
endif ()

# ----------------------------------------------------------------------
# Check whether we can build the Mirror VFD
# Header-check flags set in config/cmake_ext_mod/ConfigureChecks.cmake
# ----------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_MIRROR_VFD "Build the Mirror Virtual File Driver" OFF)
else ()
set(HDF5_ENABLE_MIRROR_VFD OFF)
endif ()
if (HDF5_ENABLE_MIRROR_VFD)
  if ( ${HDF_PREFIX}_HAVE_NETINET_IN_H AND
       ${HDF_PREFIX}_HAVE_NETDB_H      AND
       ${HDF_PREFIX}_HAVE_ARPA_INET_H  AND
       ${HDF_PREFIX}_HAVE_SYS_SOCKET_H AND
       ${HDF_PREFIX}_HAVE_FORK)
      set (${HDF_PREFIX}_HAVE_MIRROR_VFD 1)
  else()
    message(WARNING "The socket-based Mirror VFD was requested but cannot be built. System prerequisites are not met.")
  endif()
endif()

#-----------------------------------------------------------------------------
# Check if C has __float128 extension
#-----------------------------------------------------------------------------

HDF_CHECK_TYPE_SIZE(__float128 _SIZEOF___FLOAT128)
if (_SIZEOF___FLOAT128)
  set (${HDF_PREFIX}_HAVE_FLOAT128 1)
  set (${HDF_PREFIX}_SIZEOF___FLOAT128 ${_SIZEOF___FLOAT128})
else ()
  set (${HDF_PREFIX}_HAVE_FLOAT128 0)
  set (${HDF_PREFIX}_SIZEOF___FLOAT128 0)
endif ()

HDF_CHECK_TYPE_SIZE(_Quad _SIZEOF__QUAD)
if (NOT _SIZEOF__QUAD)
  set (${HDF_PREFIX}_SIZEOF__QUAD 0)
else ()
  set (${HDF_PREFIX}_SIZEOF__QUAD ${_SIZEOF__QUAD})
endif ()

if (NOT CMAKE_CROSSCOMPILING)
#-----------------------------------------------------------------------------
# The provided CMake C macros don't provide a general compile/run function
# so this one is used.
#-----------------------------------------------------------------------------
set (RUN_OUTPUT_PATH_DEFAULT ${CMAKE_BINARY_DIR})
macro (C_RUN FUNCTION_NAME SOURCE_CODE RETURN_VAR RETURN_OUTPUT_VAR)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
      message (VERBOSE "Detecting C ${FUNCTION_NAME}")
    endif ()
    file (WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testCCompiler1.c
        ${SOURCE_CODE}
    )
    TRY_RUN (RUN_RESULT_VAR COMPILE_RESULT_VAR
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testCCompiler1.c
        COMPILE_DEFINITIONS "-D_SIZEOF___FLOAT128=${H5_SIZEOF___FLOAT128};-D_HAVE_QUADMATH_H=${H5_HAVE_QUADMATH_H}"
        COMPILE_OUTPUT_VARIABLE COMPILEOUT
        RUN_OUTPUT_VARIABLE OUTPUT_VAR
    )

    set (${RETURN_OUTPUT_VAR} ${OUTPUT_VAR})

    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
      message (VERBOSE "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
      message (VERBOSE "Test COMPILE_RESULT_VAR ${COMPILE_RESULT_VAR} ")
      message (VERBOSE "Test COMPILE_OUTPUT ${COMPILEOUT} ")
      message (VERBOSE "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
      message (VERBOSE "Test RUN_RESULT_VAR ${RUN_RESULT_VAR} ")
      message (VERBOSE "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
    endif ()

    if (COMPILE_RESULT_VAR)
      if (RUN_RESULT_VAR EQUAL "0")
        set (${RETURN_VAR} 1 CACHE INTERNAL "Have C function ${FUNCTION_NAME}")
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
          message (VERBOSE "Testing C ${FUNCTION_NAME} - OK")
        endif ()
        file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
            "Determining if the C ${FUNCTION_NAME} exists passed with the following output:\n"
            "${OUTPUT_VAR}\n\n"
        )
      else ()
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
          message (VERBOSE "Testing C ${FUNCTION_NAME} - Fail")
        endif ()
        set (${RETURN_VAR} 0 CACHE INTERNAL "Have C function ${FUNCTION_NAME}")
        file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
            "Determining if the C ${FUNCTION_NAME} exists failed with the following output:\n"
            "${OUTPUT_VAR}\n\n")
      endif ()
    else ()
        message (FATAL_ERROR "Compilation of C ${FUNCTION_NAME} - Failed")
    endif ()
endmacro ()

set (PROG_SRC
    "
#include <float.h>\n\
#include <stdio.h>\n\
#define CHECK_FLOAT128 _SIZEOF___FLOAT128\n\
#if CHECK_FLOAT128!=0\n\
#if _HAVE_QUADMATH_H!=0\n\
#include <quadmath.h>\n\
#endif\n\
#ifdef FLT128_DIG\n\
#define C_FLT128_DIG FLT128_DIG\n\
#else\n\
#define C_FLT128_DIG 0\n\
#endif\n\
#else\n\
#define C_FLT128_DIG 0\n\
#endif\n\
#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L\n\
#define C_LDBL_DIG DECIMAL_DIG\n\
#else\n\
#define C_LDBL_DIG LDBL_DIG\n\
#endif\n\nint main() {\nprintf(\"\\%d\\\;\\%d\\\;\", C_LDBL_DIG, C_FLT128_DIG)\\\;\n\nreturn 0\\\;\n}\n
     "
)

C_RUN ("maximum decimal precision for C" ${PROG_SRC} PROG_RES PROG_OUTPUT4)
#message (STATUS "Testing maximum decimal precision for C - ${PROG_OUTPUT4}")

# dnl The output from the above program will be:
# dnl  -- long double decimal precision  --  __float128 decimal precision

list (GET PROG_OUTPUT4 0 H5_LDBL_DIG)
list (GET PROG_OUTPUT4 1 H5_FLT128_DIG)
endif ()

if (${HDF_PREFIX}_SIZEOF___FLOAT128 EQUAL "0" OR FLT128_DIG EQUAL "0")
  set (${HDF_PREFIX}_HAVE_FLOAT128 0)
  set (${HDF_PREFIX}_SIZEOF___FLOAT128 0)
  set (_PAC_C_MAX_REAL_PRECISION ${H5_LDBL_DIG})
else ()
  set (_PAC_C_MAX_REAL_PRECISION ${H5_FLT128_DIG})
endif ()
if (NOT ${_PAC_C_MAX_REAL_PRECISION})
  set (${HDF_PREFIX}_PAC_C_MAX_REAL_PRECISION 0)
else ()
  set (${HDF_PREFIX}_PAC_C_MAX_REAL_PRECISION ${_PAC_C_MAX_REAL_PRECISION})
endif ()
#message (STATUS "maximum decimal precision for C var - ${${HDF_PREFIX}_PAC_C_MAX_REAL_PRECISION}")

#-----------------------------------------------------------------------------
# Macro to determine the various conversion capabilities
#-----------------------------------------------------------------------------
macro (H5ConversionTests TEST msg)
  if (NOT DEFINED ${TEST})
    TRY_RUN (${TEST}_RUN   ${TEST}_COMPILE
        ${CMAKE_BINARY_DIR}
        ${HDF_RESOURCES_DIR}/ConversionTests.c
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=-D${TEST}_TEST
        OUTPUT_VARIABLE OUTPUT
    )
    if (${TEST}_COMPILE)
      if (${TEST}_RUN EQUAL "0")
        set (${TEST} 1 CACHE INTERNAL ${msg})
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
          message (VERBOSE "${msg}... yes")
        endif ()
      else ()
        set (${TEST} "" CACHE INTERNAL ${msg})
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
          message (VERBOSE "${msg}... no")
        endif ()
        file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
              "Test ${TEST} Run failed with the following output and exit code:\n ${OUTPUT}\n"
        )
      endif ()
    else ()
      set (${TEST} "" CACHE INTERNAL ${msg})
      if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
        message (VERBOSE "${msg}... no")
      endif ()
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Test ${TEST} Compile failed with the following output:\n ${OUTPUT}\n"
      )
    endif ()

  endif ()
endmacro ()

#-----------------------------------------------------------------------------
# Check various conversion capabilities
#-----------------------------------------------------------------------------

# ----------------------------------------------------------------------
# Set the flag to indicate that the machine is using a special algorithm to convert
# 'long double' to '(unsigned) long' values.  (This flag should only be set for
# the IBM Power6 Linux.  When the bit sequence of long double is
# 0x4351ccf385ebc8a0bfcc2a3c3d855620, the converted value of (unsigned)long
# is 0x004733ce17af227f, not the same as the library's conversion to 0x004733ce17af2282.
# The machine's conversion gets the correct value.  We define the macro and disable
# this kind of test until we figure out what algorithm they use.
#-----------------------------------------------------------------------------
H5ConversionTests (${HDF_PREFIX}_LDOUBLE_TO_LONG_SPECIAL  "Checking IF your system converts long double to (unsigned) long values with special algorithm")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine is using a special algorithm
# to convert some values of '(unsigned) long' to 'long double' values.
# (This flag should be off for all machines, except for IBM Power6 Linux,
# when the bit sequences are 003fff..., 007fff..., 00ffff..., 01ffff...,
# ..., 7fffff..., the compiler uses a unknown algorithm.  We define a
# macro and skip the test for now until we know about the algorithm.
#-----------------------------------------------------------------------------
H5ConversionTests (${HDF_PREFIX}_LONG_TO_LDOUBLE_SPECIAL "Checking IF your system can convert (unsigned) long to long double values with special algorithm")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# 'long double' to '(unsigned) long long' values.  (This flag should be set for
# all machines, except for Mac OS 10.4 and SGI IRIX64 6.5.  When the bit sequence
# of long double is 0x4351ccf385ebc8a0bfcc2a3c..., the values of (unsigned)long long
# start to go wrong on these two machines.  Adjusting it higher to
# 0x4351ccf385ebc8a0dfcc... or 0x4351ccf385ebc8a0ffcc... will make the converted
# values wildly wrong.  This test detects this wrong behavior and disable the test.
#-----------------------------------------------------------------------------
H5ConversionTests (${HDF_PREFIX}_LDOUBLE_TO_LLONG_ACCURATE "Checking IF correctly converting long double to (unsigned) long long values")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# '(unsigned) long long' to 'long double' values.  (This flag should be set for
# all machines, except for Mac OS 10.4, when the bit sequences are 003fff...,
# 007fff..., 00ffff..., 01ffff..., ..., 7fffff..., the converted values are twice
# as big as they should be.
#-----------------------------------------------------------------------------
H5ConversionTests (${HDF_PREFIX}_LLONG_TO_LDOUBLE_CORRECT "Checking IF correctly converting (unsigned) long long to long double values")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine can accurately convert
# some long double values
#-----------------------------------------------------------------------------
H5ConversionTests (${HDF_PREFIX}_DISABLE_SOME_LDOUBLE_CONV "Checking IF the cpu is power9 and cannot correctly converting long double values")
# ----------------------------------------------------------------------
# Check if pointer alignments are enforced
#-----------------------------------------------------------------------------
H5ConversionTests (${HDF_PREFIX}_NO_ALIGNMENT_RESTRICTIONS "Checking IF alignment restrictions are strictly enforced")
