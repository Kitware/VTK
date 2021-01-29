#=========================================================================
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.  See the above copyright notice for more information.
#
#=========================================================================

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
          NAMES libasan.so.5
          DOC "ASan library")
        mark_as_advanced(VTK_ASAN_LIBRARY)

        set(_vtk_testing_ld_preload
          "${VTK_ASAN_LIBRARY}")
      endif ()
    endif ()
  endif()
endif()
