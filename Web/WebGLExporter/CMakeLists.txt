# The exporter will behave as any other ParaView exporter (VRML, X3D, POV...)
# but will generate several types of files. The main one is the scene graph
# description define as a JSON object with all the corresponding binary+base64
# pieces that come along with it. But also with it come a single standalone HTML
# file that can directly be used to see the data in a browser without any plugin.
#
# This code base should be cleaned up to follow VTK standard and even be
# integrated into VTK itself. But for now it is provided as is.

set(classes
  vtkPVWebGLExporter
  vtkWebGLDataSet
  vtkWebGLExporter
  vtkWebGLObject
  vtkWebGLPolyData
  vtkWebGLWidget)

set(javascript_files
  webglRenderer.js
  glMatrix.js)

set(sources)
set(private_headers)

foreach (javascript_file IN LISTS javascript_files)
  vtk_encode_string(
    INPUT         "${javascript_file}"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources
    ${source})
  list(APPEND private_headers
    ${header})
endforeach ()

vtk_module_add_module(VTK::WebGLExporter
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
