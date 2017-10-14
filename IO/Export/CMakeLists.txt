set(Module_SRCS
  vtkExporter.cxx
  vtkGL2PSExporter.cxx
  vtkIVExporter.cxx
  vtkOBJExporter.cxx
  vtkOOGLExporter.cxx
  vtkPDFContextDevice2D.cxx
  vtkPDFExporter.cxx
  vtkPOVExporter.cxx
  vtkRIBExporter.cxx
  vtkRIBLight.cxx
  vtkRIBProperty.cxx
  vtkSVGContextDevice2D.cxx
  vtkSVGExporter.cxx
  vtkVRMLExporter.cxx
  vtkX3D.cxx
  vtkX3DExporter.cxx
  vtkX3DExporterFIWriter.cxx
  #vtkX3DExporterJavaHelper.cxx Needs JNI
  vtkX3DExporterWriter.cxx
  vtkX3DExporterXMLWriter.cxx
  )

set_source_files_properties(
  vtkPDFContextDevice2D
  vtkSVGContextDevice2D
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
include(vtkOpenGL)
vtk_opengl_link(vtkIOExport)
