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
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

if (FALSE) # XXX(kitware): Silence extraneous messages.
message (STATUS "Warnings Configuration:")
endif ()
set (CMAKE_C_FLAGS "${CMAKE_C99_STANDARD_COMPILE_OPTION} ${CMAKE_C_FLAGS}")
#-----------------------------------------------------------------------------
# Compiler specific flags : Shouldn't there be compiler tests for these
#-----------------------------------------------------------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
if (CMAKE_COMPILER_IS_GNUCC)
  set (CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  if (${HDF_CFG_NAME} MATCHES "Debug")
    if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0)
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -ftrapv -fno-common")
    endif ()
  else ()
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0)
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstdarg-opt")
    endif ()
  endif ()
endif ()
if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
  set (CMAKE_CXX_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_CXX_FLAGS}")
  if (${HDF_CFG_NAME} MATCHES "Debug")
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -ftrapv -fno-common")
    endif ()
  else ()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstdarg-opt")
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to disable compiler warnings
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_DISABLE_COMPILER_WARNINGS "Disable compiler warnings" OFF)
else ()
set(HDF5_DISABLE_COMPILER_WARNINGS OFF)
endif ()
if (HDF5_DISABLE_COMPILER_WARNINGS)
  message (STATUS "....Compiler warnings are suppressed")
  # MSVC uses /w to suppress warnings.  It also complains if another
  # warning level is given, so remove it.
  if (MSVC)
    set (HDF5_WARNINGS_BLOCKED 1)
    string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
    endif ()
  endif ()
  if (WIN32)
    add_definitions (-D_CRT_SECURE_NO_WARNINGS)
  endif ()
  # Borland uses -w- to suppress warnings.
  if (BORLAND)
    set (HDF5_WARNINGS_BLOCKED 1)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-")
  endif ()

  # Most compilers use -w to suppress warnings.
  if (NOT HDF5_WARNINGS_BLOCKED)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# CDash is configured to only allow 3000 warnings, so
