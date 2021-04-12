set(classes
  vtkAbstractGridConnectivity
  vtkCompositeDataGeometryFilter
  vtkDataSetGhostGenerator
  vtkDataSetRegionSurfaceFilter
  vtkDataSetSurfaceFilter
  vtkExplicitStructuredGridSurfaceFilter
  vtkGeometryFilter
  vtkHierarchicalDataSetGeometryFilter
  vtkImageDataGeometryFilter
  vtkImageDataToUniformGrid
  vtkLinearToQuadraticCellsFilter
  vtkMarkBoundaryFilter
  vtkProjectSphereFilter
  vtkRectilinearGridGeometryFilter
  vtkRectilinearGridPartitioner
  vtkStructuredAMRGridConnectivity
  vtkStructuredAMRNeighbor
  vtkStructuredGridConnectivity
  vtkStructuredGridGeometryFilter
  vtkStructuredGridGhostDataGenerator
  vtkStructuredGridPartitioner
  vtkStructuredNeighbor
  vtkStructuredPointsGeometryFilter
  vtkUniformGridGhostDataGenerator
  vtkUnstructuredGridGeometryFilter)

set(templates
  vtkDataSetSurfaceFilter.txx)

vtk_module_add_module(VTK::FiltersGeometry
  CLASSES ${classes}
  TEMPLATES ${templates})
