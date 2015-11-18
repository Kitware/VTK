set(Module_SRCS
  vtkPDataSetGhostGenerator.cxx
  vtkPStructuredGridConnectivity.cxx
  vtkPStructuredGridGhostDataGenerator.cxx
  vtkPUniformGridGhostDataGenerator.cxx
  vtkPUnstructuredGridGhostCellsGenerator.cxx
  )

if (NOT VTK_LEGACY_REMOVE)
  # only add those classes if not removing legacy code.
  list(APPEND Module_SRCS
    vtkPUnstructuredGridConnectivity.cxx
    vtkPUnstructuredGridGhostDataGenerator.cxx)
endif()

set_source_files_properties(
  vtkPDataSetGhostGenerator
  ABSTRACT
  )

vtk_module_library(vtkFiltersParallelGeometry ${Module_SRCS})
