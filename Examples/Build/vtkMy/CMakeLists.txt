#
# Set the project name.
#

PROJECT (VTKMY)

#
# Assume everything is set up correctly for build.
#
# If some requirements are not met, set it to 0.
# In that case, the build won't be done.
#

SET (VTKMY_CAN_BUILD 1)

#
# Load CMake commands that you probably should not modify.
#

INCLUDE (${VTKMY_SOURCE_DIR}/CMakeOptions.cmake)

#
# If everything is OK, then go into the sub directories and build.
#

IF (VTKMY_CAN_BUILD)

  #
  # Here is where you can list the sub-directories holding your local 
  # classes. Sorting classes by 'package' type like VTK does (Common, 
  # Rendering, Filtering, Imaging, IO, etc.) is a good thing and prevents 
  # numerous dependencies problems.
  #

  SUBDIRS (
    Common
    Imaging 
    Unsorted
  )

  #
  # You can put your include path(s) here
  #

  INCLUDE_DIRECTORIES(${VTKMY_SOURCE_DIR}/Common)
  INCLUDE_DIRECTORIES (${VTKMY_SOURCE_DIR}/Imaging)
  INCLUDE_DIRECTORIES (${VTKMY_SOURCE_DIR}/Unsorted)

  #
  # Build examples too ?
  #

  OPTION(BUILD_EXAMPLES "Build examples." ON)
  IF (BUILD_EXAMPLES)
    SUBDIRS(Examples)
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

  INCLUDE_DIRECTORIES(${VTKMY_BINARY_DIR})

ENDIF (VTKMY_CAN_BUILD)
