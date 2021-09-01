set(classes
  vtkIOSSReader)

set(private_classes
  vtkIOSSFilesScanner
  vtkIOSSUtilities)

vtk_module_add_module(VTK::IOIOSS
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})
