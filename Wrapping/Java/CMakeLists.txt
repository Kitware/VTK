find_package(Java REQUIRED)
find_package(JNI REQUIRED)

include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})

# Add the option to package VTK for custom Java packaging
option(VTK_JAVA_INSTALL "Use the Java rules to build the native libraries." OFF)
if(VTK_JAVA_INSTALL)
  set(VTK_CUSTOM_LIBRARY_SUFFIX "" CACHE STRING "" FORCE)
  mark_as_advanced(VTK_CUSTOM_LIBRARY_SUFFIX)
  if(APPLE)
     set(CMAKE_INSTALL_NAME_DIR @loader_path CACHE STRING "" FORCE)
     mark_as_advanced(CMAKE_INSTALL_NAME_DIR)
  endif()
  if(NOT MAVEN_LOCAL_NATIVE_NAME)
    set(MAVEN_LOCAL_NATIVE_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "")
  endif()
  if(NOT MAVEN_NATIVE_ARTIFACTS)
    set(MAVEN_NATIVE_ARTIFACTS "${MAVEN_LOCAL_NATIVE_NAME}" CACHE STRING "")
  endif()
  if(NOT MAVEN_LOCAL_REPOSITORY)
    set(MAVEN_LOCAL_REPOSITORY "\${user.home}/vtk-maven-repos" CACHE STRING "")
  endif()
  if(NOT MAVEN_VTK_GROUP_ID)
    set(MAVEN_VTK_GROUP_ID "kitware.community" CACHE STRING "")
  endif()
  mark_as_advanced(MAVEN_LOCAL_REPOSITORY MAVEN_NATIVE_ARTIFACTS MAVEN_LOCAL_NATIVE_NAME MAVEN_VTK_GROUP_ID)
  set(MAVEN_DEPENDENCY_XML)
endif()

# Add the option for build the SWT component to VTK.
option(VTK_JAVA_SWT_COMPONENT "Should SWT component for Java be built (it requires Eclipse)?" OFF)

if(VTK_JAVA_SWT_COMPONENT)
  find_package(SWT REQUIRED)
endif()

# Add the option for build the JOGL component to VTK.
set(JOGL_VERSION "2.0.2")
option(VTK_JAVA_JOGL_COMPONENT "Need JOGL jar files" OFF)

