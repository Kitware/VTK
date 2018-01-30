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
include (${HDF_RESOURCES_EXT_DIR}/ConfigureChecks.cmake)

if (HDF5_ENABLE_USING_MEMCHECKER)
  set (H5_USING_MEMCHECKER 1)
endif ()

#-----------------------------------------------------------------------------
# Option for --enable-strict-format-checks
#-----------------------------------------------------------------------------
# XXX(kitware): Hardcode settings.
if (FALSE)
option (HDF5_STRICT_FORMAT_CHECKS "Whether to perform strict file format checks" OFF)
else ()
set(HDF5_STRICT_FORMAT_CHECKS OFF)
endif ()
if (HDF5_STRICT_FORMAT_CHECKS)
  set (H5_STRICT_FORMAT_CHECKS 1)
endif ()
MARK_AS_ADVANCED (HDF5_STRICT_FORMAT_CHECKS)

#-----------------------------------------------------------------------------
# Option for --enable-metadata-trace-file
#-----------------------------------------------------------------------------
# XXX(kitware): Hardcode settings.
if (FALSE)
option (HDF5_METADATA_TRACE_FILE "Enable metadata trace file collection" OFF)
else ()
set(HDF5_METADATA_TRACE_FILE OFF)
endif ()
if (HDF5_METADATA_TRACE_FILE)
  set (H5_METADATA_TRACE_FILE 1)
endif ()
MARK_AS_ADVANCED (HDF5_METADATA_TRACE_FILE)

# ----------------------------------------------------------------------
# Decide whether the data accuracy has higher priority during data
# conversions.  If not, some hard conversions will still be prefered even
# though the data may be wrong (for example, some compilers don't
# support denormalized floating values) to maximize speed.
#
# XXX(kitware): Hardcode settings.
if (FALSE)
option (HDF5_WANT_DATA_ACCURACY "IF data accuracy is guaranteed during data conversions" ON)
else ()
set(HDF5_WANT_DATA_ACCURACY ON)
endif ()
if (HDF5_WANT_DATA_ACCURACY)
  set (H5_WANT_DATA_ACCURACY 1)
endif ()
MARK_AS_ADVANCED (HDF5_WANT_DATA_ACCURACY)

# ----------------------------------------------------------------------
# Decide whether the presence of user's exception handling functions is
# checked and data conversion exceptions are returned.  This is mainly
# for the speed optimization of hard conversions.  Soft conversions can
# actually benefit little.
#
# XXX(kitware): Hardcode settings.
if (FALSE)
option (HDF5_WANT_DCONV_EXCEPTION "exception handling functions is checked during data conversions" ON)
else ()
set(HDF5_WANT_DCONV_EXCEPTION ON)
endif ()
if (HDF5_WANT_DCONV_EXCEPTION)
  set (H5_WANT_DCONV_EXCEPTION 1)
endif ()
MARK_AS_ADVANCED (HDF5_WANT_DCONV_EXCEPTION)

# ----------------------------------------------------------------------
# Check if they would like the function stack support compiled in
#
# XXX(kitware): Hardcode settings.
if (FALSE)
option (HDF5_ENABLE_CODESTACK "Enable the function stack tracing (for developer debugging)." OFF)
else ()
set(HDF5_ENABLE_CODESTACK OFF)
endif ()
if (HDF5_ENABLE_CODESTACK)
  set (H5_HAVE_CODESTACK 1)
endif ()
MARK_AS_ADVANCED (HDF5_ENABLE_CODESTACK)

#-----------------------------------------------------------------------------
#  Are we going to use HSIZE_T
#-----------------------------------------------------------------------------
# XXX(kitware): Hardcode settings.
if (FALSE)
option (HDF5_ENABLE_HSIZET "Enable datasets larger than memory" ON)
else ()
set(HDF5_ENABLE_HSIZET OFF)
endif ()
if (HDF5_ENABLE_HSIZET)
  set (H5_HAVE_LARGE_HSIZET 1)
endif ()

# so far we have no check for this
set (H5_HAVE_TMPFILE 1)

# TODO --------------------------------------------------------------------------
# Should the Default Virtual File Driver be compiled?
# This is hard-coded now but option should added to match configure
#
set (H5_DEFAULT_VFD H5FD_SEC2)

if (NOT DEFINED "H5_DEFAULT_PLUGINDIR")
  if (WINDOWS)
    set (H5_DEFAULT_PLUGINDIR "%ALLUSERSPROFILE%\\\\hdf5\\\\lib\\\\plugin")
  else ()
    set (H5_DEFAULT_PLUGINDIR "/usr/local/hdf5/lib/plugin")
  endif ()
endif ()

if (WINDOWS)
  set (H5_HAVE_WINDOWS 1)
  # ----------------------------------------------------------------------
  # Set the flag to indicate that the machine has window style pathname,
  # that is, "drive-letter:\" (e.g. "C:") or "drive-letter:/" (e.g. "C:/").
  # (This flag should be _unset_ for all machines, except for Windows)
  set (H5_HAVE_WINDOW_PATH 1)
