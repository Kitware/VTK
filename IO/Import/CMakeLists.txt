set(classes
  vtk3DSImporter
  vtkGLTFImporter
  vtkImporter
  vtkVRMLImporter
  vtkOBJImporter)

set(nowrap_classes
  vtkOBJImporterInternals)

set(headers
  vtk3DS.h)

vtk_module_add_module(VTK::IOImport
  CLASSES ${classes}
  NOWRAP_CLASSES  ${nowrap_classes}
  HEADERS ${headers})
