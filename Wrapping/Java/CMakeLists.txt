set(CMAKE_INSTALL_JNILIBDIR ""
  CACHE PATH "JNI libraries (LIBDIR/java/vtk-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
mark_as_advanced(CMAKE_INSTALL_JNILIBDIR)
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
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  # Update vtkSettings.java.in if this changes.
  JNILIB_DESTINATION  "${CMAKE_INSTALL_JNILIBDIR}")

# TODO: Make FindJava.cmake use imported targets.
find_package(Java REQUIRED COMPONENTS Development)
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
    COMMENT "Copying ${java_file} to the binary directory")

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
set(VTK_JAVA_SOURCE_VERSION "<DEFAULT>" CACHE STRING "javac source version")
mark_as_advanced(VTK_JAVA_SOURCE_VERSION)
set(VTK_JAVA_TARGET_VERSION "<DEFAULT>" CACHE STRING "javac target version")
mark_as_advanced(VTK_JAVA_TARGET_VERSION)

if (VTK_JAVA_SOURCE_VERSION STREQUAL "<DEFAULT>")
  set(VTK_JAVA_SOURCE_VERSION "1.6")
endif ()
if (VTK_JAVA_TARGET_VERSION STREQUAL "<DEFAULT>")
  set(VTK_JAVA_TARGET_VERSION "1.6")
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
    "SHELL:-source ${VTK_JAVA_SOURCE_VERSION}"
    "SHELL:-target ${VTK_JAVA_TARGET_VERSION}")
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

if (BUILD_TESTING AND TARGET VTK::TestingRendering)
  add_subdirectory(Testing)
endif ()
