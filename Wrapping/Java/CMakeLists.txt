set(CMAKE_INSTALL_JNILIBDIR ""
  CACHE STRING "JNI libraries (LIBDIR/java/vtk-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})")
mark_as_advanced(CMAKE_INSTALL_JNILIBDIR)

# Since CMAKE_INSTALL_LIBDIR can mutate, set CMAKE_INSTALL_JNILIBDIR here to
# avoid expanding its content as opposed as setting the default value during
# the declaration of the CACHE var which would then expand its value.
if (NOT CMAKE_INSTALL_JNILIBDIR)
  set(CMAKE_INSTALL_JNILIBDIR
    "${CMAKE_INSTALL_LIBDIR}/java/vtk-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif ()
if (IS_ABSOLUTE "${CMAKE_INSTALL_JNILIBDIR}")
  message(FATAL_ERROR
    "The `CMAKE_INSTALL_JNILIBDIR` must not be an absolute path.")
endif ()

include(vtkModuleWrapJava)
vtk_module_wrap_java(
  JAVA_OUTPUT     "${CMAKE_CURRENT_BINARY_DIR}/vtk"
  MODULES         ${vtk_modules}
  WRAPPED_MODULES vtk_java_wrapped_modules
  UTILITY_TARGET  "VTK::vtkbuild"
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  # Update vtkSettings.java.in if this changes.
  JNILIB_DESTINATION  "${CMAKE_INSTALL_JNILIBDIR}")

# Needed for FindJOGL.cmake
include(UseJava)

option(VTK_JAVA_SWT_COMPONENT "Enable the SWT component for Java" OFF)
if (VTK_JAVA_SWT_COMPONENT)
  find_package(SWT REQUIRED)
endif ()

# Add the option for build the JOGL component to VTK.
option(VTK_JAVA_JOGL_COMPONENT "Need JOGL jar files" OFF)
if (VTK_JAVA_JOGL_COMPONENT)
  find_package(JOGL REQUIRED)
endif ()

set(java_sources
  vtk/CellType
  vtk/vtkJavaGarbageCollector
  vtk/vtkJavaMemoryManager
  vtk/vtkJavaMemoryManagerImpl
  vtk/vtkReferenceInformation)

if (TARGET VTK::RenderingCore)
  list(APPEND java_sources
    vtk/AxesActor)

  if (TARGET VTK::TestingRendering)
    list(APPEND java_sources
      vtk/vtkJavaTesting)
  endif ()

  if (TARGET VTK::IOImage)
    list(APPEND java_sources
      vtk/vtkPanel

      vtk/rendering/vtkAbstractComponent
      vtk/rendering/vtkAbstractEventInterceptor
      vtk/rendering/vtkComponent
      vtk/rendering/vtkEventInterceptor
      vtk/rendering/vtkInteractorForwarder

      vtk/rendering/awt/vtkAwtComponent
      vtk/rendering/awt/vtkInternalAwtComponent
      vtk/rendering/awt/vtkPanelComponent

      vtk/sample/Demo
      vtk/sample/InternalFrames
      vtk/sample/ReleaseVtkPanel
      vtk/sample/SimpleVTK
      vtk/sample/VTKCanvas

      vtk/sample/rendering/AwtConeRendering
      vtk/sample/rendering/annotation/LabeledCubeAxesActor)

    if (VTK_JAVA_SWT_COMPONENT)
      list(APPEND java_sources
        vtk/rendering/swt/vtkInternalSwtComponent
        vtk/rendering/swt/vtkSwtComponent
        vtk/rendering/swt/vtkSwtInteractorForwarderDecorator

        vtk/sample/rendering/SwtConeRendering)
    endif ()

    if (VTK_JAVA_JOGL_COMPONENT)
      list(APPEND java_sources
        vtk/rendering/jogl/vtkAbstractJoglComponent
        vtk/rendering/jogl/vtkJoglCanvasComponent
        vtk/rendering/jogl/vtkJoglPanelComponent

        vtk/sample/rendering/JoglConeRendering)
    endif ()
  endif ()
endif ()

if (TARGET VTK::InteractionWidgets)
  list(APPEND java_sources
    vtk/vtkCanvas)

  if (TARGET VTK::RenderingCore)
    list(APPEND java_sources
      vtk/vtkRenderWindowPanel)
  endif ()
endif ()

if (TARGET VTK::FiltersModeling AND TARGET VTK::IOImage)
  list(APPEND java_sources
    vtk/sample/ImagePlaneWidget)
endif ()

set(_vtk_java_depends_args)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
  list(APPEND _vtk_java_depends_args
    DEPENDS_EXPLICIT_ONLY)
endif ()

set(java_input_files)
set(java_class_files)
foreach (java_file IN LISTS java_sources)
  set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${java_file}.java")
  add_custom_command(
    OUTPUT  "${output_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java"
            "${output_file}"
    COMMENT "Copying ${java_file} to the binary directory"
    ${_vtk_java_depends_args})

  list(APPEND java_input_files
    "${output_file}")
endforeach ()

set(vtk_java_native_libraries)
set(vtk_java_wrapped_module_libraries)
foreach (_vtk_java_module IN LISTS vtk_java_wrapped_modules)
  get_property(_vtk_java_library_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_java_module}_library_name")
  list(APPEND vtk_java_wrapped_module_libraries
    "${_vtk_java_library_name}")
  get_property(_vtk_module_java_files
    TARGET    "${_vtk_java_library_name}Java"
    PROPERTY  "_vtk_module_java_files")
  list(APPEND java_input_files
    ${_vtk_module_java_files})

  string(APPEND vtk_java_native_libraries
    "  ${_vtk_java_library_name}(\"${_vtk_java_library_name}Java\"),\n")
endforeach ()

# Replace the last comma with a semicolon.
string(REGEX REPLACE ",\n$" ";\n" vtk_java_native_libraries "${vtk_java_native_libraries}")

set(CMAKE_INSTALL_JARDIR ""
  CACHE STRING "Java jar files (LIBDIR/java)")
mark_as_advanced(CMAKE_INSTALL_JARDIR)

# XXX: Handle any cache entries which use `<LIBDIR>` replacements.
if (CMAKE_INSTALL_JARDIR STREQUAL "<LIBDIR>/java")
  # If using the default, blow it away.
  set_property(CACHE CMAKE_INSTALL_JARDIR
    PROPERTY VALUE "")
endif ()
if (CMAKE_INSTALL_JARDIR MATCHES "<LIBDIR>")
  # Replace it in any other usage.
  string(REPLACE "<LIBDIR>" "${CMAKE_INSTALL_LIBDIR}" CMAKE_INSTALL_JARDIR "${CMAKE_INSTALL_JARDIR}")
  set_property(CACHE CMAKE_INSTALL_JARDIR
    PROPERTY VALUE "")
endif ()
# XXX: See above

if (NOT CMAKE_INSTALL_JARDIR)
  set(CMAKE_INSTALL_JARDIR
    "${CMAKE_INSTALL_LIBDIR}/java")
endif ()
if (IS_ABSOLUTE "${CMAKE_INSTALL_JARDIR}")
  message(FATAL_ERROR
    "The `CMAKE_INSTALL_JARDIR` must not be an absolute path.")
endif ()
file(RELATIVE_PATH jnilib_relative_path
  "/prefix/${CMAKE_INSTALL_JARDIR}"
  "/prefix/${CMAKE_INSTALL_JNILIBDIR}")

set(java_configure_sources
  vtk/vtkNativeLibrary
  vtk/vtkSettings)

file(RELATIVE_PATH _vtk_add_executable_relpath
  "/prefix/${_vtk_build_RUNTIME_DESTINATION}"
  "/prefix/${_vtk_build_LIBRARY_DESTINATION}")

foreach (java_file IN LISTS java_configure_sources)
  set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${java_file}.java")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${java_file}.java"
    @ONLY)

  list(APPEND java_input_files
    "${output_file}")
