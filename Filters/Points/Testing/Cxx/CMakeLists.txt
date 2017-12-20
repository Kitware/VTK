vtk_add_test_cxx(vtkFiltersPointsCxxTests tests
  UnitTestKernels.cxx,NO_VALID
  TestSPHKernels.cxx,NO_VALID
  PlotSPHKernels.cxx
  TestPointCloudFilterArrays.cxx,NO_VALID,NO_DATA
  )
vtk_test_cxx_executable(vtkFiltersPointsCxxTests tests
  RENDERING_FACTORY
  )
