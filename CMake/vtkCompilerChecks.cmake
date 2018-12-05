# Minimum compiler version check: GCC >= 4.8
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
  message(FATAL_ERROR "GCC 4.8 or later is required.")
endif ()

# Minimum compiler version check: LLVM Clang >= 3.3
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
  message(FATAL_ERROR "LLVM Clang 3.3 or later is required.")
endif ()

# Minimum compiler version check: Apple Clang >= 5.0 (Xcode 5.0)
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
  message(FATAL_ERROR "Apple Clang 5.0 or later is required.")
endif ()

# Minimum compiler version check: Microsoft C/C++ >= 19.0 (aka VS 2015)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
  message(FATAL_ERROR "Microsoft Visual Studio 2015 or later is required.")
endif ()

# Minimum compiler version check: Intel C++ (ICC) >= 14
if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.0)
  message(FATAL_ERROR "Intel C++ (ICC) 14.0 or later is required.")
endif ()

# Make sure we have C++11 enabled.
if(NOT VTK_IGNORE_CMAKE_CXX11_CHECKS)
  # Needed to make sure libraries and executables not built by the
  # vtkModuleMacros still have the C++11 compiler flags enabled
  # Wrap this in an escape hatch for unknown compilers
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  set(CMAKE_CXX_EXTENSIONS False)
endif()
