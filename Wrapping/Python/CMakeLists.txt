# Only do this on CMake 1.4

IF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)
  IF (CMAKE_MINOR_VERSION MATCHES [4-9][0-9]*)

# Create a setup.py file.
    CONFIGURE_FILE(
      ${VTK_SOURCE_DIR}/Wrapping/Python/setup.py.in 
      ${VTK_BINARY_DIR}/Wrapping/Python/setup.py)

# create the VTK/Python  executable
    CONFIGURE_FILE(
      ${VTK_SOURCE_DIR}/Wrapping/Python/vtkpython.cxx.in 
      ${VTK_BINARY_DIR}/Wrapping/Python/vtkpython.cxx)
    ADD_EXECUTABLE(vtkpython ${VTK_BINARY_DIR}/Wrapping/Python/vtkpython.cxx)

    IF (APPLE)
      OPTION(VTK_HAVE_FRAMEWORK_PYTHON "The system has Python installed by default (Mac OSX 10.2)" OFF)
      IF (VTK_HAVE_FRAMEWORK_PYTHON)
        TARGET_LINK_LIBRARIES (vtkpython "-framework Python")
      ELSE (VTK_HAVE_FRAMEWORK_PYTHON)
        TARGET_LINK_LIBRARIES (vtkpython ${PYTHON_LIBRARY})
      ENDIF (VTK_HAVE_FRAMEWORK_PYTHON)
      TARGET_LINK_LIBRARIES (vtkpython vtkIO vtkGraphics vtkImaging)
    ELSE (APPLE)
      IF(WIN32)
        TARGET_LINK_LIBRARIES (vtkpython
                               debug ${PYTHON_DEBUG_LIBRARY}
                               optimized ${PYTHON_LIBRARY})
      ELSE(WIN32)
        TARGET_LINK_LIBRARIES (vtkpython ${PYTHON_LIBRARY})
      ENDIF(WIN32)
      TARGET_LINK_LIBRARIES (vtkpython
                             vtkIOPython vtkGraphicsPython vtkImagingPython)

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
    ENDIF (APPLE)

    FIND_LIBRARY(UTIL_LIBRARY
      NAMES util
      PATHS /usr/lib
    )
    IF (UTIL_LIBRARY)
      TARGET_LINK_LIBRARIES(vtkpython ${UTIL_LIBRARY})
    ENDIF (UTIL_LIBRARY) 
    INSTALL_TARGETS(/bin vtkpython)    

# Allow the user to customize their build with some local options
#
    INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
    INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)

  ENDIF (CMAKE_MINOR_VERSION MATCHES [4-9][0-9]*)
ENDIF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)
