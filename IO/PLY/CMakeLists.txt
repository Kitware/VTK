set(classes
  vtkPLY
  vtkPLYReader
  vtkPLYWriter)

vtk_module_add_module(VTK::IOPLY
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOPLY)
