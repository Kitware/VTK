IF(JAVA_COMPILE)
  SET(VTK_JAVA_DEPENDENCIES "")
  INCLUDE(${VTK_BINARY_DIR}/Common/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/Filtering/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/IO/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/Graphics/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/GenericFiltering/JavaDependencies.cmake OPTIONAL)
  INCLUDE(${VTK_BINARY_DIR}/Imaging/JavaDependencies.cmake OPTIONAL)
  IF (VTK_USE_RENDERING)
    INCLUDE(${VTK_BINARY_DIR}/Rendering/JavaDependencies.cmake OPTIONAL)
    INCLUDE(${VTK_BINARY_DIR}/VolumeRendering/JavaDependencies.cmake OPTIONAL)
    INCLUDE(${VTK_BINARY_DIR}/Hybrid/JavaDependencies.cmake OPTIONAL)
    INCLUDE(${VTK_BINARY_DIR}/Widgets/JavaDependencies.cmake OPTIONAL)
  ENDIF(VTK_USE_RENDERING)
  IF (VTK_USE_PARALLEL)
    INCLUDE(${VTK_BINARY_DIR}/Parallel/JavaDependencies.cmake OPTIONAL)
  ENDIF(VTK_USE_PARALLEL)
  IF (VTK_USE_INFOVIS)
    INCLUDE(${VTK_BINARY_DIR}/Infovis/JavaDependencies.cmake OPTIONAL)
  ENDIF(VTK_USE_INFOVIS)

  SET(VTK_JAVA_NEW_STYLE 0)
  IF(COMMAND VTK_GENERATE_JAVA_DEPENDENCIES)
    IF(VTK_JAVA_DEPENDENCIES)
      SET(VTK_JAVA_NEW_STYLE 1)
    ENDIF(VTK_JAVA_DEPENDENCIES)
  ENDIF(COMMAND VTK_GENERATE_JAVA_DEPENDENCIES)

  SET(VTK_JAR_PATH "${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}")

  IF(VTK_JAVA_NEW_STYLE)
    SET(CLASS_FILES ${VTK_JAVA_DEPENDENCIES})
    STRING(REGEX REPLACE "\\.java" ".class;" CLASS_FILES ${VTK_JAVA_DEPENDENCIES})
    ADD_CUSTOM_TARGET(VTKJava ALL)
    ADD_CUSTOM_TARGET(VTKJavaJar ALL)

    SET(KITS Common Filtering IO Graphics GenericFiltering Imaging)
    IF (VTK_USE_RENDERING)
      SET(KITS ${KITS} Rendering)
      SET(KITS ${KITS} VolumeRendering)
      SET(KITS ${KITS} Hybrid)
      SET(KITS ${KITS} Widgets)
    ENDIF(VTK_USE_RENDERING)
    IF (VTK_USE_PARALLEL)
      SET(KITS ${KITS} Parallel)
    ENDIF(VTK_USE_PARALLEL)
    IF (VTK_USE_INFOVIS)
      SET(KITS ${KITS} Infovis)
    ENDIF(VTK_USE_INFOVIS)

    # Some platforms have libjvm in a subdirectory from
    # the rest of the java libraries and are missing the symlink.
    IF(JAVA_AWT_LIBRARY)
      FOREACH(__java_library ${JAVA_AWT_LIBRARY})
        GET_FILENAME_COMPONENT(JAVA_LIB_DIR ${__java_library} PATH)
        IF(EXISTS ${JAVA_LIB_DIR}/xawt)
          LINK_DIRECTORIES(${JAVA_LIB_DIR}/xawt)
        ENDIF(EXISTS ${JAVA_LIB_DIR}/xawt)
        IF(EXISTS ${JAVA_LIB_DIR}/client)
          LINK_DIRECTORIES(${JAVA_LIB_DIR}/client)
        ENDIF(EXISTS ${JAVA_LIB_DIR}/client)
        IF(EXISTS ${JAVA_LIB_DIR}/server)
          LINK_DIRECTORIES(${JAVA_LIB_DIR}/server)
        ENDIF(EXISTS ${JAVA_LIB_DIR}/server)
      ENDFOREACH(__java_library)
    ENDIF(JAVA_AWT_LIBRARY)
    SET(CMAKE_SKIP_RPATH 0)
    ADD_EXECUTABLE(VTKJavaExecutable VTKJava.cxx)
    SET(JAVA_LIBRARIES)

    SET(VTK_BUILD_ALL_CONTENT "")
    SET(VTK_BUILD_ALL_DEPENDS "")

    FOREACH(kit ${KITS})
      IF(APPLE)
        SET(src ${VTK_JAR_PATH}/libvtk${kit}Java.dylib)
        SET(tgt ${VTK_JAR_PATH}/libvtk${kit}Java.jnilib)
        ADD_CUSTOM_COMMAND(OUTPUT ${tgt}
                           COMMAND ln
                           ARGS -sf ${src} ${tgt}
                           DEPENDS ${src})
        SET(JAVA_LIBRARIES ${JAVA_LIBRARIES} ${tgt})
      ENDIF(APPLE)
      TARGET_LINK_LIBRARIES(VTKJavaExecutable vtk${kit}Java)
      SET(VTK_JAVA_DEPENDENCIES "")
      ADD_CUSTOM_TARGET(VTKJava${kit} ALL)
      SET(VTK_BUILD_ALL_CONTENT
        "${VTK_BUILD_ALL_CONTENT}\n    vtk.vtkVTKJava${kit}Driver.Initialize(args);")
      SET(VTK_BUILD_ALL_DEPENDS
        ${VTK_BUILD_ALL_DEPENDS} 
        ${VTK_BINARY_DIR}/java/vtk/vtkVTKJava${kit}Driver.java)
      INCLUDE(${VTK_BINARY_DIR}/${kit}/JavaDependencies.cmake OPTIONAL)
      VTK_GENERATE_JAVA_DEPENDENCIES(VTKJava${kit} ${VTK_JAVA_DEPENDENCIES})
      #ADD_DEPENDENCIES(VTKJava${kit} VTKJavaExecutable)
      ADD_DEPENDENCIES(VTKJava${kit} vtk${kit}JavaJavaClasses)
      #ADD_DEPENDENCIES(VTKJavaJar VTKJava${kit})
    ENDFOREACH(kit)

    CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Java/vtkBuildAllDriver.java.in
      ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.java)
    ADD_CUSTOM_TARGET(VTKBuildAll ALL)

