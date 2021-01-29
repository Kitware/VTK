#=========================================================================
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.  See the above copyright notice for more information.
#
#=========================================================================

# This code has been adapted from smtk (https://gitlab.kitware.com/cmb/smtk)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
  if(VTK_ENABLE_SANITIZER)
    set(vtk_sanitize_args
      "-fsanitize=${VTK_SANITIZER}")

    if (CMAKE_COMPILER_IS_CLANGXX)
      configure_file(
        "${VTK_SOURCE_DIR}/Utilities/DynamicAnalysis/sanitizer_ignore.txt.in"
        "${VTK_BINARY_DIR}/sanitizer_ignore.txt"
        @ONLY)
      string(APPEND vtk_sanitize_args " \"-fsanitize-blacklist=${VTK_BINARY_DIR}/sanitizer_ignore.txt\"")
    endif ()

    # We're setting the CXX flags and C flags beacuse they're propagated down
    # independent of build type.
    string(APPEND CMAKE_CXX_FLAGS " ${vtk_sanitize_args}")
    string(APPEND CMAKE_C_FLAGS " ${vtk_sanitize_args}")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " ${vtk_sanitize_args}")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${vtk_sanitize_args}")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${vtk_sanitize_args}")
  endif ()
endif()
