cmake_minimum_required(VERSION 2.8.8)

# Clear the contents of the file
file(WRITE "${RSP_FILENAME}.tmp" "")

file(GLOB vtk_java_files "${VTK_BINARY_DIR}/java/vtk/*.java")
file(GLOB vtk_java_rendering_files "${VTK_BINARY_DIR}/java/vtk/rendering/*.java")
file(GLOB vtk_java_rendering_awt_files "${VTK_BINARY_DIR}/java/vtk/rendering/awt/*.java")
file(GLOB vtk_java_sample_files "${VTK_BINARY_DIR}/java/vtk/sample/*.java")

foreach( java_file IN LISTS vtk_java_files vtk_java_rendering_files
                            vtk_java_rendering_awt_files vtk_java_sample_files)
  file(APPEND "${RSP_FILENAME}.tmp" "${java_file}\n")
endforeach()

if(VTK_JAVA_SWT_COMPONENT)
  file(GLOB vtk_java_swt_files "${VTK_BINARY_DIR}/java/vtk/rendering/swt/*.java")
  foreach( java_file IN LISTS vtk_java_swt_files )
    file(APPEND "${RSP_FILENAME}.tmp" "${java_file}\n")
  endforeach()
endif()

if(VTK_JAVA_JOGL_COMPONENT)
  file(GLOB vtk_java_jogl_files "${VTK_BINARY_DIR}/java/vtk/rendering/jogl/*.java")
  list(APPEND vtk_java_jogl_files "${VTK_BINARY_DIR}/java/vtk/sample/rendering/JoglConeRendering.java")
  foreach( java_file IN LISTS vtk_java_jogl_files )
    file(APPEND "${RSP_FILENAME}.tmp" "${java_file}\n")
  endforeach()
endif()

configure_file("${RSP_FILENAME}.tmp" "${RSP_FILENAME}" COPYONLY)

file(REMOVE "${RSP_FILENAME}.tmp")
