set(Module_SRCS
  vtkPDataSetGhostGenerator.cxx
  vtkPStructuredGridConnectivity.cxx
  vtkPStructuredGridGhostDataGenerator.cxx
  vtkPUniformGridGhostDataGenerator.cxx
  vtkPUnstructuredGridConnectivity.cxx
  vtkPUnstructuredGridGhostDataGenerator.cxx
  )

set_source_files_properties(
  vtkPDataSetGhostGenerator
  ABSTRACT
  )

vtk_module_library(vtkFiltersParallelGeometry ${Module_SRCS})
