IF(JAVA_COMPILE)
  SET(VTK_JAVA_DEPENDENCIES "")
  INCLUDE(${VTK_BINARY_DIR}/Common/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/Filtering/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/Graphics/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/IO/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/Imaging/JavaDependencies.cmake OPTIONAL)
  IF (VTK_USE_HYBRID)
    INCLUDE(${VTK_BINARY_DIR}/Hybrid/JavaDependencies.cmake OPTIONAL)
  ENDIF(VTK_USE_HYBRID)
  IF (VTK_USE_PARALLEL)
    INCLUDE(${VTK_BINARY_DIR}/Parallel/JavaDependencies.cmake OPTIONAL)
  ENDIF(VTK_USE_PARALLEL)
  IF (VTK_USE_PATENTED)
    INCLUDE(${VTK_BINARY_DIR}/Patented/JavaDependencies.cmake OPTIONAL)
  ENDIF(VTK_USE_PATENTED)
  IF (VTK_USE_RENDERING)
    INCLUDE(${VTK_BINARY_DIR}/Rendering/JavaDependencies.cmake OPTIONAL)
  ENDIF(VTK_USE_RENDERING)
  SET(VTK_JAVA_NEW_STYLE 0)
  IF(COMMAND VTK_GENERATE_JAVA_DEPENDENCIES)
    IF(VTK_JAVA_DEPENDENCIES)
      SET(VTK_JAVA_NEW_STYLE 1)
    ENDIF(VTK_JAVA_DEPENDENCIES)
  ENDIF(COMMAND VTK_GENERATE_JAVA_DEPENDENCIES)

  IF(VTK_JAVA_NEW_STYLE)
    ADD_CUSTOM_TARGET(VTKJava ALL)
    SET(JAVA_TESTS "")
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
    VTK_GENERATE_JAVA_DEPENDENCIES(VTKJava vtk.jar ${VTK_JAVA_DEPENDENCIES} DEPENDS ${JAVA_TESTS})

  ELSE(VTK_JAVA_NEW_STYLE)

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
                              -cvf "\"${LIBRARY_OUTPUT_PATH}/vtk.jar\""
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

      INSTALL_FILES(/lib/vtk/java FILES ${LIBRARY_OUTPUT_PATH}/vtk.jar)
    ENDIF(JAVA_ARCHIVE)
  ENDIF(VTK_JAVA_NEW_STYLE)
ENDIF(JAVA_COMPILE)
