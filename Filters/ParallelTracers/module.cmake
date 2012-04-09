vtk_module(vtkFiltersParallelTracers
  DEPENDS
    vtkFiltersTracers
    vtkParallelCore
    vtkFiltersParallel
  TEST_DEPENDS
    vtkRenderingParallel
  )
