set(classes
  vtkPCellSizeFilter)

vtk_module_add_module(VTK::FiltersParallelVerdict
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersParallelVerdict)