if(VTK_JAVA_JOGL_COMPONENT)
  set(MAVEN_REPO "$ENV{HOME}/.m2/repository")
  find_file(JOGL_LIB jogl-all-${JOGL_VERSION}.jar PATHS ${MAVEN_REPO}/org/jogamp/jogl/jogl-all/${JOGL_VERSION} )
  find_file(JOGL_GLUE gluegen-rt-${JOGL_VERSION}.jar PATHS ${MAVEN_REPO}/org/jogamp/gluegen/gluegen-rt/${JOGL_VERSION})
  mark_as_advanced(JOGL_LIB JOGL_GLUE)
  if(NOT JOGL_LIB)
     message(ERROR "
     CMake can not build VTK with VTK_JAVA_JOGL_COMPONENT ON if the JOGL jar file is not provided.
     Please set the JOGL_LIB variable.
     ")
  endif()
  if(NOT JOGL_GLUE)
     message(ERROR "
     CMake can not build VTK with VTK_JAVA_JOGL_COMPONENT ON if the GLUE jar file is not provided.
     Please set the JOGL_GLUE variable.
     ")
  endif()
endif()

set(WrappingJava_SRCS
  vtkJavaUtil.cxx
  )

vtk_module_library(vtkWrappingJava ${WrappingJava_SRCS})

# javac runs out memory on Mac so increase heapsize
if(APPLE)
  set(JAVAC_OPTIONS -J-Xmx512m)
endif()

foreach(module ${VTK_MODULES_ENABLED})
  if(NOT ${module}_EXCLUDE_FROM_WRAPPING)
    include(${${module}_BINARY_DIR}/JavaDependencies.cmake OPTIONAL)
    list(APPEND _MODULES_TO_WRAP ${module})
  endif()
endforeach()

if(NOT VTK_JAR_PATH)
  set(VTK_JAR_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif()

set(VTK_JAVA_SOURCE_FILES ${VTK_JAVA_DEPENDENCIES})
string(REGEX REPLACE "\\.java" ".class;" VTK_JAVA_CLASS_FILES ${VTK_JAVA_SOURCE_FILES})

# Setup custom targets
if (BUILD_TESTING)
  add_custom_target(VTKJavaTestClasses ALL DEPENDS ${VTK_BINARY_DIR}/java/javac_tests_stamp.txt)
endif()
add_custom_target(VTKJavaJar ALL DEPENDS ${VTK_JAR_PATH}/vtk.jar)
add_custom_target(VTKJavaClasses ALL DEPENDS ${VTK_BINARY_DIR}/java/javac_stamp.txt)

add_dependencies(VTKJavaJar VTKJavaClasses)
if (BUILD_TESTING)
  add_dependencies(VTKJavaTestClasses VTKJavaJar)
endif()
# Some platforms have libjvm in a subdirectory from
# the rest of the java libraries and are missing the symlink.

if(JAVA_AWT_LIBRARY)
  foreach(__java_library ${JAVA_AWT_LIBRARY})
    get_filename_component(JAVA_LIB_DIR ${__java_library} PATH)
    if(EXISTS ${JAVA_LIB_DIR}/xawt)
      link_directories(${JAVA_LIB_DIR}/xawt)
    endif()
    if(EXISTS ${JAVA_LIB_DIR}/client)
       link_directories(${JAVA_LIB_DIR}/client)
    endif()
    if(EXISTS ${JAVA_LIB_DIR}/server)
      link_directories(${JAVA_LIB_DIR}/server)
    endif()
  endforeach()
endif()

set(CMAKE_SKIP_RPATH 0)
include_directories(${vtkCommonCore_SOURCE_DIR})
include_directories(${vtkCommonCore_BINARY_DIR})
add_executable(VTKJavaExecutable VTKJava.cxx)
set(JAVA_LIBRARIES)

foreach(module ${_MODULES_TO_WRAP})

  string(REGEX REPLACE "^vtk" "" _module_no_prefix "${module}")

  if(APPLE AND NOT VTK_JAVA_INSTALL)
    set(src ${VTK_JAR_PATH}/libvtk${_module_no_prefix}Java.dylib)
    set(tgt ${VTK_JAR_PATH}/libvtk${_module_no_prefix}Java.jnilib)
    add_custom_command(OUTPUT ${tgt}
                       COMMAND ln
                       ARGS -sf ${src} ${tgt}
                       DEPENDS ${src})
    set(JAVA_LIBRARIES ${JAVA_LIBRARIES} ${tgt})
  endif()

  target_link_libraries(VTKJavaExecutable ${module}Java)

  add_custom_target(VTKJava${_module_no_prefix} ALL)
  add_dependencies(VTKJava${_module_no_prefix} VTKJavaExecutable)
  add_dependencies(VTKJava${_module_no_prefix} ${module}JavaJavaClasses)
  add_dependencies(VTKJavaJar VTKJava${_module_no_prefix})
  # Make sure all .java files are build before any .class files are built.
  foreach(otherModule ${_MODULES_TO_WRAP})
    add_dependencies(VTKJava${_module_no_prefix} ${otherModule}JavaJavaClasses)
  endforeach()

  # Make sure all the VTKJava${kit} projects build *before* VTKJavaClasses.
  add_dependencies(VTKJavaClasses VTKJava${_module_no_prefix})
  add_dependencies(VTKJavaClasses ${module}Java)
endforeach()

# KITS is used in vtkSettings.java.in ( module name no prefix )
string(REGEX REPLACE "vtk" "" KITS "${_MODULES_TO_WRAP}")
configure_file(${VTK_SOURCE_DIR}/Wrapping/Java/vtk/vtkSettings.java.in
               ${VTK_BINARY_DIR}/java/vtk/vtkSettings.java)

# configure vtkNativeLibrary.java to enable loading of VTK module libraries
set(VTK_JAVA_NATIVE_LIBRARIES)
foreach(module ${VTK_MODULES_ALL})
  if(NOT ${module}_EXCLUDE_FROM_WRAPPING)
    string(TOUPPER ${module} _enum_name)
    set(_enabled "false")
    if(${${module}_ENABLED})
      set(_enabled "true")
    endif()
    set(_enum_contr_call "    ${_enum_name}(\"${module}Java\", ${_enabled}),\n")
    set(VTK_JAVA_NATIVE_LIBRARIES "${VTK_JAVA_NATIVE_LIBRARIES}${_enum_contr_call}")
  endif()
endforeach()

# replace last , with ;
string(REGEX REPLACE "^(.*),\n$" "\\1;\n" VTK_JAVA_NATIVE_LIBRARIES ${VTK_JAVA_NATIVE_LIBRARIES})

configure_file(${VTK_SOURCE_DIR}/Wrapping/Java/vtk/vtkNativeLibrary.java.in
               ${VTK_BINARY_DIR}/java/vtk/vtkNativeLibrary.java)

# Add to class list so classes get cleaned up
list(APPEND VTK_JAVA_CLASS_FILES ${VTK_BINARY_DIR}/java/vtk/vtkSettings.class)

set(VTK_JAVA_DEPENDENCIES
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
  if (BUILD_TESTING)
  set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
    vtkJavaTesting)
  endif()
  set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
    AxesActor
    vtkRenderWindowPanel
    )
endif()

if(vtkInteractionWidgets_ENABLED)
  set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
    vtkCanvas
    )
endif()

if(vtkRenderingCore_ENABLED AND vtkIOImage_ENABLED)
  set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
    vtkPanel

    rendering/vtkAbstractComponent
    rendering/vtkComponent
    rendering/vtkEventInterceptor
    rendering/vtkInteractorForwarder
    rendering/awt/vtkAwtComponent
    rendering/awt/vtkInternalAwtComponent
    )
  if(VTK_JAVA_SWT_COMPONENT)
    set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
      rendering/swt/vtkSwtComponent
      rendering/swt/vtkInternalSwtComponent
      rendering/swt/vtkSwtInteractorForwarderDecorator
    )
  endif()
  if(VTK_JAVA_JOGL_COMPONENT)
    set(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}
      rendering/jogl/vtkAbstractJoglComponent
      rendering/jogl/vtkJoglCanvasComponent
      rendering/jogl/vtkJoglPanelComponent
      sample/rendering/JoglConeRendering
    )
  endif()
  set(VTK_JAVA_SAMPLE_DEPENDENCIES ${VTK_JAVA_SAMPLE_DEPENDENCIES}
    InternalFrames
    Demo
    SimpleVTK
    VTKCanvas
    ReleaseVtkPanel
    rendering/AwtConeRendering
    rendering/annotation/LabeledCubeAxesActor
    )
  if(VTK_JAVA_SWT_COMPONENT)
    set(VTK_JAVA_SAMPLE_DEPENDENCIES ${VTK_JAVA_SAMPLE_DEPENDENCIES}
      rendering/SwtConeRendering
    )
  endif()