# break into groups (from the config/gnu-flags file)
#-----------------------------------------------------------------------------
if (NOT MSVC AND CMAKE_COMPILER_IS_GNUCC)
  if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -erroff=%none -DBSD_COMP")
  else ()
    # General flags
    #
    # Note that some of the flags listed here really should be developer
    # flags (listed in a separate variable, below) but we put them here
    # because they are not raised by the current code and we'd like to
    # know if they do start showing up.
    #
    # NOTE: Don't add -Wpadded here since we can't/won't fix the (many)
    # warnings that are emitted. If you need it, add it at configure time.
    if (CMAKE_C_COMPILER_ID STREQUAL "Intel")
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcheck -Wall")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wcomment -Wdeprecated -Wmain -Wmissing-declarations -Wmissing-prototypes -Wp64 -Wpointer-arith")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wreturn-type -Wstrict-prototypes -Wuninitialized")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wunknown-pragmas -Wunused-function -Wunused-variable")
      # this is just a failsafe
      set (H5_CFLAGS0 "${H5_CFLAGS0} -finline-functions")
      if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 18.0)
        set (H5_CFLAGS0 "${H5_CFLAGS0} -Wextra-tokens -Wformat -Wformat-security -Wic-pointer -Wshadow")
        set (H5_CFLAGS0 "${H5_CFLAGS0} -Wsign-compare -Wtrigraphs -Wwrite-strings")
      endif()
    elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wbad-function-cast -Wc++-compat -Wcast-align")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wcast-qual -Wconversion -Wdeclaration-after-statement -Wdisabled-optimization -Wfloat-equal")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wformat=2 -Winit-self -Winvalid-pch -Wmissing-declarations -Wmissing-include-dirs")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wmissing-prototypes -Wnested-externs -Wold-style-definition -Wpacked -Wpointer-arith")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wredundant-decls -Wshadow -Wstrict-prototypes -Wswitch-default -Wswitch-enum")
      set (H5_CFLAGS0 "${H5_CFLAGS0} -Wundef -Wunused-macros -Wunsafe-loop-optimizations -Wwrite-strings")
      # gcc automatically inlines based on the optimization level
      # this is just a failsafe
      set (H5_CFLAGS0 "${H5_CFLAGS0} -finline-functions")
    endif ()
  endif ()

    #-----------------------------------------------------------------------------
    # Option to allow the user to enable developer warnings
    # Developer warnings (suggestions from gcc, not code problems)
    #-----------------------------------------------------------------------------
    if (FALSE) # XXX(kitware): Hardcode settings.
    option (HDF5_ENABLE_DEV_WARNINGS "Enable HDF5 developer group warnings" OFF)
    else ()
    set(HDF5_ENABLE_DEV_WARNINGS OFF)
    endif ()
    if (HDF5_ENABLE_DEV_WARNINGS)
      message (STATUS "....HDF5 developer group warnings are enabled")
      if (CMAKE_C_COMPILER_ID STREQUAL "Intel")
        set (H5_CFLAGS0 "${H5_CFLAGS0} -Winline -Wreorder -Wport -Wstrict-aliasing")
      elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set (H5_CFLAGS0 "${H5_CFLAGS0} -Winline -Waggregate-return -Wmissing-format-attribute -Wmissing-noreturn")
      endif ()
    else ()
      if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set (H5_CFLAGS0 "${H5_CFLAGS0} -Wno-inline -Wno-aggregate-return -Wno-missing-format-attribute -Wno-missing-noreturn")
      endif ()
    endif ()


    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      # Append warning flags that only gcc 4.3+ knows about
      #
      # Technically, variable-length arrays are part of the C99 standard, but
      #   we should approach them a bit cautiously... -QAK
      set (H5_CFLAGS1 "${H5_CFLAGS1} -Wlogical-op -Wlarger-than=2560 -Wvla")

      # Append more extra warning flags that only gcc 4.4+ know about
      set (H5_CFLAGS1 "${H5_CFLAGS1} -Wsync-nand -Wframe-larger-than=16384 -Wpacked-bitfield-compat")
    endif ()

    # Append more extra warning flags that only gcc 4.5+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.5)
      set (H5_CFLAGS1 "${H5_CFLAGS1} -Wstrict-overflow=5 -Wjump-misses-init -Wunsuffixed-float-constants")
    endif ()

    # Append more extra warning flags that only gcc 4.6+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.6)
      set (H5_CFLAGS2 "${H5_CFLAGS2} -Wdouble-promotion -Wtrampolines")
      if (HDF5_ENABLE_DEV_WARNINGS)
        set (H5_CFLAGS2 "${H5_CFLAGS2} -Wsuggest-attribute=const")
      else ()
        set (H5_CFLAGS2 "${H5_CFLAGS2} -Wno-suggest-attribute=const")
      endif ()
    endif ()

    # Append more extra warning flags that only gcc 4.7+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.7)
      set (H5_CFLAGS2 "${H5_CFLAGS2} -Wstack-usage=8192 -Wvector-operation-performance")
      if (HDF5_ENABLE_DEV_WARNINGS)
        set (H5_CFLAGS2 "${H5_CFLAGS2} -Wsuggest-attribute=pure -Wsuggest-attribute=noreturn")
      else ()
        set (H5_CFLAGS2 "${H5_CFLAGS2} -Wno-suggest-attribute=pure -Wno-suggest-attribute=noreturn")
      endif ()
    endif ()

    # Append more extra warning flags that only gcc 4.8+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.8)
      if (HDF5_ENABLE_DEV_WARNINGS)
        set (H5_CFLAGS2 "${H5_CFLAGS2} -Wsuggest-attribute=format")
      else ()
        set (H5_CFLAGS2 "${H5_CFLAGS2} -Wno-suggest-attribute=format")
      endif ()
    endif ()

    # Append more extra warning flags that only gcc 4.9+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.9)
      set (H5_CFLAGS2 "${H5_CFLAGS2} -Wdate-time")
    endif ()

    # Append more extra warning flags that only gcc 5.1+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.1)
      set (H5_CFLAGS3 "${H5_CFLAGS3} -Warray-bounds=2 -Wc99-c11-compat")
    endif ()

    # Append more extra warning flags that only gcc 6.x+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 6.0)
      set (H5_CFLAGS4 "${H5_CFLAGS4} -Wnull-dereference -Wunused-const-variable -Wduplicated-cond -Whsa -Wnormalized")
    endif ()

    # Append more extra warning flags that only gcc 7.x+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 7.0)
      set (H5_CFLAGS4 "${H5_CFLAGS4} -Walloc-zero -Walloca -Wduplicated-branches -Wformat-overflow=2 -Wformat-truncation=2 -Wimplicit-fallthrough=5 -Wrestrict")
    endif ()

    # Append more extra warning flags that only gcc 8.x+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 8.0)
      set (H5_CFLAGS4 "${H5_CFLAGS4} -Wattribute-alias -Wcast-align=strict -Wshift-overflow=2 -Wno-suggest-attribute=cold -Wno-suggest-attribute=malloc")
    endif ()

    # Append more extra warning flags that only gcc 9.x+ know about
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 9.0)
      set (H5_CFLAGS4 "${H5_CFLAGS4} -Wattribute-alias=2 -Wmissing-profile")
    endif ()
