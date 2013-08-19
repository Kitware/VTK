# The exporter will behave as any other ParaView exporter (VRML, X3D, POV...)
# but will generate several types of files. The main one is the scene graph
# description define as a JSON object with all the corresponding binary+base64
# pieces that come along with it. But also with it come a single standalone HTML
# file that can directly be used to see the data in a browser without any plugin.
#
# This code base should be cleaned up to follow VTK standard and even be
# integrated into VTK itself. But for now it is provided as is.


set(Module_SRCS
  vtkPVWebGLExporter.cxx
  vtkWebGLDataSet.cxx
  vtkWebGLExporter.cxx
  vtkWebGLObject.cxx
  vtkWebGLPolyData.cxx
  vtkWebGLWidget.cxx
)

set_source_files_properties(md5.cxx WRAP_EXCLUDE)

set(JAVASCRIPT_SRCS
  webglRenderer
  glMatrix
)


foreach(file ${JAVASCRIPT_SRCS})
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/${file}.js)
  set(res ${CMAKE_CURRENT_BINARY_DIR}/${file}.h)
  add_custom_command(
    OUTPUT ${res}
    DEPENDS ${src}
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file}
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${res} WRAP_EXCLUDE)
endforeach()

vtk_module_library(${vtk-module} ${Module_SRCS})
