foreach(module ${VTK_MODULES_ENABLED})
  if(NOT VTK_MODULE_${module}_EXCLUDE_FROM_WRAPPING)
    include(${${module}_BINARY_DIR}/JavaDependencies.cmake OPTIONAL)
    string(REGEX REPLACE "^vtk" "" kit_name "${module}")
    set(_MODULES_TO_WRAP ${_MODULES_TO_WRAP} ${module})
    set(KITS ${KITS} ${kit_name}) 
  endif()
endforeach()

IF(NOT VTK_JAR_PATH)
  SET(VTK_JAR_PATH "${LIBRARY_OUTPUT_PATH}")
ENDIF(NOT VTK_JAR_PATH)

SET(CLASS_FILES ${VTK_JAVA_DEPENDENCIES})
STRING(REGEX REPLACE "\\.java" ".class;" CLASS_FILES ${VTK_JAVA_DEPENDENCIES})
ADD_CUSTOM_TARGET(VTKJava ALL)
ADD_CUSTOM_TARGET(VTKJavaJar ALL)
ADD_CUSTOM_TARGET(VTKJavaSampleJar ALL)

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
include_directories(${vtkCommonCore_SOURCE_DIR})
include_directories(${vtkCommonCore_BINARY_DIR})
ADD_EXECUTABLE(VTKJavaExecutable VTKJava.cxx)
SET(JAVA_LIBRARIES)

SET(VTK_BUILD_ALL_CONTENT "")
SET(VTK_BUILD_ALL_DEPENDS "")

FOREACH(module ${_MODULES_TO_WRAP})
      
  string(REGEX REPLACE "^vtk" "" kit "${module}")
    
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
  INCLUDE(${${module}_BINARY_DIR}/JavaDependencies.cmake OPTIONAL)
  if(VTK_JAVA_DEPENDENCIES)
    vtk_generate_java_dependencies(VTKJava${kit} ${VTK_JAVA_DEPENDENCIES})
  endif()
 
  add_dependencies(VTKJava${kit} VTKJavaExecutable)
  add_dependencies(VTKJava${kit} vtk${kit}JavaJavaClasses)
  add_dependencies(VTKJavaJar VTKJava${kit})
  # Make sure all .java files are build before any .class files are built.
  FOREACH(otherKit ${KITS})
    ADD_DEPENDENCIES(VTKJava${kit} vtk${otherKit}JavaJavaClasses)
  ENDFOREACH(otherKit)
ENDFOREACH(module)

CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Java/vtkBuildAllDriver.java.in
               ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.java)
ADD_CUSTOM_TARGET(VTKBuildAll ALL)

CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Java/vtk/vtkSettings.java.in
               ${VTK_BINARY_DIR}/java/vtk/vtkSettings.java)

ADD_CUSTOM_COMMAND(
  TARGET VTKBuildAll
  OUTPUTS ${VTK_BINARY_DIR}/java/vtkBuildAllDriver.class
  DEPENDS ${VTK_BUILD_ALL_DEPENDS}
  SOURCE ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.java
  COMMAND ${JAVA_COMPILE}
  ARGS -classpath ${VTK_JAVA_HOME}/.. -d ${VTK_JAVA_HOME}/.. 
  ${VTK_BINARY_DIR}/java/vtk/vtkBuildAllDriver.java
  )

ADD_CUSTOM_COMMAND(
  TARGET VTKBuildAll
  SOURCE VTKBuildAll
  DEPENDS ${VTK_BINARY_DIR}/java/vtkBuildAllDriver.class
  )

ADD_DEPENDENCIES(VTKJavaJar VTKBuildAll)

# Make sure all the VTKJava${kit} projects build *before* VTKBuildAll.
# (All individual generated .java files should be compiled into .class
# files before compiling the "driver" sources...)
#
FOREACH(kit ${KITS})
  ADD_DEPENDENCIES(VTKBuildAll VTKJava${kit})
  ADD_DEPENDENCIES(VTKBuildAll vtk${kit}Java)
ENDFOREACH(kit)

SET(VTK_JAVA_DEPENDENCIES
  CellType
  vtkReferenceInformation
  vtkJavaGarbageCollector
  vtkJavaMemoryManager
  vtkJavaMemoryManagerImpl
  vtkNativeLibrary
  vtkSettings
  )
    
set(VTK_JAVA_SAMPLE_DEPENDENCIES)
    
if(vtkRenderingCore_ENABLED)
  set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
    vtkJavaTesting
    AxesActor
    vtkRenderWindowPanel
    )
endif()
    
if(vtkInteractionWidgets_ENABLED)
  set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
    vtkCanvas)
endif()
    
if(vtkRenderingCore_ENABLED AND vtkIOImage_ENABLED)
  set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
    vtkPanel)
  set(VTK_JAVA_SAMPLE_DEPENDENCIES ${VTK_JAVA_SAMPLE_DEPENDENCIES}
    InternalFrames
    Demo
    SimpleVTK
    VTKCanvas
    ReleaseVtkPanel)    
endif()