elseif (CMAKE_C_COMPILER_ID STREQUAL "PGI")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Minform=inform")
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable all warnings
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_ALL_WARNINGS "Enable all warnings" OFF)
else ()
set(HDF5_ENABLE_ALL_WARNINGS OFF)
endif ()
if (HDF5_ENABLE_ALL_WARNINGS)
  message (STATUS "....All Warnings are enabled")
  if (MSVC)
    if (HDF5_ENABLE_DEV_WARNINGS)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Wall /wd4668")
      if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
        string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall /wd4668")
      endif ()
    else ()
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
      if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
        string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
      endif ()
    endif ()
  else ()
    if (CMAKE_COMPILER_IS_GNUCC)
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${H5_CFLAGS0} ${H5_CFLAGS1} ${H5_CFLAGS2}")
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable warnings by groups
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_GROUPZERO_WARNINGS "Enable group zero warnings" OFF)
else ()
set(HDF5_ENABLE_GROUPZERO_WARNINGS OFF)
endif ()
if (HDF5_ENABLE_GROUPZERO_WARNINGS)
  message (STATUS "....Group Zero warnings are enabled")
  if (MSVC)
    string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W1")
    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W1")
    endif ()
  else ()
    if (CMAKE_COMPILER_IS_GNUCC)
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${H5_CFLAGS0}")
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable warnings by groups
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_GROUPONE_WARNINGS "Enable group one warnings" OFF)
else ()
set(HDF5_ENABLE_GROUPONE_WARNINGS OFF)
endif ()
if (HDF5_ENABLE_GROUPONE_WARNINGS)
  message (STATUS "....Group One warnings are enabled")
  if (MSVC)
    string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W2")
    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2")
    endif ()
  else ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${H5_CFLAGS1}")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable warnings by groups
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_GROUPTWO_WARNINGS "Enable group two warnings" OFF)
else ()
set(HDF5_ENABLE_GROUPTWO_WARNINGS OFF)
endif ()
if (HDF5_ENABLE_GROUPTWO_WARNINGS)
  message (STATUS "....Group Two warnings are enabled")
  if (MSVC)
    string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    endif ()
  else ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${H5_CFLAGS2}")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable warnings by groups
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_GROUPTHREE_WARNINGS "Enable group three warnings" OFF)
else ()
set(HDF5_ENABLE_GROUPTHREE_WARNINGS OFF)
endif ()
if (HDF5_ENABLE_GROUPTHREE_WARNINGS)
  message (STATUS "....Group Three warnings are enabled")
  if (MSVC)
    string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif ()
  else ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${H5_CFLAGS3}")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable warnings by groups
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_GROUPFOUR_WARNINGS "Enable group four warnings" OFF)
else ()
set(HDF5_ENABLE_GROUPFOUR_WARNINGS OFF)
endif ()
if (HDF5_ENABLE_GROUPFOUR_WARNINGS)
  message (STATUS "....Group Four warnings are enabled")
  if (NOT MSVC)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${H5_CFLAGS4}")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# This is in here to help some of the GCC based IDES like Eclipse
