IF(JAVA_COMPILE)
  ADD_EXECUTABLE(VTKJava VTKJava.cxx)
  ADD_CUSTOM_COMMAND(SOURCE VTKJava
                     COMMAND ${JAVA_COMPILE} 
		     ARGS ${VTK_BINARY_DIR}/java/vtk/vtk*.java
                     TARGET VTKJava
		     OUTPUTS ${VTK_BINARY_DIR}/java/vtk/vtkObject.class)
  IF(JAVA_ARCHIVE)
    ADD_CUSTOM_COMMAND(SOURCE VTKJava
	  	       COMMAND ${CMAKE_COMMAND}
                       ARGS -E chdir 
                            ${VTK_BINARY_DIR}/java ${JAVA_ARCHIVE}
                            -cvf ${LIBRARY_OUTPUT_PATH}/vtk.jar
                            vtk
		       TARGET VTKJava
		       OUTPUTS ${LIBRARY_OUTPUT_PATH}/vtk.jar)
    IF(BUILD_TESTING)
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
      ENDFOREACH(jfile)
    ENDIF(BUILD_TESTING)
  ENDIF(JAVA_ARCHIVE)

  TARGET_LINK_LIBRARIES (VTKJava vtkIO vtkGraphics vtkImaging)

  IF (VTK_USE_RENDERING)
    TARGET_LINK_LIBRARIES(VTKJava vtkRendering)
  ENDIF (VTK_USE_RENDERING)

  IF (VTK_USE_PATENTED)
    TARGET_LINK_LIBRARIES(VTKJava vtkPatented)
  ENDIF (VTK_USE_PATENTED)

  IF (VTK_USE_HYBRID)
    TARGET_LINK_LIBRARIES(VTKJava vtkHybrid)
  ENDIF (VTK_USE_HYBRID)

  IF (VTK_USE_PARALLEL)
    TARGET_LINK_LIBRARIES(VTKJava vtkParallel)
  ENDIF (VTK_USE_PARALLEL)

ENDIF(JAVA_COMPILE)
