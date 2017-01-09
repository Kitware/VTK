cmake_minimum_required(VERSION 3.3)

file(GLOB vtk_java_files "${VTK_BINARY_DIR}/java/vtk/*.java")
file(GLOB vtk_java_rendering_files "${VTK_BINARY_DIR}/java/vtk/rendering/*.java")
file(GLOB vtk_java_rendering_awt_files "${VTK_BINARY_DIR}/java/vtk/rendering/awt/*.java")
file(GLOB vtk_java_sample_files "${VTK_BINARY_DIR}/java/vtk/sample/*.java")

set(all_vtk_java_files vtk_java_files vtk_java_rendering_files vtk_java_rendering_awt_files vtk_java_sample_files)

if(VTK_JAVA_SWT_COMPONENT)
  file(GLOB vtk_java_swt_files "${VTK_BINARY_DIR}/java/vtk/rendering/swt/*.java")
  list(APPEND all_vtk_java_files vtk_java_swt_files)
endif()

if(VTK_JAVA_JOGL_COMPONENT)
  file(GLOB vtk_java_jogl_files "${VTK_BINARY_DIR}/java/vtk/rendering/jogl/*.java")
  list(APPEND vtk_java_jogl_files "${VTK_BINARY_DIR}/java/vtk/sample/rendering/JoglConeRendering.java")
  list(APPEND all_vtk_java_files vtk_java_jogl_files)
endif()

set(CMAKE_CONFIGURABLE_FILE_CONTENT)
foreach( java_file IN LISTS ${all_vtk_java_files} )
  set(CMAKE_CONFIGURABLE_FILE_CONTENT "${CMAKE_CONFIGURABLE_FILE_CONTENT}\"${java_file}\"\n")
endforeach()

configure_file(
  "${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
  "${RSP_FILENAME}"
  @ONLY)
