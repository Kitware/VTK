set(classes
  vtkPLY
  vtkPLYReader
  vtkPLYWriter)

vtk_module_add_module(VTK::IOPLY
  CLASSES ${classes})
