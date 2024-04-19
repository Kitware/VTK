set(classes
  vtkDistributedPointCloudFilter
  vtkPExtractGrid
  vtkPExtractRectilinearGrid
  vtkPExtractVOI
  vtkStructuredImplicitConnectivity)

vtk_module_add_module(VTK::FiltersParallelMPI
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersParallelMPI)
