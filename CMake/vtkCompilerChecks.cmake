# Minimum compiler version check: GCC >= 8.0
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
  message(FATAL_ERROR "GCC 8.0 or later is required.")
endif ()

# Minimum compiler version check: LLVM Clang >= 7.0
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
  message(FATAL_ERROR "LLVM Clang 7.0 or later is required.")
endif ()

# Minimum compiler version check: Apple Clang >= 11.0 (Xcode 11.3.1)
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
  message(FATAL_ERROR "Apple Clang 11.0 or later is required.")
endif ()

# Minimum compiler version check: Microsoft C/C++ >= 19.10 (aka VS 2017)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.10)
  message(FATAL_ERROR "Microsoft Visual Studio 2017 or later is required.")
endif ()

# Minimum compiler version check: Intel C++ (ICC) >= 19
if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
  message(FATAL_ERROR "Intel C++ (ICC) 19.0 or later is required.")
endif ()

# Make sure we have C++17 enabled.
if(NOT VTK_IGNORE_CMAKE_CXX17_CHECKS)
  # Needed to make sure libraries and executables not built by the
  # vtkModuleMacros still have the C++17 compiler flags enabled
  # Wrap this in an escape hatch for unknown compilers
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  set(CMAKE_CXX_EXTENSIONS False)
endif()
