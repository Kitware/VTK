set(Module_SRCS
  vtkExporter.cxx
  vtkIVExporter.cxx
  vtkOBJExporter.cxx
  vtkOOGLExporter.cxx
  vtkPOVExporter.cxx
  vtkRIBExporter.cxx
  vtkRIBLight.cxx
  vtkRIBProperty.cxx
  vtkVRMLExporter.cxx
  vtkX3D.cxx
  vtkX3DExporter.cxx
  vtkX3DExporterFIWriter.cxx
  #vtkX3DExporterJavaHelper.cxx Needs JNI
  vtkX3DExporterWriter.cxx
  vtkX3DExporterXMLWriter.cxx
  vtkGL2PSExporter.cxx
  )

if(NOT VTK_USE_SYSTEM_GL2PS)
  # TODO: Build vtkgl2ps library over in its module
  list(APPEND Module_SRCS ${VTK_SOURCE_DIR}/ThirdParty/gl2ps/vtkgl2ps/gl2ps.c)
  add_definitions(-DGL2PS_HAVE_ZLIB)
  add_definitions(-DGL2PS_HAVE_LIBPNG)
  set_source_files_properties(
    ${VTK_SOURCE_DIR}/ThirdParty/gl2ps/vtkgl2ps/gl2ps
    WRAP_EXCLUDE
  )
endif()

set_source_files_properties(
  vtkX3D
  vtkX3DExporterWriter
  vtkX3DExporterFIWriter
  vtkX3DExporterXMLWriter
  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkExporter
  ABSTRACT
  )

vtk_module_library(vtkIOExport ${Module_SRCS})
