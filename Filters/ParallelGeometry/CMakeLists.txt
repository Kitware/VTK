include(vtkObjectFactory)

set(Module_SRCS
  vtkPConnectivityFilter.cxx
  vtkPDataSetGhostGenerator.cxx
  vtkPStructuredGridConnectivity.cxx
  vtkPStructuredGridGhostDataGenerator.cxx
  vtkPUniformGridGhostDataGenerator.cxx
  vtkPUnstructuredGridGhostCellsGenerator.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )

# Now to generate our object factory.
vtk_add_override(vtkUnstructuredGridGhostCellsGenerator vtkPUnstructuredGridGhostCellsGenerator)
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(vtkFiltersParallelGeometry ${Module_SRCS})
