set(Module_SRCS
  vtkCellQuality.cxx
  vtkMatrixMathFilter.cxx
  vtkMeshQuality.cxx
  )

vtk_module_library(vtkFiltersVerdict ${Module_SRCS})
