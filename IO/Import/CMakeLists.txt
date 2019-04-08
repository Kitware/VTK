set(classes
  vtk3DSImporter
  vtkGLTFImporter
  vtkImporter
  vtkVRMLImporter
  vtkOBJImporter
  vtkOBJImporterInternals)

set(headers
  vtk3DS.h)

vtk_module_add_module(VTK::IOImport
  CLASSES ${classes}
  HEADERS ${headers})
