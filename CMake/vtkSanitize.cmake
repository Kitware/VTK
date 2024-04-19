# This code has been adapted from smtk (https://gitlab.kitware.com/cmb/smtk)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
  #Add option for enabling sanitizers
  option(VTK_ENABLE_SANITIZER "Build with sanitizer support." OFF)
  mark_as_advanced(VTK_ENABLE_SANITIZER)

  if(VTK_ENABLE_SANITIZER)
    set(VTK_SANITIZER "address"
      CACHE STRING "The sanitizer to use")
    mark_as_advanced(VTK_SANITIZER)

    if (UNIX AND NOT APPLE)
      # Tests using external binaries need additional help to load the ASan
      # runtime when in use.
      if (VTK_SANITIZER STREQUAL "address" OR
          VTK_SANITIZER STREQUAL "undefined")
        find_library(VTK_ASAN_LIBRARY
          NAMES libasan.so.6 libasan.so.5
          DOC "ASan library")
        mark_as_advanced(VTK_ASAN_LIBRARY)

        set(_vtk_testing_ld_preload
          "${VTK_ASAN_LIBRARY}")
      endif ()
    endif ()

    set(vtk_sanitize_args
      "-fsanitize=${VTK_SANITIZER}")

    if (CMAKE_COMPILER_IS_CLANGXX)
      configure_file(
        "${VTK_SOURCE_DIR}/Utilities/DynamicAnalysis/sanitizer_ignore.txt.in"
        "${VTK_BINARY_DIR}/sanitizer_ignore.txt"
        @ONLY)
      list(APPEND vtk_sanitize_args
        "SHELL:-fsanitize-blacklist=${VTK_BINARY_DIR}/sanitizer_ignore.txt")
    endif ()

    target_compile_options(vtkbuild
      INTERFACE
        "$<BUILD_INTERFACE:${vtk_sanitize_args}>")
    target_link_options(vtkbuild
      INTERFACE
        "$<BUILD_INTERFACE:${vtk_sanitize_args}>")
  endif()
endif()