endif()

if(vtkFiltersModeling_ENABLED AND vtkIOImage_ENABLED)
  set(VTK_JAVA_SAMPLE_DEPENDENCIES ${VTK_JAVA_SAMPLE_DEPENDENCIES}
    ImagePlaneWidget
    )
endif()

# Copy over the sample Java files
foreach(jfile ${VTK_JAVA_SAMPLE_DEPENDENCIES})
  set(src ${VTK_SOURCE_DIR}/Wrapping/Java/vtk/sample/${jfile}.java)
  set(dst ${VTK_JAVA_HOME}/sample/${jfile}.java)
  list(APPEND VTK_JAVA_SOURCE_FILES ${dst})
  set(class ${VTK_JAVA_HOME}/sample/${jfile}.class)
  # Add to class list so classes get cleaned up
  list(APPEND VTK_JAVA_CLASS_FILES ${class})
  configure_file(${src} ${dst} COPYONLY)
endforeach()

# Copy over the handcrafted Java classes
foreach(jfile ${VTK_JAVA_DEPENDENCIES})
  set(src ${VTK_SOURCE_DIR}/Wrapping/Java/vtk/${jfile}.java)
  set(dst ${VTK_JAVA_HOME}/${jfile}.java)
  list(APPEND VTK_JAVA_SOURCE_FILES ${dst})
  set(class ${VTK_JAVA_HOME}/${jfile}.class)
  # Add to class list so classes get cleaned up
  list(APPEND VTK_JAVA_CLASS_FILES ${class})
  if(jfile MATCHES "vtkSettings" OR jfile MATCHES "vtkNativeLibrary")
  else()
    configure_file(${src} ${dst} COPYONLY)
  endif()
