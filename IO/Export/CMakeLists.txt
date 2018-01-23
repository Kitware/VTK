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
  vtkSingleVTPExporter.cxx
  vtkVRMLExporter.cxx
  vtkX3D.cxx
  vtkX3DExporter.cxx
  vtkX3DExporterFIWriter.cxx
  #vtkX3DExporterJavaHelper.cxx Needs JNI
  vtkX3DExporterWriter.cxx
  vtkX3DExporterXMLWriter.cxx
  )

vtk_module_library(vtkIOExport ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(vtkIOExport)
