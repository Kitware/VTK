# Stock CI builds test everything possible (platforms will disable modules as
# needed).
set(VTK_BUILD_ALL_MODULES ON CACHE BOOL "")

set(VTK_BUILD_LEGACY_REMOVE ON CACHE BOOL "")
set(VTK_BUILD_TESTING WANT CACHE STRING "")
set(VTK_BUILD_EXAMPLES ON CACHE BOOL "")

set(VTK_BUILD_SCALE_SOA_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_SOA_ARRAYS ON CACHE BOOL "")

set(VTK_DEBUG_LEAKS ON CACHE BOOL "")
set(VTK_USE_LARGE_DATA ON CACHE BOOL "")
set(VTK_LINKER_FATAL_WARNINGS ON CACHE BOOL "")

# The install trees on CI machines need help since dependencies are not in a
# default location.
set(VTK_RELOCATABLE_INSTALL ON CACHE BOOL "")

# Remote modules are not under VTK's development process.
set(VTK_ENABLE_REMOTE_MODULES OFF CACHE BOOL "")

# We run the install right after the build. Avoid rerunning it when installing.
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY "ON" CACHE BOOL "")

# Install VTK.
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "")
set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_options.cmake")

# Default to Release builds.
if ("$ENV{CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
else ()
  set(CMAKE_BUILD_TYPE "$ENV{CMAKE_BUILD_TYPE}" CACHE STRING "")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/configure_sccache.cmake")