endforeach()

if(WIN32)
  set(SEPARATOR "\\;")
else()
  set(SEPARATOR ":")
endif()

if(VTK_JAVA_SWT_COMPONENT)
  set(SWT_FILES "${VTK_BINARY_DIR}/java/vtk/rendering/swt/*.java")
endif()

if(VTK_JAVA_JOGL_COMPONENT)
  set(JOGL_FILES "${VTK_BINARY_DIR}/java/vtk/rendering/jogl/*.java" "${VTK_BINARY_DIR}/java/vtk/sample/rendering/JoglConeRendering.java")
endif()

# Handle Java class path
set(JAVA_COMPILATION_CLASSPATH "${VTK_JAVA_HOME}/..")
if(ECLIPSE_SWT_LIBRARIES)
    set(JAVA_COMPILATION_CLASSPATH "${JAVA_COMPILATION_CLASSPATH}${SEPARATOR}${ECLIPSE_SWT_LIBRARIES}")
endif()
if(JOGL_LIB)
    set(JAVA_COMPILATION_CLASSPATH "${JAVA_COMPILATION_CLASSPATH}${SEPARATOR}${JOGL_LIB}")
endif()
if(JOGL_GLUE)
    set(JAVA_COMPILATION_CLASSPATH "${JAVA_COMPILATION_CLASSPATH}${SEPARATOR}${JOGL_GLUE}")
endif()

