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
    
      LINK_LIBRARIES (
        vtkImagingPython
        vtkGraphicsPython
        vtkIOPython
        vtkFilteringPython
        vtkCommonPython
        vtkIO
        vtkGraphics
        vtkImaging
        vtkFiltering 
        vtkCommon 
      )
      IF(VTK_WRAP_TCL)
        LINK_LIBRARIES(
          vtkIOTCL
          vtkGraphicsTCL
          vtkImagingTCL
          vtkFilteringTCL
          vtkCommonTCL
        )
      ENDIF(VTK_WRAP_TCL)
      
      IF (VTK_USE_RENDERING)
# add in opengl
        IF (WIN32)
          LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
        ELSE (WIN32)
          IF (OPENGL_LIBRARY)
            LINK_LIBRARIES (${OPENGL_LIBRARY})
          ENDIF (OPENGL_LIBRARY)
        ENDIF (WIN32)
        IF (VTK_MANGLE_MESA)
          LINK_LIBRARIES( ${MESA_LIBRARY})
          IF ( MESA_OS_LIBRARY )
            LINK_LIBRARIES(${MESA_OS_LIBRARY})
          ENDIF ( MESA_OS_LIBRARY )
        ELSE (VTK_MANGLE_MESA)
          IF (MESA_OS_LIBRARY AND VTK_OPENGL_HAS_OSMESA)
            LINK_LIBRARIES(${MESA_OS_LIBRARY})
          ENDIF (MESA_OS_LIBRARY AND VTK_OPENGL_HAS_OSMESA)
        ENDIF (VTK_MANGLE_MESA)
      
# add in the Tk values if found
        IF (TK_INCLUDE_PATH)
          INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
        ENDIF (TK_INCLUDE_PATH)
        IF (TK_XLIB_PATH)
          INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
        ENDIF (TK_XLIB_PATH)
        IF (TK_LIBRARY)
          LINK_LIBRARIES (${TK_LIBRARY})
        ELSE (TK_LIBRARY)
          MESSAGE("VTK_WRAP_TCL and VTK_USE_RENDERING are ON, but the Tk library was not found.  Please set TK_LIBRARY." "Error")
        ENDIF (TK_LIBRARY)
      
# add in xwindows stuff
        IF (VTK_USE_X)
          LINK_LIBRARIES(-lXt)
          LINK_LIBRARIES(${CMAKE_X_LIBS})
          ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
        ENDIF (VTK_USE_X)
      
        LINK_LIBRARIES(vtkRenderingPython vtkRendering)
      ENDIF (VTK_USE_RENDERING)
      
      IF (VTK_USE_PATENTED)
        LINK_LIBRARIES(vtkPatentedPython vtkPatented)
      ENDIF (VTK_USE_PATENTED)
      
      IF (VTK_USE_HYBRID)
        LINK_LIBRARIES(vtkHybridPython vtkHybrid)
        IF (WIN32)
          IF (VTK_USE_VIDEO_FOR_WINDOWS)
            LINK_LIBRARIES (vfw32)
          ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
          IF (VTK_USE_MATROX_IMAGING)
            LINK_LIBRARIES (${MIL_LIBRARY})
          ENDIF (VTK_USE_MATROX_IMAGING)
        ENDIF (WIN32)
      ENDIF (VTK_USE_HYBRID)
      
      IF (VTK_USE_PARALLEL)
        LINK_LIBRARIES(vtkParallelPython vtkParallel)
      
        IF (WIN32)
          LINK_LIBRARIES( wsock32 )
        ENDIF (WIN32)
      
        IF (VTK_USE_MPI)
          INCLUDE (${CMAKE_ROOT}/Modules/FindMPI.cmake)
          IF (MPI_LIBRARY)
            LINK_LIBRARIES (${MPI_LIBRARY})
          ENDIF (MPI_LIBRARY)
          IF (MPI_EXTRA_LIBRARY)
            LINK_LIBRARIES (${MPI_EXTRA_LIBRARY})
          ENDIF (MPI_EXTRA_LIBRARY)
        ENDIF (VTK_USE_MPI)
      ENDIF (VTK_USE_PARALLEL)
      
#
# link in the PNG library if it was found
#
      TARGET_LINK_LIBRARIES(vtkpython vtkpng vtkzlib vtkjpeg vtktiff)
    
      INSTALL_TARGETS(/bin vtkpython)
    
    ENDIF (VTK_BUILD_PYTHON_EXECUTABLE)
    
# Allow the user to customize their build with some local options
#
    INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
    INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
  
  ENDIF (CMAKE_MINOR_VERSION MATCHES [4-9][0-9]*)
ENDIF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)
