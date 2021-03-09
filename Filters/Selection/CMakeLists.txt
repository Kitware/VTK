set(classes
  vtkCellDistanceSelector
  vtkKdTreeSelector
  vtkLinearSelector)

vtk_module_add_module(VTK::FiltersSelection
  CLASSES ${classes})
