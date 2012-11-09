set(Module_SRCS
  vtkKdTreeSelector.cxx
  vtkLinearSelector.cxx
  vtkCellDistanceSelector.cxx
  )

vtk_module_library(vtkFiltersSelection ${Module_SRCS})
