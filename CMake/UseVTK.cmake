
# This file sets up include directories, link directories, and
# compiler settings for a project to use VTK.  It should not be
# included directly, but rather through the VTK_USE_FILE setting
# obtained from VTKConfig.cmake.

if(VTK_USE_FILE_INCLUDED)
  return()
endif()
set(VTK_USE_FILE_INCLUDED 1)

# Update CMAKE_MODULE_PATH so includes work.
list(APPEND CMAKE_MODULE_PATH ${VTK_CMAKE_DIR})

# Add compiler flags needed to use VTK.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VTK_REQUIRED_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTK_REQUIRED_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${VTK_REQUIRED_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${VTK_REQUIRED_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${VTK_REQUIRED_MODULE_LINKER_FLAGS}")

# Add preprocessor definitions needed to use VTK.
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})

# Add include directories needed to use VTK.
include_directories(${VTK_INCLUDE_DIRS})

# Add link directories needed to use VTK.
link_directories(${VTK_LIBRARY_DIRS})