# and code blocks parse the compiler errors and warnings better.
#-----------------------------------------------------------------------------
if (CMAKE_COMPILER_IS_GNUCC)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
endif ()
if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_LOADED)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
endif ()

#-----------------------------------------------------------------------------
# Option for --enable-asserts
# By default, CMake adds NDEBUG to CMAKE_${lang}_FLAGS for Release build types
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
#option (HDF5_ENABLE_ASSERTS "Determines whether NDEBUG is defined to control assertions." OFF)
set (HDF5_ENABLE_ASSERTS "OFF" CACHE STRING "Determines whether NDEBUG is defined to control assertions (OFF NO YES)")
set_property (CACHE HDF5_ENABLE_ASSERTS PROPERTY STRINGS OFF NO YES)
if (HDF5_ENABLE_ASSERTS MATCHES "YES")
  add_compile_options ("-UNDEBUG")
elseif (HDF5_ENABLE_ASSERTS MATCHES "NO")
  add_compile_options ("-DNDEBUG")
endif ()
MARK_AS_ADVANCED (HDF5_ENABLE_ASSERTS)

#-----------------------------------------------------------------------------
# Option for --enable-symbols
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
#option (HDF5_ENABLE_SYMBOLS "Add debug symbols to the library independent of the build mode and optimization level." OFF)
set (HDF5_ENABLE_SYMBOLS "OFF" CACHE STRING "Add debug symbols to the library independent of the build mode and optimization level (OFF NO YES)")
set_property (CACHE HDF5_ENABLE_SYMBOLS PROPERTY STRINGS OFF NO YES)
if (HDF5_ENABLE_SYMBOLS MATCHES "YES")
  if (CMAKE_C_COMPILER_ID STREQUAL "Intel")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
  elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fno-omit-frame-pointer")
  endif ()
  if(CMAKE_CXX_COMPILER_LOADED)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    endif ()
  endif ()
elseif (HDF5_ENABLE_SYMBOLS MATCHES "NO")
  if (CMAKE_C_COMPILER_ID STREQUAL "Intel")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-s")
  elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
  endif ()
  if(CMAKE_CXX_COMPILER_LOADED)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
      set (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wl,-s")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
    endif ()
  endif ()
endif ()
MARK_AS_ADVANCED (HDF5_ENABLE_SYMBOLS)

#-----------------------------------------------------------------------------
# Option for --enable-profiling
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_PROFILING "Enable profiling flags independently from the build mode." OFF)
if (HDF5_ENABLE_PROFILING)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROFILE_CFLAGS}")
  if(CMAKE_CXX_COMPILER_LOADED)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROFILE_CXXFLAGS}")
  endif ()
endif ()
MARK_AS_ADVANCED (HDF5_ENABLE_PROFILING)

#-----------------------------------------------------------------------------
# Option for --enable-optimization
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
option (HDF5_ENABLE_OPTIMIZATION "Enable optimization flags/settings independently from the build mode" OFF)
if (HDF5_ENABLE_OPTIMIZATION)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPTIMIZE_CFLAGS}")
  if(CMAKE_CXX_COMPILER_LOADED)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPTIMIZE_CXXFLAGS}")
  endif ()
endif ()
MARK_AS_ADVANCED (HDF5_ENABLE_OPTIMIZATION)
