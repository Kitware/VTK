set(Module_SRCS
  vtkCompositeDataGeometryFilter.cxx
  vtkGeometryFilter.cxx
  vtkHierarchicalDataSetGeometryFilter.cxx
  vtkImageDataGeometryFilter.cxx
  vtkProjectSphereFilter.cxx
  vtkRectilinearGridGeometryFilter.cxx
  vtkStructuredGridGeometryFilter.cxx
  vtkStructuredPointsGeometryFilter.cxx
  vtkUnstructuredGridGeometryFilter.cxx

  vtkDataSetSurfaceFilter.cxx

  vtkAbstractGridConnectivity.cxx
  vtkDataSetGhostGenerator.cxx
  vtkRectilinearGridPartitioner.cxx
  vtkStructuredGridConnectivity.cxx
  vtkStructuredGridGhostDataGenerator.cxx
  vtkStructuredGridPartitioner.cxx
  vtkStructuredNeighbor.cxx
  vtkUniformGridGhostDataGenerator.cxx
  )

set_source_files_properties(
  vtkAbstractGridConnectivity
  vtkDataSetGhostGenerator
  ABSTRACT
  )

set_source_files_properties(
  vtkStructuredNeighbor
  WRAP_EXCLUDE
  )

vtk_module_library(vtkFiltersGeometry ${Module_SRCS})
