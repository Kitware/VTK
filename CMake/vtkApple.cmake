# Objective-C++ compile flags.
# CMake has no equivalent of CMAKE_CXX_FLAGS for Objective-C++ (bug #4756)
# so we provide this in case the user needs to specify flags specifically
# for Objective-C++ source files.  For example, to build with garbage
# collection support, the -fobjc-gc flag would be used.
set(VTK_OBJCXX_FLAGS_DEFAULT "")
set(VTK_REQUIRED_OBJCXX_FLAGS ${VTK_OBJCXX_FLAGS_DEFAULT} CACHE STRING "Extra flags for Objective-C++ compilation")
mark_as_advanced(VTK_REQUIRED_OBJCXX_FLAGS)

mark_as_advanced(
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
  CMAKE_OSX_SYSROOT)

if (CMAKE_OSX_DEPLOYMENT_TARGET AND
    CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.7")
  message(FATAL_ERROR "Minimum OS X deployment target is 10.7, please update CMAKE_OSX_DEPLOYMENT_TARGET.")
endif ()
