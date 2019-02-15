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

vtk_module_add_module(VTK::FiltersGeometry
  CLASSES ${classes})
