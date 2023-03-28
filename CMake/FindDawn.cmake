#[==[
Provides the following variables:

  * `DAWN_INCLUDE_DIRS`: Include directories necessary to use Dawn.
  * `DAWN_LIBRARIES`: Libraries necessary to use Dawn.

The following components are discovered:

  * `dawn_native`
  * `dawn_platform`
  * `dawn_proc`
#]==]

find_library(DAWN_NATIVE_LIBRARY
  NAMES
    libdawn_native.so
    libdawn_native.dll
  PATHS ${DAWN_BINARY_DIR}
  DOC "Path to libdawn_native"
  NO_CACHE REQUIRED
)

find_library(DAWN_PLATFORM_LIBRARY
  NAMES
    libdawn_platform.so
    libdawn_platform.dll
  PATHS ${DAWN_BINARY_DIR}
  DOC "Path to libdawn_platform"
  NO_CACHE REQUIRED
)

find_library(DAWN_PROC_LIBRARY
  NAMES
    libdawn_proc.so
    libdawn_proc.dll
  PATHS ${DAWN_BINARY_DIR}
  DOC "Path to libdawn_proc"
  NO_CACHE REQUIRED
)

find_path(DAWN_GEN_INCLUDE_DIR
  NAMES dawn/webgpu_cpp.h
  PATHS ${DAWN_BINARY_DIR}/gen/include
  DOC "Path to generated dawn/webgpu_cpp.h"
  NO_CACHE REQUIRED
)

find_path(DAWN_GEN_SRC_DIR
  NAMES dawn/webgpu_cpp.cpp
  PATHS ${DAWN_BINARY_DIR}/gen/src
  DOC "Path to generated dawn/webgpu_cpp.cpp"
  NO_CACHE REQUIRED
)

find_path(DAWN_INCLUDE_DIR
  NAMES dawn/dawn_proc.h
  PATHS ${DAWN_SOURCE_DIR}/include
  DOC "Path to generated dawn/dawn_proc.h"
  NO_CACHE REQUIRED
)

set(DAWN_LIBRARIES
  ${DAWN_PLATFORM_LIBRARY}
  ${DAWN_NATIVE_LIBRARY}
  ${DAWN_PROC_LIBRARY}
)

set(DAWN_INCLUDE_DIRS
  ${DAWN_INCLUDE_DIR}
  ${DAWN_GEN_INCLUDE_DIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Dawn
  REQUIRED_VARS DAWN_INCLUDE_DIRS DAWN_LIBRARIES)
