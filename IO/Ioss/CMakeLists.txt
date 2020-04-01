set(classes
  vtkIossReader)

set(private_classes
  vtkIossFilesScanner
  vtkIossUtilities)

vtk_module_add_module(VTK::IOIoss
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})