#    ADD_CUSTOM_COMMAND(OUTPUT ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.class
#      COMMAND ${JAVA_COMPILE}
#      ARGS -classpath ${VTK_JAVA_HOME}/.. -d ${VTK_JAVA_HOME}/.. ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.java
#      DEPENDS ${VTK_BUILD_ALL_DEPENDS})

    ADD_CUSTOM_COMMAND(
      TARGET VTKBuildAll
      OUTPUTS ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.class
      DEPENDS ${VTK_BUILD_ALL_DEPENDS}
      SOURCE ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.java
      COMMAND ${JAVA_COMPILE}
      ARGS -classpath ${VTK_JAVA_HOME}/.. -d ${VTK_JAVA_HOME}/.. 
      ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.java
      )

    ADD_CUSTOM_COMMAND(
      TARGET VTKBuildAll
      SOURCE VTKBuildAll
      DEPENDS ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.class
      )

    ADD_DEPENDENCIES(VTKJavaJar VTKBuildAll)

    # Make sure all the VTKJava${kit} projects build *before* VTKBuildAll.
    # (All individual generated .java files should be compiled into .class
    # files before compiling the "driver" sources...)
    #
    FOREACH(kit ${KITS})
      ADD_DEPENDENCIES(VTKBuildAll VTKJava${kit})
    ENDFOREACH(kit)

    ADD_DEPENDENCIES(VTKJavaFiltering VTKJavaCommon)
    ADD_DEPENDENCIES(VTKJavaIO VTKJavaFiltering)
    ADD_DEPENDENCIES(VTKJavaGraphics VTKJavaIO)
    ADD_DEPENDENCIES(VTKJavaGenericFiltering VTKJavaGraphics)
    ADD_DEPENDENCIES(VTKJavaImaging VTKJavaFiltering)

    SET(VTK_JAVA_DEPENDENCIES vtkTesting2 vtkSettings)
    IF(VTK_USE_RENDERING)
      SET(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES} vtkCanvas vtkPanel)
      SET(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES} vtkPanel)
      ADD_DEPENDENCIES(VTKJavaRendering VTKJavaGraphics VTKJavaImaging)
      ADD_DEPENDENCIES(VTKJavaVolumeRendering VTKJavaRendering)
      ADD_DEPENDENCIES(VTKJavaHybrid VTKJavaRendering)
      ADD_DEPENDENCIES(VTKJavaWidgets VTKJavaHybrid)
    ENDIF(VTK_USE_RENDERING)
    IF(VTK_USE_PARALLEL)
      ADD_DEPENDENCIES(VTKJavaParallel VTKJavaRendering)
    ENDIF(VTK_USE_PARALLEL)
    IF(VTK_USE_INFOVIS)
      ADD_DEPENDENCIES(VTKJavaInfovis VTKJavaWidgets)
    ENDIF(VTK_USE_INFOVIS)

    FOREACH(jfile ${VTK_JAVA_DEPENDENCIES})
      SET(src ${VTK_SOURCE_DIR}/Wrapping/Java/vtk/${jfile}.java)
      SET(dst ${VTK_JAVA_HOME}/${jfile}.java)
      SET(class ${VTK_JAVA_HOME}/${jfile}.class)
      IF(${jfile} MATCHES vtkSettings)
        CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Java/vtk/vtkSettings.java.in
          ${VTK_JAVA_HOME}/vtkSettings.java)
      ELSE(${jfile} MATCHES vtkSettings)
        ADD_CUSTOM_COMMAND(
          TARGET VTKJavaJar
          SOURCE ${src}
          OUTPUTS ${dst}
          COMMAND ${CMAKE_COMMAND}
          ARGS -E copy ${src} ${dst}
          )
      ENDIF(${jfile} MATCHES vtkSettings)

      ADD_CUSTOM_COMMAND(
        TARGET VTKJavaJar
        SOURCE ${dst}
        OUTPUTS ${class}
        DEPENDS ${VTK_JAVA_HOME}/vtkPanel.java
        COMMAND ${JAVA_COMPILE}
        ARGS -classpath ${VTK_JAVA_HOME}/.. -d ${VTK_JAVA_HOME}/.. ${dst}
        )

      SET(CLASS_FILES ${CLASS_FILES} ${class})
    ENDFOREACH(jfile)
    ADD_DEPENDENCIES(VTKJava VTKJavaJar)

    ADD_CUSTOM_COMMAND(SOURCE ${VTK_JAVA_HOME}/vtkObject.class
      COMMAND ${JAVA_ARCHIVE}
      ARGS -cvf  "${VTK_JAR_PATH}/vtk.jar"
      -C ${VTK_BINARY_DIR}/java
      vtk
      TARGET VTKJavaJar
      DEPENDS ${CLASS_FILES} ${JAVA_LIBRARIES}
      OUTPUTS ${VTK_JAR_PATH}/vtk.jar
      COMMENT "Java Archive")

    ADD_CUSTOM_COMMAND(SOURCE VTKJavaJar
      TARGET VTKJavaJar
      DEPENDS ${VTK_JAR_PATH}/vtk.jar)

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
          ARGS -classpath ${VTK_JAR_PATH}/vtk.jar
          -sourcepath ${VTK_SOURCE_DIR}/Wrapping/Java
          -d ${VTK_BINARY_DIR}/Wrapping/Java
          ${VTK_SOURCE_DIR}/Wrapping/Java/${jfile}.java
          TARGET VTKJava
          DEPENDS ${VTK_JAR_PATH}/vtk.jar
          OUTPUTS ${VTK_BINARY_DIR}/Wrapping/Java/${jfile}.class
          COMMENT "Java Test")
        SET(JAVA_TESTS ${JAVA_TESTS} ${VTK_BINARY_DIR}/Wrapping/Java/${jfile}.class)
      ENDFOREACH(jfile)

      IF(WIN32)
        SET(SEPARATOR "\;")
      ELSE(WIN32)
        SET(SEPARATOR ":")
      ENDIF(WIN32)

      # Since vtkTesting is within vtkRendering we need blockers
      IF(VTK_USE_RENDERING AND VTK_USE_DISPLAY)
        ADD_TEST(JavaRegression ${JAVA_RUNTIME} 
          -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
          Regression -D ${VTK_DATA_ROOT}
          -V Baseline/Graphics/Cone.png)
      ENDIF(VTK_USE_RENDERING AND VTK_USE_DISPLAY)
    ENDIF(BUILD_TESTING)

    ADD_CUSTOM_COMMAND( 
      SOURCE    VTKJava
      TARGET    VTKJava
      DEPENDS   ${VTK_JAR_PATH}/vtk.jar ${JAVA_TESTS}
      )   

  ELSE(VTK_JAVA_NEW_STYLE)

    ADD_CUSTOM_TARGET(VTKJava ALL)

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
        -cvf "\"${VTK_JAR_PATH}/vtk.jar\""
        vtk
        TARGET VTKJava
        DEPENDS ${VTK_JAVA_HOME}/vtkObject.class
        OUTPUTS ${VTK_JAR_PATH}/vtk.jar)

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
            ARGS -classpath ${VTK_JAR_PATH}/vtk.jar
            -sourcepath ${VTK_SOURCE_DIR}/Wrapping/Java
            -d ${VTK_BINARY_DIR}/Wrapping/Java
            ${VTK_SOURCE_DIR}/Wrapping/Java/${jfile}.java
            TARGET VTKJava
            DEPENDS ${VTK_JAR_PATH}/vtk.jar
            OUTPUTS ${VTK_BINARY_DIR}/Wrapping/Java/${jfile}.class)
          SET(JAVA_TESTS ${JAVA_TESTS} ${VTK_BINARY_DIR}/Wrapping/Java/${jfile}.class)
        ENDFOREACH(jfile)

        IF(WIN32)
          SET(SEPARATOR "\;")
        ELSE(WIN32)
          SET(SEPARATOR ":")
        ENDIF(WIN32)

        # Since vtkTesting is within vtkRendering we need blockers
        IF(VTK_USE_RENDERING AND VTK_USE_DISPLAY)
          ADD_TEST(JavaRegression ${JAVA_RUNTIME} 
            -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
            Regression -D ${VTK_DATA_ROOT}
            -V Baseline/Graphics/Cone.png)
        ENDIF(VTK_USE_RENDERING AND VTK_USE_DISPLAY)
      ENDIF(BUILD_TESTING)

      ADD_CUSTOM_COMMAND( 
        SOURCE    VTKJava
        COMMAND   echo 
        ARGS "Checking dependencies for VTK Java"
        TARGET    VTKJava
        DEPENDS   ${VTK_JAR_PATH}/vtk.jar ${JAVA_TESTS}
        )   

    ENDIF(JAVA_ARCHIVE)
  ENDIF(VTK_JAVA_NEW_STYLE)

  IF(NOT VTK_INSTALL_NO_RUNTIME)
    INSTALL(FILES
      ${VTK_JAR_PATH}/vtk.jar
      DESTINATION ${VTK_INSTALL_JAVA_DIR_CM24}/vtk
      COMPONENT RuntimeLibraries)
  ENDIF(NOT VTK_INSTALL_NO_RUNTIME)
ENDIF(JAVA_COMPILE)