endif ()

# ----------------------------------------------------------------------
# END of WINDOWS Hard code Values
# ----------------------------------------------------------------------

CHECK_FUNCTION_EXISTS (difftime          H5_HAVE_DIFFTIME)

#-----------------------------------------------------------------------------
#  Check if Direct I/O driver works
#-----------------------------------------------------------------------------
if (NOT WINDOWS)
  # XXX(kitware): Hardcode settings.
  if (FALSE)
  option (HDF5_ENABLE_DIRECT_VFD "Build the Direct I/O Virtual File Driver" OFF)
  else ()
  set(HDF5_ENABLE_DIRECT_VFD OFF)
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
      if (TEST_DIRECT_VFD_WORKS_RUN MATCHES 0)
        HDF_FUNCTION_TEST (HAVE_DIRECT)
        set (CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} -D_GNU_SOURCE")
        add_definitions ("-D_GNU_SOURCE")
      else ()
        set (TEST_DIRECT_VFD_WORKS "" CACHE INTERNAL ${msg})
        message (STATUS "${msg}... no")
        file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
              "Test TEST_DIRECT_VFD_WORKS Run failed with the following output and exit code:\n ${OUTPUT}\n"
        )
      endif ()
    else ()
      set (TEST_DIRECT_VFD_WORKS "" CACHE INTERNAL ${msg})
      message (STATUS "${msg}... no")
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Test TEST_DIRECT_VFD_WORKS Compile failed with the following output:\n ${OUTPUT}\n"
      )
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Check if C has __float128 extension
#-----------------------------------------------------------------------------

CHECK_TYPE_SIZE("__float128" SIZEOF___FLOAT128)
if (${HAVE_SIZEOF___FLOAT128})
  set (H5_HAVE_FLOAT128 1)
else ()
  set (H5_HAVE_FLOAT128 0)
  set (SIZEOF___FLOAT128 0)
endif ()

#-----------------------------------------------------------------------------
# Macro to determine the various conversion capabilities
#-----------------------------------------------------------------------------
macro (H5ConversionTests TEST msg)
  if (NOT DEFINED ${TEST})
   # message (STATUS "===> ${TEST}")
    TRY_RUN (${TEST}_RUN   ${TEST}_COMPILE
        ${CMAKE_BINARY_DIR}
        ${HDF_RESOURCES_DIR}/ConversionTests.c
        CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=-D${TEST}_TEST
        OUTPUT_VARIABLE OUTPUT
    )
    if (${TEST}_COMPILE)
      if (${TEST}_RUN MATCHES 0)
        set (${TEST} 1 CACHE INTERNAL ${msg})
        message (STATUS "${msg}... yes")
      else ()
        set (${TEST} "" CACHE INTERNAL ${msg})
        message (STATUS "${msg}... no")
        file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
              "Test ${TEST} Run failed with the following output and exit code:\n ${OUTPUT}\n"
        )
      endif ()
    else ()
      set (${TEST} "" CACHE INTERNAL ${msg})
      message (STATUS "${msg}... no")
      file (APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Test ${TEST} Compile failed with the following output:\n ${OUTPUT}\n"
      )
    endif ()

  endif ()
endmacro ()

#-----------------------------------------------------------------------------
# Macro to make some of the conversion tests easier to write/read
#-----------------------------------------------------------------------------
macro (H5MiscConversionTest VAR TEST msg)
  if ("${TEST}" MATCHES "^${TEST}$")
    if (${VAR})
      set (${TEST} 1 CACHE INTERNAL ${msg})
      message (STATUS "${msg}... yes")
    else ()
      set (${TEST} "" CACHE INTERNAL ${msg})
      message (STATUS "${msg}... no")
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
#
H5ConversionTests (H5_LDOUBLE_TO_LONG_SPECIAL  "Checking IF your system converts long double to (unsigned) long values with special algorithm")
# ----------------------------------------------------------------------
# Set the flag to indicate that the machine is using a special algorithm
# to convert some values of '(unsigned) long' to 'long double' values.
# (This flag should be off for all machines, except for IBM Power6 Linux,
# when the bit sequences are 003fff..., 007fff..., 00ffff..., 01ffff...,
# ..., 7fffff..., the compiler uses a unknown algorithm.  We define a
# macro and skip the test for now until we know about the algorithm.
#
H5ConversionTests (H5_LONG_TO_LDOUBLE_SPECIAL "Checking IF your system can convert (unsigned) long to long double values with special algorithm")
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
# Check if pointer alignments are enforced
#
H5ConversionTests (H5_NO_ALIGNMENT_RESTRICTIONS "Checking IF alignment restrictions are strictly enforced")

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
endforeach ()
