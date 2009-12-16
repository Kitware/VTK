# Stub for backwards compatibility - includes the CMake FindPythonLibs module.
# You should change any includes to read include(FindPythonLibs) so that CMake
# will search its modules directory if the file is not present in VTK's module\
# directory. This stub will be removed in a future release.

# Issue a warning that the FindPythonLibs module will be removed, include the
# CMake module instead of directly referencing the file path
if(NOT _WARNED_ABOUT_FINDPYTHONLIBS_MOVE)
  set(_WARNED_ABOUT_FINDPYTHONLIBS_MOVE TRUE CACHE BOOL
    "The FindPythonLibs.cmake copy will be removed in a future VTK release.")
  mark_as_advanced(_WARNED_ABOUT_FINDPYTHONLIBS_MOVE)
  message(STATUS "WARNING: This module will be removed in a future VTK release. Please use INCLUDE(FindPythonLibs) to include the CMake module if the VTK copy is not present.")
endif()

# Include the FindPythonLibs module bundled with CMake
include(${CMAKE_ROOT}/Modules/FindPythonLibs.cmake)
