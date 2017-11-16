set(Module_SRCS
  vtkPDataSetGhostGenerator.cxx
  vtkPStructuredGridConnectivity.cxx
  vtkPStructuredGridGhostDataGenerator.cxx
  vtkPUniformGridGhostDataGenerator.cxx
  vtkPUnstructuredGridGhostCellsGenerator.cxx
  )

vtk_module_library(vtkFiltersParallelGeometry ${Module_SRCS})
