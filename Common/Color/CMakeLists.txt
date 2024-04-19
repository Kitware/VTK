set(classes
  vtkColorSeries
  vtkNamedColors)

vtk_module_add_module(VTK::CommonColor
  CLASSES ${classes})
vtk_add_test_mangling(VTK::CommonColor)
