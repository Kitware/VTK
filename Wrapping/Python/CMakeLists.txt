# Create a setup.py file.
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/setup.py.in 
               ${VTK_BINARY_DIR}/Wrapping/Python/setup.py)

# create the VTK/Python  executable
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h.in
               ${VTK_BINARY_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h)
ADD_EXECUTABLE(vtkpython vtkPythonAppInit.cxx)

# Link against all the kit wrappers.
TARGET_LINK_LIBRARIES (vtkpython
  vtkCommonPython
  vtkFilteringPython
  vtkIOPython
  vtkGraphicsPython
  vtkImagingPython
)

IF (VTK_USE_RENDERING)
  TARGET_LINK_LIBRARIES(vtkpython vtkRenderingPython)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkpython vtkPatentedPython)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_HYBRID)
  TARGET_LINK_LIBRARIES(vtkpython vtkHybridPython)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PARALLEL)
  TARGET_LINK_LIBRARIES(vtkpython vtkParallelPython)
ENDIF (VTK_USE_PARALLEL)

# Create the pvtkpython Python wrapper executable with MPI support.
IF (VTK_USE_PARALLEL)
  IF (VTK_USE_MPI)
    ADD_EXECUTABLE(pvtkpython vtkParaPythonAppInit.cxx)

    TARGET_LINK_LIBRARIES (pvtkpython
      vtkCommonPython
      vtkFilteringPython
      vtkIOPython
      vtkGraphicsPython
      vtkImagingPython
      vtkParallelPython
    )

    IF (VTK_USE_PATENTED)
      TARGET_LINK_LIBRARIES(pvtkpython vtkPatentedPython)
    ENDIF (VTK_USE_PATENTED)
    IF (VTK_USE_HYBRID)
      TARGET_LINK_LIBRARIES(pvtkpython vtkHybridPython)
    ENDIF (VTK_USE_HYBRID)

    INSTALL_TARGETS(/bin pvtkpython)
  ENDIF (VTK_USE_MPI)
ENDIF (VTK_USE_PARALLEL)

FIND_LIBRARY(UTIL_LIBRARY
  NAMES util
  PATHS /usr/lib
)
IF (UTIL_LIBRARY)
  TARGET_LINK_LIBRARIES(vtkpython ${UTIL_LIBRARY})
  IF (VTK_USE_PARALLEL)
    IF (VTK_USE_MPI)
      TARGET_LINK_LIBRARIES(pvtkpython ${UTIL_LIBRARY})
    ENDIF (VTK_USE_MPI)
  ENDIF (VTK_USE_PARALLEL)
ENDIF (UTIL_LIBRARY) 
INSTALL_TARGETS(/bin vtkpython)    

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
