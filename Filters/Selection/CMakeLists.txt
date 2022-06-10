set(classes
  vtkCellDistanceSelector
  vtkKdTreeSelector
  vtkLinearSelector)

vtk_module_add_module(VTK::FiltersSelection
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersSelection)
