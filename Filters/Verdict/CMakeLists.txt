set(classes
  vtkBoundaryMeshQuality
  vtkCellQuality
  vtkCellSizeFilter
  vtkMatrixMathFilter
  vtkMeshQuality)

vtk_module_add_module(VTK::FiltersVerdict
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersVerdict)