if(vtkFiltersModeling_ENABLED AND vtkIOImage_ENABLED)
  set(VTK_JAVA_SAMPLE_DEPENDENCIES ${VTK_JAVA_SAMPLE_DEPENDENCIES}
    ImagePlaneWidget)
endif()
        
FOREACH(jfile ${VTK_JAVA_DEPENDENCIES})
  SET(src ${VTK_SOURCE_DIR}/Wrapping/Java/vtk/${jfile}.java)
  SET(dst ${VTK_JAVA_HOME}/${jfile}.java)
  SET(class ${VTK_JAVA_HOME}/${jfile}.class)
  IF(jfile MATCHES "vtkSettings")
  ELSE(jfile MATCHES "vtkSettings")
    CONFIGURE_FILE(${src} ${dst} COPYONLY)
  ENDIF(jfile MATCHES "vtkSettings")

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

FOREACH(jfile ${VTK_JAVA_SAMPLE_DEPENDENCIES})
  SET(src ${VTK_SOURCE_DIR}/Wrapping/Java/vtk/sample/${jfile}.java)
  SET(dst ${VTK_JAVA_HOME}/sample/${jfile}.java)
  SET(class ${VTK_JAVA_HOME}/sample/${jfile}.class)
  CONFIGURE_FILE(${src} ${dst} COPYONLY)

  ADD_CUSTOM_COMMAND(
    TARGET VTKJavaSampleJar
    SOURCE ${dst}
    OUTPUTS ${class}
    DEPENDS ${VTK_JAVA_HOME}/vtkPanel.class
    COMMAND ${JAVA_COMPILE}
    ARGS -classpath ${VTK_JAVA_HOME}/.. -d ${VTK_JAVA_HOME}/.. ${dst}
    )

  SET(CLASS_FILES ${CLASS_FILES} ${class})
ENDFOREACH(jfile)

ADD_DEPENDENCIES(VTKJava VTKJavaSampleJar)

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
  IF(WIN32)
    SET(SEPARATOR "\;")
  ELSE(WIN32)
    SET(SEPARATOR ":")
  ENDIF(WIN32)

  FOREACH(jfile
    Regression
    ConcurrencyGC
    JavaDelete
    ManualGC
   JavaGCAndDelete)
 
    ADD_CUSTOM_COMMAND(
      SOURCE ${VTK_SOURCE_DIR}/Wrapping/Java/vtk/test/${jfile}.java
      COMMAND ${JAVA_COMPILE}
      ARGS -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      -sourcepath ${VTK_SOURCE_DIR}/Wrapping/Java
      -d ${VTK_BINARY_DIR}/Wrapping/Java
      ${VTK_SOURCE_DIR}/Wrapping/Java/vtk/test/${jfile}.java
      TARGET VTKJava
      DEPENDS ${VTK_JAR_PATH}/vtk.jar
      OUTPUTS ${VTK_BINARY_DIR}/Wrapping/Java/vtk/test/${jfile}.class
      COMMENT "Java Test")
      SET(JAVA_TESTS ${JAVA_TESTS} ${VTK_BINARY_DIR}/Wrapping/Java/vtk/test/${jfile}.class)
  ENDFOREACH(jfile)

  # Since vtkTesting is within vtkTestingRendering we need blockers
  if(vtkTestingRendering_ENABLED)

    # Require Rendering
    if(vtkRenderingOpenGL_ENABLED)
      # If JAVA_DATAMODEL_FLAG is set, pass it as first argument. Otherwise, it's empty.
      # For example, use "-d64" to force 64-bit java jvm.
      #
      ADD_TEST(JavaRegression ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG} 
         -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
         vtk.test.Regression -D ${VTK_DATA_ROOT}
         -V Baseline/Graphics/Cone.png 
         -T ${VTK_BINARY_DIR}/Testing/Temporary)
    endif()

    ADD_TEST(JavaGCRegression ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG} 
      -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      vtk.test.ConcurrencyGC
      -T ${VTK_BINARY_DIR}/Testing/Temporary)

    ADD_TEST(JavaDeleteRegression ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG} 
      -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      vtk.test.JavaDelete
     -T ${VTK_BINARY_DIR}/Testing/Temporary)

    ADD_TEST(JavaManualGC ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG} 
      -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      vtk.test.ManualGC
      -T ${VTK_BINARY_DIR}/Testing/Temporary)

    ADD_TEST(JavaDeleteAndGC ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG} 
      -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      vtk.test.JavaGCAndDelete
     -T ${VTK_BINARY_DIR}/Testing/Temporary)
  endif()      
ENDIF(BUILD_TESTING)

ADD_CUSTOM_COMMAND( 
  SOURCE    VTKJava
  TARGET    VTKJava
  DEPENDS   ${VTK_JAR_PATH}/vtk.jar ${JAVA_TESTS}
  )   

#  IF(NOT VTK_INSTALL_NO_RUNTIME)
#    INSTALL(FILES
#      ${VTK_JAR_PATH}/vtk.jar
#      DESTINATION ${VTK_INSTALL_JAVA_DIR_CM24}
#      COMPONENT RuntimeLibraries)
#  ENDIF(NOT VTK_INSTALL_NO_RUNTIME)
