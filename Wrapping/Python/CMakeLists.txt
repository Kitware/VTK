# Create a setup.py file.
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/setup.py.in 
               ${VTK_BINARY_DIR}/Wrapping/Python/setup.py)

# create the VTK/Python  executable
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h.in
               ${VTK_BINARY_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h)
ADD_EXECUTABLE(vtkpython vtkPythonAppInit.cxx)

FIND_LIBRARY(UTIL_LIBRARY
  NAMES util
  PATHS /usr/lib
)
IF(UTIL_LIBRARY)
  SET(UTIL_LIBRARY_LIB ${UTIL_LIBRARY})
ENDIF(UTIL_LIBRARY)


# Link against all the kit wrappers.
IF (APPLE)
  SET_TARGET_PROPERTIES(vtkpython PROPERTIES LINK_FLAGS "-flat_namespace -undefined suppress")
  OPTION(VTK_HAVE_FRAMEWORK_PYTHON "The system has Python installed by default (Mac OSX 10.2)" OFF)
  IF (VTK_HAVE_FRAMEWORK_PYTHON)
    SET(PYTHON_LIBRARY "-framework Python")
  ENDIF (VTK_HAVE_FRAMEWORK_PYTHON)
ENDIF (APPLE)

TARGET_LINK_LIBRARIES(vtkpython 
  ${PYTHON_LIBRARY} 
  vtkCommon 
  vtkFiltering
  vtkIO
  vtkGraphics
  vtkImaging
  ${UTIL_LIBRARY_LIB})

IF (VTK_USE_RENDERING)
  TARGET_LINK_LIBRARIES(vtkpython vtkRendering)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkpython vtkPatented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_HYBRID)
  TARGET_LINK_LIBRARIES(vtkpython vtkHybrid)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PARALLEL)
  TARGET_LINK_LIBRARIES(vtkpython vtkParallel)
ENDIF (VTK_USE_PARALLEL)

# Create the pvtkpython Python wrapper executable with MPI support.
IF (VTK_USE_PARALLEL)
  IF (VTK_USE_MPI)
    ADD_EXECUTABLE(pvtkpython vtkParaPythonAppInit.cxx)

    TARGET_LINK_LIBRARIES (pvtkpython
      ${PYTHON_LIBRARY}
      vtkCommon
      vtkFiltering
      vtkIO
      vtkGraphics
      vtkImaging
      vtkParallel
      ${UTIL_LIBRARY_LIB}
    )
    IF (VTK_USE_PATENTED)
      TARGET_LINK_LIBRARIES(pvtkpython vtkPatented)
    ENDIF (VTK_USE_PATENTED)
    IF (VTK_USE_HYBRID)
      TARGET_LINK_LIBRARIES(pvtkpython vtkHybrid)
    ENDIF (VTK_USE_HYBRID)
    INSTALL_TARGETS(/bin pvtkpython)
  ENDIF (VTK_USE_MPI)
ENDIF (VTK_USE_PARALLEL)

INSTALL_TARGETS(/bin vtkpython)    

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
