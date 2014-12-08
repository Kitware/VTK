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
  )

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  list (APPEND Module_SRCS
    vtkGL2PSExporter.cxx
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
