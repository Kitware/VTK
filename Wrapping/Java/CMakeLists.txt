IF(JAVA_COMPILE)
  ADD_EXECUTABLE(VTKJava VTKJava.cxx)
  ADD_CUSTOM_COMMAND(SOURCE VTKJava
                     COMMAND ${JAVA_COMPILE} 
		     ARGS ${VTK_BINARY_DIR}/java/vtk/vtk*.java
                     TARGET VTKJava
		     OUTPUTS ${VTK_BINARY_DIR}/java/vtk/vtkObject.class)
  IF(JAVA_ARCHIVE)
    ADD_CUSTOM_COMMAND(SOURCE VTKJava
	  	       COMMAND cd 
                       ARGS ${VTK_BINARY_DIR}/java\; ${JAVA_ARCHIVE}
                            -cvf ${LIBRARY_OUTPUT_PATH}/vtk.jar
                            vtk
		       TARGET VTKJava
		       OUTPUTS ${LIBRARY_OUTPUT_PATH}/vtk.jar)
    FOREACH(jfile
            SimpleVTK
            Regression)
      ADD_CUSTOM_COMMAND(SOURCE VTKJava
  		       COMMAND ${JAVA_COMPILE}
                         ARGS -classpath ${LIBRARY_OUTPUT_PATH}/vtk.jar
                              -sourcepath ${VTK_SOURCE_DIR}/Wrapping/Java
                              -d ${VTK_BINARY_DIR}/Wrapping/Java
                              ${VTK_SOURCE_DIR}/Wrapping/Java/${jfile}.java
                         TARGET VTKJava
                         OUTPUTS ${VTK_BINARY_DIR}/Wrapping/Java/${jfile}.class)
    ENDFOREACH(jfile
  IF(JAVA_ARCHIVE)

  LINK_LIBRARIES (
    vtkImagingTCL
    vtkGraphicsTCL
    vtkIOTCL
    vtkFilteringTCL 
    vtkCommonTCL  
    vtkIO
    vtkGraphics
    vtkImaging
    vtkFiltering 
    vtkCommon 
  )
 
  IF (TCL_LIBRARY)
    LINK_LIBRARIES (${TCL_LIBRARY})
  ENDIF (TCL_LIBRARY)

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
      IF (APPLE)
        INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Rendering/tkOSXInternals)
      ENDIF (APPLE)
    ENDIF (TK_INCLUDE_PATH)
    IF (TK_XLIB_PATH)
      INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
    ENDIF (TK_XLIB_PATH)
    IF (TK_LIBRARY)
      LINK_LIBRARIES (${TK_LIBRARY})
    ENDIF (TK_LIBRARY)
  
    # add in xwindows stuff
    IF (VTK_USE_X)
      LINK_LIBRARIES(-lXt)
      LINK_LIBRARIES(${CMAKE_X_LIBS})
      ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
    ENDIF (VTK_USE_X)

    LINK_LIBRARIES(vtkRenderingTCL vtkRendering)
  ENDIF (VTK_USE_RENDERING)

  IF (VTK_USE_PATENTED)
    LINK_LIBRARIES(vtkPatentedTCL vtkPatented)
  ENDIF (VTK_USE_PATENTED)

  IF (VTK_USE_HYBRID)
    LINK_LIBRARIES(vtkHybridTCL vtkHybrid)
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
    LINK_LIBRARIES(vtkParallelTCL vtkParallel)

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
  
  # link in the PNG library if it was found
  #
  TARGET_LINK_LIBRARIES(VTKJava vtkpng vtkzlib vtkjpeg vtktiff vtkexpat)

ENDIF(JAVA_COMPILE)