endforeach ()

# Set the javac source version
set(VTK_JAVA_RELEASE_VERSION "<DEFAULT>" CACHE STRING "javac release version")
mark_as_advanced(VTK_JAVA_RELEASE_VERSION)

if (VTK_JAVA_RELEASE_VERSION STREQUAL "<DEFAULT>")
  set(VTK_JAVA_RELEASE_VERSION "8")
endif ()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_JARDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_JARDIR}")

set(classpath_separator ":")
if (WIN32)
  set(classpath_separator "\\;")
endif ()
set(java_classpath
  "${CMAKE_CURRENT_BINARY_DIR}")
if (VTK_JAVA_JOGL_COMPONENT)
  list(APPEND java_classpath
    "$<TARGET_PROPERTY:JOGL::glue,LOCATION>"
    "$<TARGET_PROPERTY:JOGL::JOGL,LOCATION>")
endif ()
if (VTK_JAVA_SWT_COMPONENT)
  list(APPEND java_classpath
    ${SWT_ECLIPSE_LIBRARIES})
endif ()
string(REPLACE ";" "${classpath_separator}" java_classpath "${java_classpath}")

add_library(vtkjava STATIC
  ${java_input_files})
set_source_files_properties(${java_input_files} PROPERTIES GENERATED TRUE)
add_library(VTK::vtkjava ALIAS vtkjava)
set_property(TARGET vtkjava
  PROPERTY
    LINKER_LANGUAGE Java)
