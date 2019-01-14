set(classes
  vtkCellQuality
  vtkCellSizeFilter
  vtkMatrixMathFilter
  vtkMeshQuality)

vtk_module_add_module(VTK::FiltersVerdict
  CLASSES ${classes})