# Compile the Java classes
add_custom_command(
  OUTPUT ${VTK_BINARY_DIR}/java/javac_stamp.txt
  DEPENDS ${VTK_JAVA_SOURCE_FILES}
  COMMAND ${JAVA_COMPILE} ${JAVAC_OPTIONS}
    -source 1.5 -classpath ${JAVA_COMPILATION_CLASSPATH} -sourcepath ${VTK_SOURCE_DIR}/Wrapping/Java/ -d ${VTK_BINARY_DIR}/java
    ${VTK_BINARY_DIR}/java/vtk/*.java ${VTK_BINARY_DIR}/java/vtk/rendering/*.java ${VTK_BINARY_DIR}/java/vtk/rendering/awt/*.java ${VTK_BINARY_DIR}/java/vtk/sample/*.java ${SWT_FILES} ${JOGL_FILES}
  COMMAND ${CMAKE_COMMAND} -E touch ${VTK_BINARY_DIR}/java/javac_stamp.txt
  COMMENT "Compiling Java Classes"
  )

# Create the JAR file containing all compiled classes and
add_custom_command(
  COMMAND ${JAVA_ARCHIVE} -cvf  "${VTK_JAR_PATH}/vtk.jar"
  -C ${VTK_BINARY_DIR}/java
  vtk
  DEPENDS ${VTK_BINARY_DIR}/java/javac_stamp.txt ${JAVA_LIBRARIES}
  OUTPUT ${VTK_JAR_PATH}/vtk.jar
  COMMENT "Java Archive"
  )

if(BUILD_TESTING)
  foreach(jfile
    Regression
    ConcurrencyGC
    JavaDelete
    ManualGC
    JavaGCAndDelete)
    list(APPEND VTK_JAVA_TESTS ${VTK_SOURCE_DIR}/Wrapping/Java/vtk/test/${jfile}.java)
    # Add to class list so classes get cleaned up
    list(APPEND VTK_JAVA_CLASS_FILES ${VTK_BINARY_DIR}/Wrapping/Java/vtk/test/${jfile}.class)
  endforeach()

  # Compile Java Tests
  add_custom_command(
    OUTPUT ${VTK_BINARY_DIR}/java/javac_tests_stamp.txt
    COMMAND ${JAVA_COMPILE} ${JAVAC_OPTIONS} -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
    -source 1.5 -sourcepath ${VTK_SOURCE_DIR}/Wrapping/Java
    -d ${VTK_BINARY_DIR}/Wrapping/Java
    ${VTK_JAVA_TESTS}
    COMMAND ${CMAKE_COMMAND} -E touch ${VTK_BINARY_DIR}/java/javac_tests_stamp.txt
    DEPENDS ${VTK_JAR_PATH}/vtk.jar ${VTK_JAVA_TESTS}
    COMMENT "Compiling Java Tests"
    )

  # Since vtkTesting is within vtkTestingRendering we need blockers
  if(vtkTestingRendering_ENABLED)

    # Require Rendering
    if(vtkRenderingOpenGL_ENABLED)
      # If JAVA_DATAMODEL_FLAG is set, pass it as first argument. Otherwise, it's empty.
      # For example, use "-d64" to force 64-bit java jvm.
      #
      ExternalData_add_test(VTKData
         JavaRegression ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG}
         -classpath "${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java"
         vtk.test.Regression -D ${VTK_TEST_DATA_DIR}
         -V DATA{Baseline/Cone.png,:}
         -T ${VTK_TEST_OUTPUT_DIR}
         )
    endif()

    add_test(JavaGCRegression ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG}
      -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      vtk.test.ConcurrencyGC
      -T ${VTK_TEST_OUTPUT_DIR}
      )

    add_test(JavaDeleteRegression ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG}
      -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      vtk.test.JavaDelete
     -T ${VTK_TEST_OUTPUT_DIR}
     )

    add_test(JavaManualGC ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG}
      -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      vtk.test.ManualGC
      -T ${VTK_TEST_OUTPUT_DIR}
      )

    add_test(JavaDeleteAndGC ${JAVA_RUNTIME} ${VTK_TEST_JAVA_DATAMODEL_FLAG}
      -classpath ${VTK_JAR_PATH}/vtk.jar${SEPARATOR}${VTK_BINARY_DIR}/Wrapping/Java
      vtk.test.JavaGCAndDelete
     -T ${VTK_TEST_OUTPUT_DIR}
     )
  endif()
endif()

# Make sure .class files are cleaned up
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${VTK_JAVA_CLASS_FILES}")
install(FILES
  ${VTK_JAR_PATH}/vtk.jar
  DESTINATION ${VTK_INSTALL_LIBRARY_DIR}
  COMPONENT RuntimeLibraries)

#-----------------------------------------------------------------------------
# Post-Install script for Custom Java Packaging
# This install rule MUST stay at the bottom of that CMakeLists file as
# it has to be the last install rule that get executed
if(VTK_JAVA_INSTALL)
  set(MAVEN_NATIVE_ARTIFACT_XML)
  foreach(native_name ${MAVEN_NATIVE_ARTIFACTS})
     set(MAVEN_NATIVE_ARTIFACT_XML "${MAVEN_NATIVE_ARTIFACT_XML}\n                                <artifact><file>vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}-natives-${native_name}.jar</file><classifier>natives-${native_name}</classifier><type>jar</type></artifact>")
  endforeach()
  if(VTK_JAVA_JOGL_COMPONENT)
    set(MAVEN_DEPENDENCY_XML "${MAVEN_DEPENDENCY_XML}\n      <dependency><groupId>org.jogamp.jogl</groupId><artifactId>jogl-all-main</artifactId><version>${JOGL_VERSION}</version></dependency>\n      <dependency><groupId>org.jogamp.gluegen</groupId><artifactId>gluegen-rt-main</artifactId><version>${JOGL_VERSION}</version></dependency>")
  endif()
  configure_file(JavaInstall.cmake.in
               ${VTK_BINARY_DIR}/JavaInstall.cmake @ONLY)
  configure_file(Maven/pom.xml.in
               ${VTK_BINARY_DIR}/pom.xml @ONLY)
  configure_file(Maven/README.txt.in
               ${VTK_BINARY_DIR}/MAVEN-README.txt @ONLY)
  install(SCRIPT ${VTK_BINARY_DIR}/JavaInstall.cmake)
endif(VTK_JAVA_INSTALL)