set_property(TARGET vtkjava
  PROPERTY
    OUTPUT_NAME "vtk")
target_include_directories(vtkjava
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(vtkjava
  PRIVATE
    # TODO(java): Why does adding this to the include directories not work?
    "SHELL:-classpath \"${java_classpath}\""
    "SHELL:--release ${VTK_JAVA_RELEASE_VERSION}")
target_link_libraries(vtkjava
  PRIVATE
    $<$<BOOL:${VTK_JAVA_SWT_COMPONENT}>:${SWT_ECLIPSE_LIBRARIES}>
    "$<$<BOOL:${VTK_JAVA_JOGL_COMPONENT}>:JOGL::JOGL>")

install(
  TARGETS vtkjava
  EXPORT VTKJava
  COMPONENT "jar"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_JARDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_JARDIR}")
export(
  EXPORT    VTKJava
  NAMESPACE VTK::
  FILE      "${CMAKE_BINARY_DIR}/${vtk_cmake_destination}/VTKJava-targets.cmake")
install(
  EXPORT    VTKJava
  NAMESPACE VTK::
  FILE      "VTKJava-targets.cmake"
  DESTINATION "${vtk_cmake_destination}"
  COMPONENT "development")

if (CMAKE_GENERATOR MATCHES "Ninja")
  # We need this for targets that depends in vtkjava to delay its
  # source code compilation until the vtkjava JAR file is generated.
  add_custom_target(vtkjava_jar DEPENDS "$<TARGET_FILE:vtkjava>")
endif ()

if (VTK_BUILD_TESTING AND TARGET VTK::TestingRendering)
  add_subdirectory(Testing)
endif ()

if (NOT VTK_ABI_NAMESPACE_NAME STREQUAL "<DEFAULT>" AND NOT DEFINED ENV{CI})
  message(WARNING "Wrappings::Java ABI does not not support the VTK_ABI_NAMESPACE_NAME "
                  "and the symbols will not be mangled.")
endif ()

# Add the option to package VTK for custom Java packaging
option(VTK_BUILD_MAVEN_PKG "Use the Java rules to build the native libraries." OFF)

if (VTK_BUILD_MAVEN_PKG)
  # Make sure we embed the runtime libraries when packaging the Java binaries
  if (WIN32)
    # install system runtimes.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "bin")
    include(InstallRequiredSystemLibraries)
  endif ()
  if (APPLE)
    set(CMAKE_INSTALL_NAME_DIR @loader_path)
  endif ()
  if (NOT MAVEN_LOCAL_NATIVE_NAME)
    # We ignore CMAKE_SYSTEM_PROCESSOR since classifier field in maven only accept characters strings (no numbers)
    set(MAVEN_LOCAL_NATIVE_NAME "${CMAKE_SYSTEM_NAME}" CACHE STRING "")
  endif ()
  if (NOT MAVEN_NATIVE_ARTIFACTS)
    set(MAVEN_NATIVE_ARTIFACTS "${MAVEN_LOCAL_NATIVE_NAME}" CACHE STRING "")
  endif ()
  if (NOT MAVEN_LOCAL_REPOSITORY)
    set(MAVEN_LOCAL_REPOSITORY "\${user.home}/vtk-maven-repos" CACHE STRING "")
  endif ()
  if (NOT MAVEN_VTK_GROUP_ID)
    set(MAVEN_VTK_GROUP_ID "org.vtk" CACHE STRING "")
  endif ()
  if (NOT MAVEN_VTK_ARTIFACT_SUFFIX)
    set(MAVEN_VTK_ARTIFACT_SUFFIX "" CACHE STRING "")
  endif ()
  if (NOT MAVEN_VTK_SNAPSHOT)
    set(MAVEN_VTK_SNAPSHOT "" CACHE STRING "")
  endif ()

  mark_as_advanced(
    VTK_BUILD_MAVEN_PKG
    MAVEN_LOCAL_REPOSITORY
    MAVEN_NATIVE_ARTIFACTS
    MAVEN_LOCAL_NATIVE_NAME
    MAVEN_VTK_ARTIFACT_SUFFIX
    MAVEN_VTK_SNAPSHOT
    MAVEN_VTK_GROUP_ID)
  set(MAVEN_DEPENDENCY_XML)
endif ()

if (VTK_JAVA_JOGL_COMPONENT OR VTK_JAVA_SWT_COMPONENT)
  if (WIN32)
    set(MAVEN_REPO "$ENV{HOMEPATH}/.m2/repository")
  else ()
    set(MAVEN_REPO "$ENV{HOME}/.m2/repository")
  endif ()
  find_file(JOGL_LIB jogl-all-${JOGL_VERSION}.jar PATHS ${MAVEN_REPO}/org/jogamp/jogl/jogl-all/${JOGL_VERSION} )
  find_file(JOGL_GLUE gluegen-rt-${JOGL_VERSION}.jar PATHS ${MAVEN_REPO}/org/jogamp/gluegen/gluegen-rt/${JOGL_VERSION})
  mark_as_advanced(JOGL_LIB JOGL_GLUE)
  set(vars)
  if (VTK_JAVA_JOGL_COMPONENT)
    list(APPEND vars VTK_JAVA_JOGL_COMPONENT)
  endif ()
  if (VTK_JAVA_SWT_COMPONENT)
    list(APPEND vars VTK_JAVA_SWT_COMPONENT)
  endif ()
  string(REPLACE ";" ", " vars "${vars}")
  if (NOT JOGL_LIB)
    message(SEND_ERROR
      "CMake can not build VTK with ${vars} ON if the jar JOGL file is not "
      "provided. Please set the JOGL_LIB variable.")
  endif ()
  if (NOT JOGL_GLUE)
    message(SEND_ERROR
      "CMake can not build VTK with ${vars} ON if the jar GLUE file is not "
      "provided. Please set the JOGL_GLUE variable.")
  endif ()
endif ()

# javac runs out memory on Mac so increase heapsize
if (APPLE)
  set(JAVAC_OPTIONS -J-Xmx512m)
endif ()

if (NOT VTK_JAR_PATH)
  set(VTK_JAR_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif ()

if (JAVA_AWT_LIBRARY)
  foreach (__java_library IN LISTS JAVA_AWT_LIBRARY)
    get_filename_component(JAVA_LIB_DIR ${__java_library} PATH)
    if (EXISTS ${JAVA_LIB_DIR}/xawt)
      link_directories(${JAVA_LIB_DIR}/xawt)
    endif ()
    if (EXISTS ${JAVA_LIB_DIR}/client)
      link_directories(${JAVA_LIB_DIR}/client)
    endif ()
    if (EXISTS ${JAVA_LIB_DIR}/server)
      link_directories(${JAVA_LIB_DIR}/server)
    endif ()
  endforeach ()
endif ()

if(WIN32)
  set(SEPARATOR "\\;")
else()
  set(SEPARATOR ":")
endif()

#-----------------------------------------------------------------------------
# Post-Install script for Custom Java Packaging
# This install rule MUST stay at the bottom of that CMakeLists file as
# it has to be the last install rule that get executed
if(VTK_BUILD_MAVEN_PKG)
  set(MAVEN_NATIVE_ARTIFACT_XML)
  foreach(native_name ${MAVEN_NATIVE_ARTIFACTS})
    string(APPEND MAVEN_NATIVE_ARTIFACT_XML
      "\n                <artifact><file>vtk${MAVEN_VTK_ARTIFACT_SUFFIX}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}-natives-${native_name}.jar</file><classifier>natives-${native_name}</classifier><type>jar</type></artifact>")
  endforeach()
  if(VTK_JAVA_JOGL_COMPONENT)
    string(APPEND MAVEN_DEPENDENCY_XML "\n      <dependency><groupId>org.jogamp.jogl</groupId><artifactId>jogl-all-main</artifactId><version>${JOGL_VERSION}</version></dependency>\n      <dependency><groupId>org.jogamp.gluegen</groupId><artifactId>gluegen-rt-main</artifactId><version>${JOGL_VERSION}</version></dependency>")
  endif()
  configure_file(JavaInstall.cmake.in
                 ${CMAKE_CURRENT_BINARY_DIR}/JavaInstall.cmake @ONLY)
  configure_file(Maven/pom.xml.in
                 ${CMAKE_CURRENT_BINARY_DIR}/pom.xml @ONLY)
  configure_file(Maven/README.txt.in
                 ${CMAKE_CURRENT_BINARY_DIR}/MAVEN-README.txt @ONLY)
  install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/JavaInstall.cmake)
endif()
