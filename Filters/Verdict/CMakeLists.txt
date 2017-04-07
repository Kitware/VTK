set(Module_SRCS
  vtkCellQuality.cxx
  vtkCellSizeFilter.cxx
  vtkMatrixMathFilter.cxx
  vtkMeshQuality.cxx
  )

vtk_module_library(vtkFiltersVerdict ${Module_SRCS})
