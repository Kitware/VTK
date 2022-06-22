set(classes
  vtkAbstractGridConnectivity
  vtkAttributeSmoothingFilter
  vtkCompositeDataGeometryFilter
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
  vtkStructuredGridPartitioner
  vtkStructuredNeighbor
  vtkStructuredPointsGeometryFilter
  vtkUnstructuredGridGeometryFilter)

vtk_module_add_module(VTK::FiltersGeometry
  CLASSES ${classes})
