set(classes
  vtkDistributedPointCloudFilter
  vtkHyperTreeGridRedistribute
  vtkPExtractGrid
  vtkPExtractRectilinearGrid
  vtkPExtractVOI
  vtkPMultiVariateToRGBA
  vtkStructuredImplicitConnectivity)

vtk_module_add_module(VTK::FiltersParallelMPI
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersParallelMPI)
