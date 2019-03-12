set(classes
  vtkDistributedPointCloudFilter
  vtkPExtractGrid
  vtkPExtractRectilinearGrid
  vtkPExtractVOI
  vtkStructuredImplicitConnectivity)

vtk_module_add_module(VTK::FiltersParallelMPI
  CLASSES ${classes})
