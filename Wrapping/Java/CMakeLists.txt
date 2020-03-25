include(vtkModuleWrapJava)
vtk_module_wrap_java(
  JAVA_OUTPUT     "${CMAKE_CURRENT_BINARY_DIR}/src/vtk"
  MODULES         ${vtk_modules}
  WRAPPED_MODULES vtk_java_wrapped_modules)

option(VTK_JAVA_SWT_COMPONENT "Enable the SWT component for Java" OFF)
if (VTK_JAVA_SWT_COMPONENT)
  find_package(SWT REQUIRED)
endif ()

# Add the option for build the JOGL component to VTK.
set(JOGL_VERSION "2.3.2")
option(VTK_JAVA_JOGL_COMPONENT "Need JOGL jar files" OFF)

# TODO: Make FindJava.cmake use imported targets.
find_package(Java REQUIRED COMPONENTS Development)

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

set(vtk_java_class_output
  "${CMAKE_CURRENT_BINARY_DIR}/class")
file(MAKE_DIRECTORY "${vtk_java_class_output}")

set(java_input_files)
set(java_class_files)
foreach (java_file IN LISTS java_sources)
  set(output_file "${CMAKE_CURRENT_BINARY_DIR}/src/${java_file}.java")
  add_custom_command(
    OUTPUT  "${output_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java"
            "${output_file}"
    COMMENT "Copying ${java_file} to the binary directory")

  list(APPEND java_input_files
    "${output_file}")
  list(APPEND java_class_files
    "${vtk_java_class_output}/${java_file}.class")
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
  #list(APPEND java_class_files
  #  "${vtk_java_class_output}/${java_file}.class")

  string(APPEND vtk_java_native_libraries
    "  ${_vtk_java_library_name}(\"${_vtk_java_library_name}Java\"),\n")
endforeach ()

# Replace the last comma with a semicolon.
string(REGEX REPLACE ",\n$" ";\n" vtk_java_native_libraries "${vtk_java_native_libraries}")

set(java_configure_sources
  vtk/vtkNativeLibrary
  vtk/vtkSettings)

foreach (java_file IN LISTS java_configure_sources)
  set(output_file "${CMAKE_CURRENT_BINARY_DIR}/src/${java_file}.java")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/${java_file}.java"
    @ONLY)

  list(APPEND java_input_files
    "${output_file}")
  list(APPEND java_class_files
    "${vtk_java_class_output}/${java_file}.class")
endforeach ()

set(vtk_java_rsp_file
  "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/java_files.rsp")

file(GENERATE
  OUTPUT  "${vtk_java_rsp_file}"
  CONTENT "\'$<JOIN:${java_input_files},\'\n\'>\'\n")

# Set the javac source version
set(VTK_JAVA_SOURCE_VERSION "1.6" CACHE STRING "javac source version")
mark_as_advanced(VTK_JAVA_SOURCE_VERSION)
set(VTK_JAVA_TARGET_VERSION "1.6" CACHE STRING "javac target version")
mark_as_advanced(VTK_JAVA_TARGET_VERSION)

set(vtk_compile_classpath)
if (VTK_JAVA_SWT_COMPONENT)
  list(APPEND vtk_compile_classpath
    ${ECLIPSE_SWT_LIBRARIES})
endif ()
if (VTK_JAVA_JOGL_COMPONENT)
  list(APPEND vtk_compile_classpath
    ${JOGL_LIB}
    ${JOGL_GLUE})
endif ()

set(vtk_java_classpath_arguments)
if (vtk_compile_classpath)
  set(classpath_separator ":")
  if (WIN32)
    set(classpath_separator "\\;")
  endif ()

  string(REPLACE ";" "${classpath_separator}" vtk_compile_classpath "${vtk_compile_classpath}")
  set(vtk_java_classpath_arguments
    -classpath "${vtk_compile_classpath}")
endif ()

add_custom_command(
  OUTPUT  ${java_class_files}
  DEPENDS ${java_input_files}
  COMMAND "${Java_JAVAC_EXECUTABLE}"
          ${vtk_java_classpath_arguments}
          -source "${VTK_JAVA_SOURCE_VERSION}"
          -target "${VTK_JAVA_TARGET_VERSION}"
          -d "${vtk_java_class_output}"
          "@${vtk_java_rsp_file}"
  COMMENT "Compiling Java Classes")

set(vtk_jar_file
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vtk.jar")

add_custom_command(
  OUTPUT  "${vtk_jar_file}"
  DEPENDS ${java_class_files}
  COMMAND "${Java_JAR_EXECUTABLE}"
          -cvf "${vtk_jar_file}"
          -C "${vtk_java_class_output}"
          vtk
  COMMENT "Creating vtk.jar")

add_custom_target(vtkjava ALL
  DEPENDS
    "${vtk_jar_file}")

set_directory_properties(PROPERTIES
  ADDITIONAL_MAKE_CLEAN_FILES
    "${java_class_files};${vtk_jar_file}")
#install(
#  FILES       "${vtk_jar_file}"
#  DESTINATION "${VTK_INSTALL_JAVA_DIR}"
#  COMPONENT   "runtime")

if (BUILD_TESTING AND TARGET VTK::TestingRendering)
  add_subdirectory(Testing)
endif ()
