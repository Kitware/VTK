set(VTK_REQUIRED_C_FLAGS)
set(VTK_REQUIRED_CXX_FLAGS)

# make sure Crun is linked in with the native compiler, it is
# not used by default for shared libraries and is required for
# things like java to work.
if(CMAKE_SYSTEM MATCHES "SunOS.*")
  if(NOT CMAKE_COMPILER_IS_GNUCXX)
    find_library(VTK_SUNCC_CRUN_LIBRARY Crun /opt/SUNWspro/lib)
    if(VTK_SUNCC_CRUN_LIBRARY)
      link_libraries(${VTK_SUNCC_CRUN_LIBRARY})
    endif()
    find_library(VTK_SUNCC_CSTD_LIBRARY Cstd /opt/SUNWspro/lib)
    if(VTK_SUNCC_CSTD_LIBRARY)
      link_libraries(${VTK_SUNCC_CSTD_LIBRARY})
    endif()
  endif()
endif()

# A GCC compiler.
if(CMAKE_COMPILER_IS_GNUCXX)
  if(VTK_USE_X)
    unset(WIN32)
  endif()
  if(WIN32)
# The platform is gcc on cygwin.
    set(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -mwin32")
    set(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -mwin32")
    link_libraries(-lgdi32)
  endif()
  if(MINGW)
    set(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -mthreads")
    set(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -mthreads")
    set(VTK_REQUIRED_EXE_LINKER_FLAGS "${VTK_REQUIRED_EXE_LINKER_FLAGS} -mthreads")
    set(VTK_REQUIRED_SHARED_LINKER_FLAGS "${VTK_REQUIRED_SHARED_LINKER_FLAGS} -mthreads")
    set(VTK_REQUIRED_MODULE_LINKER_FLAGS "${VTK_REQUIRED_MODULE_LINKER_FLAGS} -mthreads")
  endif()
  if(CMAKE_SYSTEM MATCHES "SunOS.*")
# Disable warnings that occur in X11 headers.
    if(DART_ROOT AND BUILD_TESTING)
      set(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -Wno-unknown-pragmas")
      set(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -Wno-unknown-pragmas")
    endif()
  endif()
else()
  if(CMAKE_ANSI_CFLAGS)
    set(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")
  endif()
  if(CMAKE_SYSTEM MATCHES "OSF1-V.*")
     set(VTK_REQUIRED_CXX_FLAGS
         "${VTK_REQUIRED_CXX_FLAGS} -timplicit_local -no_implicit_include")
  endif()
  if(CMAKE_SYSTEM MATCHES "AIX.*")
    # allow t-ypeid and d-ynamic_cast usage (normally off by default on xlC)
    set(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -qrtti=all")
    # silence duplicate symbol warnings on AIX
    set(VTK_REQUIRED_EXE_LINKER_FLAGS "${VTK_REQUIRED_EXE_LINKER_FLAGS} -bhalt:5")
    set(VTK_REQUIRED_SHARED_LINKER_FLAGS "${VTK_REQUIRED_SHARED_LINKER_FLAGS} -bhalt:5")
    set(VTK_REQUIRED_MODULE_LINKER_FLAGS "${VTK_REQUIRED_MODULE_LINKER_FLAGS} -bhalt:5")
  endif()
  if(CMAKE_SYSTEM MATCHES "HP-UX.*")
     set(VTK_REQUIRED_C_FLAGS
         "${VTK_REQUIRED_C_FLAGS} +W2111 +W2236 +W4276")
     set(VTK_REQUIRED_CXX_FLAGS
         "${VTK_REQUIRED_CXX_FLAGS} +W2111 +W2236 +W4276")
  endif()
endif()

# figure out whether the compiler might be the Intel compiler
set(_MAY_BE_INTEL_COMPILER FALSE)
if(UNIX)
  if(CMAKE_CXX_COMPILER_ID)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
      set(_MAY_BE_INTEL_COMPILER TRUE)
    endif()
  else()
    if(NOT CMAKE_COMPILER_IS_GNUCXX)
      set(_MAY_BE_INTEL_COMPILER TRUE)
    endif()
  endif()
endif()

#if so, test whether -i_dynamic is needed
if(_MAY_BE_INTEL_COMPILER)
  include(${CMAKE_CURRENT_LIST_DIR}/TestNO_ICC_IDYNAMIC_NEEDED.cmake)
  testno_icc_idynamic_needed(NO_ICC_IDYNAMIC_NEEDED ${CMAKE_CURRENT_LIST_DIR})
  if(NO_ICC_IDYNAMIC_NEEDED)
    set(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS}")
  else()
    set(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -i_dynamic")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  # --diag_suppress=236 is for constant value asserts used for error handling
  # This can be restricted to the implementation and doesn't need to propagate
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --diag_suppress=236")

  # --diag_suppress=381 is for redundant semi-colons used in macros
  # This needs to propagate to anything that includes VTK headers
  set(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} --diag_suppress=381")
endif()

if(MSVC)
# Use the highest warning level for visual c++ compiler.
  set(CMAKE_CXX_WARNING_LEVEL 4)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
endif()

# Disable deprecation warnings for standard C and STL functions in VS2015+
# and later
if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_SCL_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

# Enable /MP flag for Visual Studio
if(MSVC)
  set(CMAKE_CXX_MP_FLAG OFF CACHE BOOL "Build with /MP flag enabled")
  set(PROCESSOR_COUNT "$ENV{NUMBER_OF_PROCESSORS}")
  set(CMAKE_CXX_MP_NUM_PROCESSORS ${PROCESSOR_COUNT} CACHE STRING "The maximum number of processes for the /MP flag")
  if (CMAKE_CXX_MP_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
  endif ()
endif()

# Enable /bigobj for MSVC to allow larger symbol tables
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
endif()

#-----------------------------------------------------------------------------
# Add compiler flags VTK needs to work on this platform.  This must be
# done after the call to CMAKE_EXPORT_BUILD_SETTINGS, but before any
# try-compiles are done.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VTK_REQUIRED_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTK_REQUIRED_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${VTK_REQUIRED_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${VTK_REQUIRED_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${VTK_REQUIRED_MODULE_LINKER_FLAGS}")
