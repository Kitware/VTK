CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3 FATAL_ERROR)

#
# Set the project name.
#

PROJECT (VTKMY)

#
# Load CMake commands that you probably should not modify.
#

INCLUDE (${VTKMY_SOURCE_DIR}/CMakeOptions.cmake)

INCLUDE_DIRECTORIES(${VTKMY_BINARY_DIR})

#
# You can put your include path(s) here
#

INCLUDE_DIRECTORIES(${VTKMY_SOURCE_DIR}/Common)
INCLUDE_DIRECTORIES(${VTKMY_SOURCE_DIR}/Imaging)
INCLUDE_DIRECTORIES(${VTKMY_SOURCE_DIR}/Unsorted)

#
# Build examples too ?
#

OPTION(BUILD_EXAMPLES "Build examples." ON)
IF (BUILD_EXAMPLES)
  ADD_SUBDIRECTORY(Examples)
ENDIF (BUILD_EXAMPLES)

#
# Common configuration settings
#
# Do not worry about this one.
#

CONFIGURE_FILE(
  ${VTKMY_SOURCE_DIR}/vtkmyConfigure.h.in
  ${VTKMY_BINARY_DIR}/vtkmyConfigure.h
)



#
# Here is where you can list the sub-directories holding your local
# classes. Sorting classes by 'package' type like VTK does (Common,
# Rendering, Filtering, Imaging, IO, etc.) is a good thing and prevents
# numerous dependencies problems.
#

ADD_SUBDIRECTORY(Common)
ADD_SUBDIRECTORY(Imaging)
ADD_SUBDIRECTORY(Unsorted)
ADD_SUBDIRECTORY(Utilities)
