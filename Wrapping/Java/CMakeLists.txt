IF(JAVA_COMPILE)
  ADD_CUSTOM_TARGET(VTKJava ALL echo "Checking for Java library")
  ADD_CUSTOM_COMMAND(SOURCE ${VTK_JAVA_HOME}/VTKJavaWrapped
                     COMMAND ${JAVA_COMPILE} 
		     ARGS ${VTK_JAVA_HOME}/vtk*.java
                     TARGET VTKJava
		     OUTPUTS ${VTK_JAVA_HOME}/vtkObject.class)
  IF(JAVA_ARCHIVE)
    ADD_CUSTOM_COMMAND(SOURCE ${VTK_JAVA_HOME}/vtkObject.class
	  	       COMMAND ${CMAKE_COMMAND}
                       ARGS -E chdir 
                            ${VTK_BINARY_DIR}/java ${JAVA_ARCHIVE}
                            -cvf ${LIBRARY_OUTPUT_PATH}/vtk.jar
                            vtk
		       TARGET VTKJava
                       DEPENDS ${VTK_JAVA_HOME}/vtkObject.class
		       OUTPUTS ${LIBRARY_OUTPUT_PATH}/vtk.jar)

    IF(BUILD_TESTING)
      FOREACH(jfile
              SimpleVTK
              Regression
              TestVTKCanvas
              TestInternalFrames
              ImagePlaneWidget
              AxesActor)
        ADD_CUSTOM_COMMAND(
             SOURCE ${VTK_SOURCE_DIR}/Wrapping/Java/${jfile}.java
             COMMAND ${JAVA_COMPILE}
             ARGS -classpath ${LIBRARY_OUTPUT_PATH}/vtk.jar
                  -sourcepath ${VTK_SOURCE_DIR}/Wrapping/Java
                  -d ${VTK_BINARY_DIR}/Wrapping/Java
                     ${VTK_SOURCE_DIR}/Wrapping/Java/${jfile}.java
             TARGET VTKJava
             DEPENDS ${LIBRARY_OUTPUT_PATH}/vtk.jar
             OUTPUTS ${VTK_BINARY_DIR}/Wrapping/Java/${jfile}.class)
        SET(JAVA_TESTS ${JAVA_TESTS} ${VTK_BINARY_DIR}/Wrapping/Java/${jfile}.class)
      ENDFOREACH(jfile)
      IF(WIN32)
        SET(SEPARATOR "\;")
      ELSE(WIN32)
        SET(SEPARATOR ":")
      ENDIF(WIN32)
      
      ADD_TEST(JavaRegression ${JAVA_RUNTIME} 
               -classpath ${LIBRARY_OUTPUT_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
               Regression -D ${VTK_DATA_ROOT}
                          -V Baseline/Graphics/Cone.png)
    ENDIF(BUILD_TESTING)

    ADD_CUSTOM_COMMAND( 
      SOURCE    VTKJava
      COMMAND   echo 
      ARGS "Checking dependencies for VTK Java"
      TARGET    VTKJava
      DEPENDS   ${LIBRARY_OUTPUT_PATH}/vtk.jar ${JAVA_TESTS}
    )   

    IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
      INSTALL_FILES(/lib/vtk/java FILES ${LIBRARY_OUTPUT_PATH}/vtk.jar)
    ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
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
