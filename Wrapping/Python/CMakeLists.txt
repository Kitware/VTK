# Only do this on CMake 1.4

IF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)
  IF (CMAKE_MINOR_VERSION MATCHES [4-9][0-9]*)

# Create a setup.py file.
    CONFIGURE_FILE(
      ${VTK_SOURCE_DIR}/Wrapping/Python/setup.py.in 
      ${VTK_BINARY_DIR}/Wrapping/Python/setup.py)
    
    MARK_AS_ADVANCED(VTK_BUILD_PYTHON_EXECUTABLE)
    OPTION (VTK_BUILD_PYTHON_EXECUTABLE "Build vtkpython executable, which will correctly initialize static constructors. (EXPERIMENTAL)" OFF)
    IF (VTK_BUILD_PYTHON_EXECUTABLE)

# create the VTK/Python  executable
      CONFIGURE_FILE(
        ${VTK_SOURCE_DIR}/Wrapping/Python/vtkpython.cxx.in 
        ${VTK_BINARY_DIR}/Wrapping/Python/vtkpython.cxx)
      ADD_EXECUTABLE(vtkpython ${VTK_BINARY_DIR}/Wrapping/Python/vtkpython.cxx)
    
      TARGET_LINK_LIBRARIES (vtkpython vtkIO vtkGraphics vtkImaging
                             vtkIOPython vtkGraphicsPython vtkImagingPython)

      IF (VTK_USE_RENDERING)
        TARGET_LINK_LIBRARIES(vtkpython vtkRenderingPython vtkRendering)
      ENDIF (VTK_USE_RENDERING)

      IF (VTK_USE_PATENTED)
        TARGET_LINK_LIBRARIES(vtkpython vtkPatentedPython vtkPatented)
      ENDIF (VTK_USE_PATENTED)

      IF (VTK_USE_HYBRID)
        TARGET_LINK_LIBRARIES(vtkpython vtkHybridPython vtkHybrid)
      ENDIF (VTK_USE_HYBRID)

      IF (VTK_USE_PARALLEL)
        TARGET_LINK_LIBRARIES(vtkpython vtkParallelPython vtkParallel)
      ENDIF (VTK_USE_PARALLEL)

      IF (VTK_WRAP_TCL)
        TARGET_LINK_LIBRARIES (vtkpython vtkIOTCL vtkGraphicsTCL vtkImagingTCL)
  
        IF (VTK_USE_RENDERING)
          TARGET_LINK_LIBRARIES(vtkpython vtkRenderingTCL)
        ENDIF (VTK_USE_RENDERING)
  
        IF (VTK_USE_PATENTED)
          TARGET_LINK_LIBRARIES(vtkpython vtkPatentedTCL)
        ENDIF (VTK_USE_PATENTED)
  
        IF (VTK_USE_HYBRID)
          TARGET_LINK_LIBRARIES(vtkpython vtkHybridTCL)
        ENDIF (VTK_USE_HYBRID)
  
        IF (VTK_USE_PARALLEL)
          TARGET_LINK_LIBRARIES(vtkpython vtkParallelTCL)
        ENDIF (VTK_USE_PARALLEL)
      ENDIF (VTK_WRAP_TCL)
    
      INSTALL_TARGETS(/bin vtkpython)
    
    ENDIF (VTK_BUILD_PYTHON_EXECUTABLE)
    
# Allow the user to customize their build with some local options
#
    INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
    INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
  
  ENDIF (CMAKE_MINOR_VERSION MATCHES [4-9][0-9]*)
ENDIF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)
