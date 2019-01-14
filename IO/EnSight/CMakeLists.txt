set(classes
  vtkEnSight6BinaryReader
  vtkEnSight6Reader
  vtkEnSightGoldBinaryReader
  vtkEnSightGoldReader
  vtkEnSightMasterServerReader
  vtkEnSightReader
  vtkGenericEnSightReader)

vtk_module_add_module(VTK::IOEnSight
  CLASSES ${classes})
