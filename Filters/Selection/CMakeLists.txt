set(classes
  vtkKdTreeSelector
  vtkLinearSelector
  vtkCellDistanceSelector)

vtk_module_add_module(VTK::FiltersSelection
  CLASSES ${classes})
