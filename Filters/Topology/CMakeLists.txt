set(classes
  vtkFiberSurface)

vtk_module_add_module(VTK::FiltersTopology
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersTopology)
