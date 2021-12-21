set(classes
  vtkIOSSReader
  vtkIOSSWriter)

set(private_classes
  vtkIOSSFilesScanner
  vtkIOSSUtilities
  vtkIOSSModel)

vtk_module_add_module(VTK::IOIOSS
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})
